#include "sched.h"
#include "myQ.h"
#include "msg.h"

int count = 0;

typedef struct pcb{
	pid_t pid;
	int io_burst;
	int cpu_burst;
}pcb_t;

typedef struct user{
	int pid;
	int io_burst;
	int cpu_burst;
}user_t;

user_t user_proc[MAX_PROC];
pcb_t pcb[MAX_PROC];
msgbuf msg;
Queue running,waiting;

int order = 0;
int next=1;
int msgq;
int kernel=0;
int io_request=0;
int cpu_set[MAX_PROC];
int io_set[MAX_PROC];

int key = 32191818;

FILE *fp;

int main()
{
	fp = fopen("FIFO.txt","w");

	init(&running);
	init(&waiting);

	msgq = msgget( key, IPC_CREAT | 0666);
	fprintf(fp,"msgq : %d\n", msgq);
	
	srand(time(NULL));

	for(int i=0;i<MAX_PROC;i++){
		io_set[i] = (rand()%20) + 1;
		cpu_set[i] = (rand()%20) + 1;
	}

	struct sigaction old_sa;
	struct sigaction new_sa;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = &signal_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	for(int i=0;i<MAX_PROC;i++){
		pid_t ppid;
		ppid = fork();

		if(ppid == -1){
			perror("fork error");
			exit(0);
		}
		else if(ppid==0){ //for child
			init(&running);
			init(&waiting);

			user_proc[i].cpu_burst = cpu_set[i];
			user_proc[i].io_burst = io_set[i];
			user_proc[i].pid = getpid();

			fprintf(fp,"proc[%d]\tio_burst : %d\tcpu_burst: %d \n",i,user_proc[i].io_burst,user_proc[i].cpu_burst);

			new_sa.sa_handler = &do_child;
			sigaction(SIGUSR1, &new_sa, &old_sa);

			while(1);
			//exit(0);
		}
		else{ 
			kernel=getpid();
			pcb[i].pid = ppid;
			pcb[i].io_burst = io_set[i];
			pcb[i].cpu_burst = cpu_set[i];

			enqueue(&running, i);
			int check = counting(&running);
		}
	}
	struct itimerval new_itimer, old_itimer;
	new_itimer.it_interval.tv_sec = 0;
	new_itimer.it_interval.tv_usec = 8000;
	new_itimer.it_value.tv_sec = 0;
	new_itimer.it_value.tv_usec = 8000;


	setitimer(ITIMER_REAL, &new_itimer, &old_itimer);

	while(1){
		int ret = msgrcv(msgq, &msg, sizeof(msgbuf), 0, IPC_NOWAIT);
		if(ret==-1){
			// perror("msgrcv error : ");
		}
		else{
			pcb[msg.order].io_burst=msg.io_time;
			enqueue(&waiting, msg.order);
		}
	}	

	fclose(fp);
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

	if(user_proc[set_order].cpu_burst==0){
		user_proc[set_order].cpu_burst=cpu_set[set_order];
		msg.mtype = 1;
		msg.order = set_order;
		msg.pid = getpid();
		msg.io_time = user_proc[set_order].io_burst;

		int key = 32161620;
		msgq = msgget( key, IPC_CREAT | 0666);
		int ret = msgsnd(msgq, &msg, sizeof(msgbuf), 0);

		if(ret==-1){
			perror("msgsnd error ");
		}
	}
}
void running_fifo(){
	check_running();
	check_waiting();

	return ;
}

void signal_handler(int signo){
	count++;
	running_fifo();

	if(count==10000){
		for(int i =0; i<MAX_PROC ; i++){
			kill(pcb[i].pid, SIGINT);
		}
		exit(0);
	}

	return ;
}

void check_waiting(){
	int num_waiting = counting(&waiting);
	int wait_arr[num_waiting];

	fprintf(fp,"waiting queue[%d] : ",num_waiting);

	int success = sorting(&waiting, wait_arr);
	for(int k=0; k<num_waiting; k++){
		fprintf(fp,"%d ",wait_arr[k]);
	}
	fprintf(fp,"\n");
	if(success == 0){
		fprintf(fp,"waiting time[%d] : ",num_waiting);
		for(int k=0; k<num_waiting; k++){
			int i = wait_arr[k];
			fprintf(fp,"%d ",pcb[i].io_burst);
			if(pcb[i].io_burst==0){
				pcb[i].io_burst=io_set[i];
				search(&waiting, i);
				enqueue(&running, i);
			}
			else{
				pcb[i].io_burst--;
			}
		}
	}
	fprintf(fp,"\n");
	return ;
}

void check_running(){
	fprintf(fp,"\n------------tick %d-----------\n",count);

	if(next==1){
		order = dequeue(&running);
		if(order==-1){
			//printf("empty");
			exit(0);
		}
		next=0;
	}

	int run_idx = counting(&running);
	int run_arr[run_idx];


	fprintf(fp,"ready queue[%d] : ",run_idx);
	sorting(&running, run_arr);

	for(int k=0; k<run_idx; k++){
		fprintf(fp,"%d ",run_arr[k]);
	}
	fprintf(fp,"\n");

	fprintf(fp,"running queue[1] : %d\n", order);
	fprintf(fp,"running_pid[%d]\tremain_cpu_burst : %d\tcpu_time : %d \n",pcb[run_idx].pid,pcb[run_idx].cpu_burst,cpu_set[run_idx]);

	pcb[order].cpu_burst--;

	kill(pcb[order].pid,SIGUSR1);
	if(pcb[order].cpu_burst==0){
		next=1;
		pcb[order].cpu_burst = cpu_set[order];
		return ;
	}
	return ;
}
