typedef struct Node {
	int data;
	struct Node *next;
}Node;


typedef struct queue {
	Node *front;
	Node *rear; 
	int count;
}Queue;

int fnum; 

void init(Queue *queue);
void enqueue(Queue *queue, int data); 
int dequeue(Queue *queue); 
int isEmpty(Queue *queue);
int sorting(Queue *queue, int *ptr);
int search(Queue *queue, int data);
int counting (Queue *queue);

void init(Queue *queue){
	queue->front = queue->rear = NULL; 
	queue->count = 0;
}
void enqueue(Queue *queue, int data)
{
	Node *now = (Node *)malloc(sizeof(Node)); 
	now->data = data;
	now->next = NULL;

	if (isEmpty(queue)){
		queue->front = now;
	} else {
		queue->rear->next = now;
	}
	queue->rear = now;
	queue->count++;
}

int dequeue(Queue *queue)
{
	int ret = -1;
	Node *now;
	if (isEmpty(queue)){
		return -1;
	}
	now = queue->front;
	ret = now->data;
	queue->front = now->next;
	queue->count--;
	return ret;
}

int sorting(Queue *queue, int *ptr){
	int i= 0;
	if (queue->count==0){return -1;}
	Node *now = queue ->front;
	while (now){
		*(ptr+i)=now -> data;
		now = now->next;
		i++;
	}
	return 0;
}

int isEmpty(Queue *queue)
{
	return queue->count == 0;   
}


int counting (Queue *queue){
	return queue->count;
}

int search(Queue *queue, int data){
	int re = -1;
	Node* now;
	fnum=0;
	Node *search = queue->front;
	Node *link = queue->front;
	
	int exist=0;
	if (isEmpty(queue))
	{
		return re;
	}
	for(int a=0; a < queue->count; a++){
		fnum++;
		if(search->data == data){
			now = search;
			exist=1;
			break;
		}
		link = search;
		search = search->next;
	}
	if(exist==0){
		return -1;
	}
	else if(exist==1){
		if(fnum==1){
			queue->front = queue->front->next;
			search->next=NULL;
			queue->count--;
			return search->data;

		}
		else if(fnum==queue->count){
			queue->rear = link;
			queue->rear->next = NULL;
			queue->count--;
			return search->data;	
		}
		link->next = search->next;
		search->next=NULL;
		queue->count--;
		return search->data;
	}
	return 0;
}