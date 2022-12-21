// 32191818 
// 2. Multi_producer : Multi_consumer : Buffer = 1 : 1 : 1

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE 256
int stat[ASCII_SIZE];
void char_stat(char* argv);
void printChar();
int endFile = 0;
FILE *rfile;

typedef struct sharedobject {
	int linenum;
	char *line;
	pthread_mutex_t lock;
	pthread_cond_t cv; 
	int full, in, count;
} so_t;

void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
        read = getdelim(&line, &len, '\n', rfile);
        pthread_mutex_lock(&so[so->in].lock);
        while(so->full==1){
            pthread_cond_wait(&so->cv, &so->lock);
        }
		if (read == -1) {
            so->line = NULL;
            so->full = 1;
            so->count = 1;
            endFile = 1;
            pthread_cond_signal(&so->cv);
            pthread_mutex_unlock(&so->lock);
			break;
		}
        so->linenum = i;
        so->line = strdup(line); /* share the line */
		so->full = 1;
        i++;
        pthread_cond_signal(&so->cv);
		pthread_mutex_unlock(&so->lock);
	}
	free(line);
	line = NULL;
    printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;

	while (1) {
        pthread_mutex_lock(&so->lock);
        while(so->full == 0){
            pthread_cond_wait(&so->cv, &so->lock);
        }
        line = so->line;
        if(so->line == NULL){
            so->full = 0;
            pthread_cond_signal(&so->cv);
            pthread_mutex_unlock(&so->lock);
            break;
        }
		
        char_stat(line);
        len = strlen(line);
        free(so->line);
        so->line = NULL;
		line = NULL;
        i++;
        so->full = 0;
        pthread_cond_signal(&so->cv);
		pthread_mutex_unlock(&so->lock);
	}
    printf("Cons: %d lines\n", i);
	*ret = i;
	pthread_exit(ret);
}

void char_stat(char *argv){
    size_t length = 0;
    char *cptr = NULL;
    char *substr = NULL;
    char *brka = NULL;
    char *sep = "{}()[],;\" \n\t^";
    cptr = strdup(argv);
    for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
		length = strlen(substr);

        for (int i = 0; i < length; i++){
            if(*substr > 1){
                stat[*substr]++;
            }
            substr++;
        }
    }
    free(cptr);
    return;
}

int main (int argc, char *argv[])
{
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int *ret;
	int i;
	if (argc == 1) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}
	so_t *share[100];
    for(i = 0; i < 100; i++){
        share[i] = malloc(sizeof(so_t));
        memset(share[i],0,sizeof(so_t));
    }
    memset(stat,0,sizeof(stat)); // stat 초기화
	rfile = fopen((char *) argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
	if (argv[2] != NULL) {
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100;
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) {
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;

    for(i = 0; i < Ncons; i++){
        share[i]->line = NULL;
        pthread_mutex_init(&share[i]->lock, NULL);
        pthread_cond_init(&share[i]->cv, NULL);
    }

	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share[i]);
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share[i]);
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
	}

    printChar();
	pthread_exit(NULL);
	exit(0);
}

void printChar(){
    printf("A : %d\n",stat['A']+stat['a']);
    printf("B : %d\n",stat['B']+stat['b']);
    printf("C : %d\n",stat['C']+stat['c']);
    printf("D : %d\n",stat['D']+stat['d']);
    printf("E : %d\n",stat['E']+stat['e']);
    printf("F : %d\n",stat['F']+stat['f']);
    printf("G : %d\n",stat['G']+stat['g']);
    printf("H : %d\n",stat['H']+stat['h']);
    printf("I : %d\n",stat['I']+stat['i']);
    printf("J : %d\n",stat['J']+stat['j']);
    printf("K : %d\n",stat['K']+stat['k']);
    printf("L : %d\n",stat['L']+stat['l']);
    printf("M : %d\n",stat['M']+stat['m']);
    printf("N : %d\n",stat['N']+stat['n']);
    printf("O : %d\n",stat['O']+stat['o']);
    printf("P : %d\n",stat['P']+stat['p']);
    printf("Q : %d\n",stat['Q']+stat['q']);
    printf("R : %d\n",stat['R']+stat['r']);
    printf("S : %d\n",stat['S']+stat['s']);
    printf("T : %d\n",stat['T']+stat['t']);
    printf("U : %d\n",stat['U']+stat['u']);
    printf("V : %d\n",stat['V']+stat['v']);
    printf("W : %d\n",stat['W']+stat['w']);
    printf("X : %d\n",stat['X']+stat['x']);
    printf("Y : %d\n",stat['Y']+stat['y']);
    printf("Z : %d\n",stat['Z']+stat['z']);
}

