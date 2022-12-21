#include "sched.h"
#include "myQ.h"

#define TRUE 1
#define FALSE 0

typedef struct pcb{
	pid_t pid;
	int exec_time;
}pcb_t;

typedef struct user_object{
	int cpu_burst;
	int pid;
}user_t;

typedef struct sort{
	int set;
	int time;
}sort_t;

user_t user_proc[MAX_PROC];
pcb_t pcb[MAX_PROC];

Queue running;

int count = 0;
int order = 0;
int next=1;
int set[MAX_PROC];
int waiting_time[MAX_PROC];


void signal_handler(int signo);
void running_fifo();
void do_child(int signo);
void swap(sort_t* i, sort_t* b);
void sort(sort_t* array);

FILE *fp;

int main()
{
	fp = fopen("SJF.txt","w");

	init(&running);

    srand((unsigned)time(NULL));
	for(int k=0; k<MAX_PROC; k++)
	{
		set[k] = (rand() % 10) + 1; // get the set randomly
	}

	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	//	srand((unsigned)time(NULL));
    srand(time(NULL));
	sort_t sort_arr[MAX_PROC];


	for(int i=0;i<MAX_PROC;i++){
		pid_t ppid;
		ppid = fork();

		if (ppid == -1) {
			perror("fork error");
			exit(0);
		}
		else if(ppid==0){ 
			user_proc[i].cpu_burst=set[i];
			user_proc[i].pid = getpid();
			fprintf(fp,"\ncpu_burst[%d]\tset[%d]\n", user_proc[i].cpu_burst,set[i]);
			fprintf(fp,"child proc %d \n\n", getpid());

			new_sa.sa_handler = &do_child;
			sigaction(SIGUSR1, &new_sa, &old_sa);
			while(1);
		}
		else{ 
			pcb[i].pid = ppid;
			pcb[i].exec_time = set[i];
			sort_arr[i].set=i;
			sort_arr[i].time = set[i];
		}
	}


	sort(sort_arr);
	for(int i=0;i<MAX_PROC;i++){
		enqueue(&running, sort_arr[i].set);
	}

	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = 1;
	new_itimer.it_value.tv_sec = 0;
	new_itimer.it_value.tv_usec = 1;
	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	while (1);
	fclose(fp);
	return 0;
}

void swap(sort_t* a, sort_t* b){
	sort_t tmp = *a;
	*a = *b;
	*b = tmp;
}

void sort(sort_t* array){
	int i,j;
	int isSwaped;

	for(i=0; i<MAX_PROC; i++){
		isSwaped=FALSE;
		for(j=0; j<MAX_PROC-1; j++){
			if(array[j].time > array[j+1].time){
				swap(&array[j],&array[j+1]);
				isSwaped=TRUE;
			}
		}
		if(isSwaped==FALSE) {
			break;
		}
	}
}

void do_child(int signo){
	int set_order=-1;

	for(int i=0; i<MAX_PROC; i++){
		if(user_proc[i].pid == getpid()){
			set_order=i;
			break;
		}
	}

	user_proc[set_order].cpu_burst--;
	fprintf(fp,"proc[%d] remain_cpu : %d \n",set_order,user_proc[set_order].cpu_burst);

	if(user_proc[set_order].cpu_burst==0){
		exit(0);
	}
}

void signal_handler(int signo){
	count++;
	running_fifo();
	return ;
}


void running_fifo(){
	if(next==1){

		order = dequeue(&running);
		if(order==-1){
			float total=0;
			fprintf(fp,"\n-----------waiting time------------\n");
			for(int i=0;i<MAX_PROC;i++){
				fprintf(fp,"prod[%d] : %d \n",i,waiting_time[i]);
				total = waiting_time[i] + total;

			}
			fprintf(fp,"\n");

			fprintf(fp,"\nAverage Waiting Time : %f \n",total/MAX_PROC);

			exit(0);
		}
		next=0;
	}
	fprintf(fp,"\n------------tick[%d]-----------\n",count);
	int num_running = counting(&running);
	int run_arr[num_running];

	fprintf(fp,"ready queue[%d] : ",num_running);
	sorting(&running, run_arr);
	for(int k=0; k<num_running; k++){
		fprintf(fp,"%d ",run_arr[k]);
	}
	fprintf(fp,"\n");


	fprintf(fp,"running queue[1] : %d \n", order);

	pcb[order].exec_time--;

	kill(pcb[order].pid,SIGUSR1);	

	if(pcb[order].exec_time==0){
		next=1;
		waiting_time[order] = count-set[order];
		return ;
	}

	return ;
}
