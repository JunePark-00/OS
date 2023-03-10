#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define PAGE_N 0x400

typedef struct page_table2{//inner page
        int page_fn[PAGE_N];
        int valid[PAGE_N];
        int residence[PAGE_N];
}pt2;

typedef struct page_table1{//outer page
        pt2* page_t2[PAGE_N];
        int valid[PAGE_N];
}pt;

typedef struct pcb{
        struct pcb* next;
        int pid;
        int io_time;
        int cpu_time;
        int all_wait_time;
        int wait_time;
        int form;
        int cnt_cpu;
        int throuput;
        int for_response;
        int response;
        int response_flag;
        int for_turnaround;
        int turnaround;
        pt* page_t;
}pcb;

typedef struct L{
        pcb *head;
        pcb *tail;
}pcb_L;

pcb_L* create_pcb(void){
        pcb_L *L;
        L = (pcb_L*)malloc(sizeof(pcb_L));
        L->head = NULL;
        L->tail = NULL;
        return L;
}

//프로세스 생성 시 run,ready q에 push
void push_pcb(pcb_L *L, int pid, int cpu_time, int wait_time, int form){
        pcb *newpcb;
        pcb *temp;
        pt *newpt;
        newpcb = (pcb*)malloc(sizeof(pcb));
        newpt = (pt*)malloc(sizeof(pt));
        newpcb->pid = pid;
        newpcb->cpu_time = cpu_time;
        newpcb->all_wait_time = wait_time;
        newpcb->wait_time = wait_time;
        newpcb->form = form;
        newpcb->next = NULL;
        newpcb->page_t = newpt;


        if(L->head == NULL){
                L->head = newpcb;
                newpcb->next = newpcb;
                L->tail = newpcb;
                return;
        } else{
                L->tail->next = newpcb;
                L->tail = newpcb;
                newpcb->next = L->head;
        }

}

//wait q에서 나온 기존 프로세스 push
void push_old_pcb(pcb_L *L , pcb* newpcb, int cpu_time){
        newpcb->cpu_time = cpu_time;
        newpcb->next = NULL;


        if(L->head == NULL){
                L->head = newpcb;
                newpcb->next = newpcb;
                L->tail = newpcb;
                return;
        }
        else{
                L->tail->next = newpcb;
                L->tail = newpcb;
                newpcb->next = L->head;
        }
}


void push_wait(pcb_L *L, pcb* newpcb)   //wait q에 push
{
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));

        if(L->head == NULL){
                L->head = newpcb;
                newpcb->next = NULL;
                return;
        }

        temp = L->head;
        if(temp->next == NULL){
                if(temp->io_time <= newpcb->io_time){
                        temp->next = newpcb;
                        newpcb->next = NULL;
                }
                else{
                        newpcb->next = temp;
                        L->head = newpcb;
                        temp->next = NULL;
                }
        }
        else{
                if(temp->io_time <= newpcb->io_time){
                        if(temp->next != NULL){
                                while(temp->next->io_time <= newpcb->io_time){
                                        temp = temp->next;
                                        if(temp->next == NULL)
                                                break;
                                }
                        }
                        newpcb->next = temp->next;
                        temp->next = newpcb;
                }
                else{
                        newpcb->next = temp;
                        L->head = newpcb;
                }
        }
}

pcb* pop_pcb(pcb_L *L)  //run,ready q에서 pop
{
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        if(L->head == NULL){
                printf("현재 node는 empty\n");
                return NULL;
        }
        else if(L->head->next == L->head){
                temp = L->head;
                L->head = NULL;
                temp->next = NULL;
                L->tail = NULL;
                return temp;
        }

        temp = L->head;
        L->head = L->head->next;
        L->tail->next = L->head;
        temp->next = NULL;
        return temp;
}

pcb* pop_wait(pcb_L *L) //wait q에서 pop
{
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        if(L->head == NULL){
                printf("현재 node는 empty\n");
                return NULL;
        }
        else if(L->head->next == L->head){
                temp = L->head;
                L->head = NULL;
                temp->next = NULL;
                return temp;
        }

        temp = L->head;
        L->head = L->head->next;
        temp->next = NULL;
        return temp;
}
