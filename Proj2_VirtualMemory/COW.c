// Copy On Write

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
#include "pushpop_cow.c"

#define MAX_PROC 10
#define MEM_REQ 10 //number of memory request
#define MEM_SIZE 100 
#define MEM_IDX 0x100000 
#define KEY_VAL 3219
#define PAGE_SIZE 0x1000 //unit of pages
#define END_TICK 10000

typedef struct shared_mem{
        int VM[MEM_REQ];
        int exec_cpu,io_receive,check_run,flag;
}sm;

typedef struct list{
        struct list* next;
        int frame_num;
}list;

typedef struct ll{ // node for free page frame list
 list* head;
 list* tail;
}fpf_head;

pcb *curr, *mmu_ptr;
pcb_L *p_pcb, *waitq;
sm* sm_info = NULL;
fpf_head* create_list();
fpf_head* fpfhead;

int MAIN_MEM[MEM_IDX];
int page_hit = 0;
int page_miss = 0;
int receive_time,check_first,gc,throughput;
float waiting_time = 0;
float response, turn;
int VM[MEM_REQ];
int count_page, process_num;
int fork_num, write_event, write_num, give_write_num;
int read_only_num = 0;
int copy_num, os_pid;
int erq = 0;

void tick_time(int signo);
void io_req();
void do_child();
void dis_cpu();
void exit_program();
void memacc_req();
void m_fork(pcb* parent);
void make_read_only(pcb* parent);
void copy_on_write(pcb* ptr, int index);
void mmu(int vm_idex);
void push_fpfl(fpf_head* point, int pfn);
void push_pcb_fork(pcb_L * L,pcb* parent);


int main(int argc, char *arg[]){
	give_write_num = 10000;
        check_first = 1;
        srand(time(NULL));
        int cpu_time,smid,pid,status;
        void* address = (void*)0;

	os_pid=getpid();
	count_page = 0;
	fork_num = 0;
	copy_num = 0;
        write_event = 0;

        if((smid = shmget(KEY_VAL, 100, IPC_CREAT|0666)) != (-1)){
                address = shmat(smid, NULL, 0);

                printf("Create the Shared Memory____________\n");
                printf("SHARED_MEM_ID : %d\n", smid);
                printf("SHARED_MEM_ADDR : %p\n", address);
                printf("____________________________________\n");
        } else{
                printf("Fail the SM\n");
        }

        sm_info = (sm*)address;
        sm_info->flag = 0;

        p_pcb = create_pcb();
        waitq = create_pcb();
        fpfhead = create_list();

        //SIGALRM
        //move to tick_time()
        struct sigaction old_sa, new_sa;
        memset(&new_sa, 0, sizeof(new_sa));
        new_sa.sa_handler = tick_time;
        sigaction(SIGALRM, &new_sa, &old_sa);

        push_pcb(p_pcb, os_pid);//m_fork

	process_num=1; //start
        
        for(int i = 0; i < MEM_IDX; i++){
                push_fpfl(fpfhead, i);
        }
        //declare 'free page frame list' as global variable

        struct itimerval new_timer, old_timer;
        memset(&new_timer, 0, sizeof(new_timer));
        new_timer.it_interval.tv_sec = 0;       //how often the alarm should be
        new_timer.it_interval.tv_usec = 10000;
        new_timer.it_value.tv_sec = 1;          //set the alarm start time 
        new_timer.it_value.tv_usec = 0;

        setitimer(ITIMER_REAL, &new_timer, &old_timer);

        curr = p_pcb->head;

        while(1){
                sleep(1);
        }

        return 0;

}

//io time consumption of wait queue
void dec_waitq(pcb_L* waitq){
        pcb* temp;
        temp = (pcb*)malloc(sizeof(pcb));
        temp = waitq->head;

        if(waitq->head != NULL){
                temp->io_time--;
                temp->for_turnaround++;
                temp = temp->next;
                while(temp != NULL){
                        temp->io_time--;
                        temp->for_turnaround++;
                        temp =temp->next;
                }
        }
}


//keep the waiting time of each process
void waiting_calc(int check_first, int gc, pcb_L* p_pcb){
        if(check_first != 1 && gc <= 1000){
                pcb* for_wait;
                for_wait = (pcb*)malloc(sizeof(pcb));
                for_wait = p_pcb->head;

                if(for_wait != NULL){
                        for_wait = for_wait->next;

                        while(for_wait != p_pcb->head){
                                for_wait->all_wait_time++;
                                for_wait->wait_time++;
                                for_wait->for_turnaround++;

                                if(for_wait->response_flag == 0){
                                        for_wait->for_response++;
                                }

                        for_wait = for_wait->next;
                        }
                }
        }

}


//wait queue(io time == 0)->run queue
void io_finish(pcb_L* waitq, pcb_L* p_pcb){
        if(waitq->head != NULL){
                while(waitq->head->io_time == 0){

                        waitq->head->throuput++;
                        waitq->head->turnaround += waitq->head->for_turnaround;
                        waitq->head->for_turnaround = 0;
                        waitq->head->response_flag = 0;
                        push_old_pcb(p_pcb, pop_wait(waitq),10);

                        if(waitq->head == NULL)
                                break;
                }
        }
}


//if SIGARM is occured,
void tick_time(int signo){
       gc++;
	fork_num++;
	write_event++;
        printf("_____________________________[%d]tick_____________________________\n", gc - 1);
        if(gc != 1){
                waiting_calc(check_first, gc, p_pcb);
        io_finish(waitq, p_pcb);
	}

   
        if(p_pcb->head == NULL){
		erq++;
                dec_waitq(waitq);
                exit_program();
                return;
        }
	memacc_req();

        if(check_first != 1 && sm_info->check_run == 1){
                sm_info->check_run = 0;
                curr = curr->next;
                p_pcb->head = p_pcb->head->next;
                p_pcb->tail = p_pcb->tail->next;
        }

        mmu_ptr = curr;

        check_first = 0;

        dec_waitq(waitq);

        sm_info->exec_cpu = curr->cpu_time;

        exit_program();

        if(curr->response_flag == 0){
                curr->response_flag = 1;
                curr->response += curr->for_response;
                curr->for_response = 0;
        }

        curr->cnt_cpu++;
        curr->for_turnaround++;
        dis_cpu(curr);


}


//if SIGUSR1 is occured
void io_req(){
        curr->cpu_time = 5;
        curr->io_time = 2;
        curr = curr->next;
        push_wait(waitq, pop_pcb(p_pcb));
}


//if SIGUSR2 is occured
void dis_cpu(){ //cpu is dispatched
        curr->cpu_time--;
       	curr->run_time--;

        for(int i = 0; i < MEM_REQ; i++){
                VM[i] = (rand() & 0x0fffffff);
                sm_info->VM[i] = VM[i];
        }
        sm_info->flag = 1;           

        if(curr->cpu_time == 0){
                
                sm_info->io_receive  = receive_time;
                curr->run_time = 3;
                io_req();
        }
        else if(curr->run_time == 0){
                sm_info->check_run = 1;
                curr->run_time = 3;
        }
}

//when it reached at 10000 tick
void exit_program(){
        if(gc == END_TICK + 1){
                pcb* wtemp;
                wtemp = (pcb*)malloc(sizeof(pcb));
                if(waitq->head != NULL){
                        wtemp = waitq->head;
                        while(wtemp != NULL){
                                waiting_time += wtemp->all_wait_time;
                                throughput += wtemp->throuput;
                                response += (float)(wtemp->response) / wtemp->throuput;
                                turn += (float)(wtemp->turnaround) / wtemp->throuput;
                                wtemp = wtemp->next;
                        }
                }
                if(p_pcb->head != NULL){
                        wtemp = p_pcb->head;
                        int one = 0;
                        while(wtemp != p_pcb->head || one == 0){
                                waiting_time += wtemp->all_wait_time;
                                throughput += wtemp->throuput;
                                response += (float)(wtemp->response) / wtemp->throuput;
                                turn += (float)(wtemp->turnaround) / wtemp->throuput;
                                wtemp = wtemp->next;
                                one = 1;
                        }
                }
		printf("_______________________________END________________________________\n");
                printf("[10000]tick: Exit the Program\n");
		printf("Page hit: %d\n",page_hit);
		printf("Page miss: %d\n",page_miss);
		printf("Copy on write: %d times \n",copy_num);
		printf("Number of writing: %d times\n",write_num);
		printf("Empty ready queue: %d\n",erq);
                exit(0);
        }

}

void push_fpfl(fpf_head* point, int pfn){//push at the end
        list* newpage;
        newpage = (list* )malloc(sizeof(list));
        newpage->frame_num = pfn;
        newpage->next = NULL;

        if(point->head==NULL){
                point->head = newpage;
                point->tail = newpage;
        }
        else{
                point->tail->next = newpage;
                point->tail = newpage;
        }
}

fpf_head* create_list(){
        fpf_head* L;
        L = (fpf_head*)malloc(sizeof(fpf_head));
        L->head = NULL;
        L->tail = NULL;
        return L;
}


void memacc_req(){ //parent process
        if(sm_info->flag == 1){
 	       	int vm_page_index;
               	int vm_offset;
		int pm;
		printf("_____________________(pid)current process : %d___________________ \n",mmu_ptr->pid);
                printf("_______________________________END________________________________\n");

               	for(int c = 0; c < MEM_REQ; c++){
                        VM[c] = sm_info->VM[c];
                        vm_page_index = (VM[c] & 0xFFFFF000) >> 12;
                        vm_offset = (VM[c] & 0x00000FFF);
			printf("____________________________access[%d]____________________________\n", c+1);

			mmu(vm_page_index);
			
                        pm = (mmu_ptr->page_t->pagetable[vm_page_index] << 12) + vm_offset;
			printf("va: %08x\tpage_index%08x\tframe number: %08x\tpa: %08x\n",VM[c] , vm_page_index,mmu_ptr->page_t->pagetable[vm_page_index] ,pm);
	
                        printf("\n");
                }
                sm_info->flag = 0;
        }
	if(process_num < MAX_PROC){
	m_fork(mmu_ptr);
	}
}
void mmu(int vm_index){
	if(write_event > 3){// write event is occured
		write_event = 0;
		copy_on_write(mmu_ptr,vm_index);
	} else{
                if(mmu_ptr->page_t->valid[vm_index] == 0){
		        //write isn't occured
                        mmu_ptr->page_t->pagetable[vm_index] = fpfhead->head->frame_num;
			MAIN_MEM[fpfhead->head->frame_num] = count_page;
			count_page++;
			page_miss++;
                        //give free page
                        if(fpfhead->head->next == NULL){
                                        fpfhead->head = NULL;
                                        fpfhead->tail = NULL;
                        } else{
                                fpfhead->head = fpfhead->head->next;
                        }
                        mmu_ptr->page_t->valid[vm_index]=1;
                } else{
		        page_hit++;
		}
	}

}
void m_fork(pcb* parent){
	if(fork_num > 600)
	{
		fork_num = 0;
		process_num++;
		push_pcb_fork(p_pcb,parent);
	}
}
	
void push_pcb_fork(pcb_L *L, pcb* parent) { //when create the process, push it to the run queue and ready queue
        pcb *newpcb;
        pcb *temp;
        pt *newpt;
        newpcb = (pcb*)malloc(sizeof(pcb));
        newpt = (pt*)malloc(sizeof(pt));
        newpcb->pid = os_pid + process_num ;//different pid
        newpcb->cpu_time = 5;
	newpcb->run_time = 3;
	newpcb->io_time = 2;
        newpcb->all_wait_time = 0;
        newpcb->wait_time = 0;
        newpcb->next = NULL;
	//copy the page table and link it
	make_read_only(parent);

	memcpy(newpt, parent->page_t, sizeof(struct page_table));//둘다 pt 구조체 가리키는 포인터
        newpcb->page_t = newpt;

	

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
	printf("[%d] is forked from [%d]\n",newpcb->pid ,parent->pid);
	printf("Saved page number(valid table entry num): %d\n ", read_only_num);
	read_only_num = 0;

}

void make_read_only(pcb* parent){
	for(int i = 0; i < 0x100000; i++){
		if(parent->page_t->valid[i]==1){//only mapped one
                        parent->page_t->read_only[i] = 1;//change the read_only to 1
                        read_only_num++;
		}
	}
}
void copy_on_write(pcb* ptr, int index){
	write_num++;
        if(ptr->page_t->valid[index] == 0){
		page_miss++;
                ptr->page_t->pagetable[index] = fpfhead->head->frame_num;
		MAIN_MEM[fpfhead->head->frame_num] = give_write_num;//write
        	give_write_num++;
		count_page++;
                //give the free page
                if(fpfhead->head->next == NULL){
                        fpfhead->head = NULL;
                        fpfhead->tail = NULL;
                } else{
                        fpfhead->head = fpfhead->head->next;
                }
                        ptr->page_t->valid[index]=1;
        } else{//when it is mapped
		page_hit++;
		if(ptr->page_t->read_only[index]==1){
                        //read_only == 1
                        //shared memory
                        //write is occureed
                        printf("Copy on Write event!\n");
                        printf("Origin frame number\tPage data: %x , %d ->", ptr->page_t->pagetable[index],MAIN_MEM[ptr->page_t->pagetable[index]]);
                        ptr->page_t->pagetable[index] = fpfhead->head->frame_num;

                        if(fpfhead->head->next == NULL){
                                fpfhead->head = NULL;
                                fpfhead->tail = NULL;
                        } else{
                                fpfhead->head = fpfhead->head->next;
                        }

                        ptr->page_t->read_only[index] = 0;
                        MAIN_MEM[ptr->page_t->pagetable[index]] = give_write_num;//
                        printf("Changed frame number\tPage data: %x , %d\n", ptr->page_t->pagetable[index],MAIN_MEM[ptr->page_t->pagetable[index]]);
                        give_write_num++;
			copy_num++;
		} else{
                        printf("Write event\n");
                        printf("page index: %x",index);
                        printf("origin data: %d -> changed data: %d\n", MAIN_MEM[ptr->page_t->pagetable[index]], give_write_num);
                        MAIN_MEM[ptr->page_t->pagetable[index]] = give_write_num;//
                        give_write_num++;
                        //not shared part
                }
	}
}
