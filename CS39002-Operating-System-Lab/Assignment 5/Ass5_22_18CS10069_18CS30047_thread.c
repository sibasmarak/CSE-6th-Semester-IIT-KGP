#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<semaphore.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<time.h>
#include<signal.h>
#include<errno.h>
#include<pthread.h>

#define MAX_QUEUE_SIZE 8
#define MAX_PRIORITY 10
#define MAX_JOB_ID 100000

// structure to represent job
typedef struct job{
	pthread_t prod_tid;
	int prod_no;
	int prod_priority;
	int comp_time;
	int job_id;
}job;


void* produce(void*);   // produce the job
void* consume(void*);   // consume the job
job create_job(pthread_t ,int ,unsigned int* );	
void print_job(job j);	

void swap(job* j1,job* j2);
int parent(int i);
int left(int i);
int right(int i);
void heapify(int i);
void insert_job(job j);
job remove_job();

// structure shared amongst all the threads
// priority queue to store the jobs
typedef struct job_queue{
    job priority_queue[MAX_QUEUE_SIZE];
    int size;
	int job_created;
	int job_completed;
	int max_jobs;
    int computed;
	sem_t mutex;
	sem_t full;
	sem_t empty;
}job_queue;

job_queue* queue;
job_queue* init_queue(int max_jobs);

int main(){

	// input
	int NP,NC,max_jobs,p,c,status;
	printf("Enter no of Producers:\n");
	scanf("%d",&NP);
	printf("Enter no of Consumers:\n");
	scanf("%d",&NC);
	printf("Enter max no of jobs:\n");
	scanf("%d",&max_jobs);
	int prod_no[NP];	// to hold producer numbers
	int cons_no[NC];	// to hold consumer numbers
	
	//array to hold producer tids
	pthread_t producers[NP];
	//array to hold consumer tids
	pthread_t consumers[NC];

	time_t start = time(0);//start time

    pthread_attr_t attr;
    pthread_attr_init(&attr); // get default attributes
    
    // initialize the job_queue
    queue = init_queue(max_jobs);

    // create the producer threads
    for(p = 0; p < NP; p++){
    	prod_no[p] = p+1;
        status = pthread_create(&producers[p], &attr, produce, (void*)&(prod_no[p]));
        if(status != 0)
        {
            printf("Error in creating producer thread!");
            exit(EXIT_FAILURE);
        }
    }

    // create the consumer threads
    for(c = 0; c < NC; c++){
    	cons_no[c] = c+1;
        status = pthread_create(&consumers[c], &attr, consume, (void*)&(cons_no[c]));
        if(status != 0)
        {
            printf("Error in creating consumer thread!");
            exit(EXIT_FAILURE);
        }
    }  

    // loop till all jobs are created and consumed
	while(1){
		// acquire lock so that while checking, state change not possible
		sem_wait(&(queue->mutex));
		// queue->computed ensures that consumer gets killed only after it has computed or slept
		if(queue->job_created >= max_jobs && queue->job_completed >= max_jobs && queue->computed >= max_jobs)
		{
			time_t end = time(0);
			int time_taken = end-start;
			printf("Time taken to run %d jobs= %d seconds\n",max_jobs,time_taken);
			// terminate all threads
			for(int i=0;i<NP;i++)
				pthread_cancel(producers[i]);
			for(int i=0;i<NC;i++)
				pthread_cancel(consumers[i]);
			sem_post(&(queue->mutex));
			break;		
		}
		sem_post(&(queue->mutex));
	}
	// destroy mutex semaphore
	sem_destroy(&(queue->mutex));
	free((void*)queue);
	return 0; 
}

// initializes the data structure members of queue
job_queue* init_queue(int max_jobs)
{
	job_queue* queue = (job_queue*)malloc(sizeof(job_queue));
	queue->size = 0;
	queue->max_jobs = max_jobs;
	queue->job_created = 0;
	queue->job_completed = 0;
    queue->computed = 0;

	// initialize the semaphore mutex	
	//binary semaphore for access to jobs_created, jobs_completed, insertion & retrieval of jobs
	int sema = sem_init(&(queue->mutex),0,1);
	//counting semaphore to check if the priority_queue is full
	int full_sema = sem_init(&(queue->full),0,0);
	//counting semaphore to check if the priority_queue is empty
	int empty_sema= sem_init(&(queue->empty),0,MAX_QUEUE_SIZE);
	if(sema<0||full_sema<0||empty_sema<0){
		printf("Error in initializing semaphore. Exiting..\n");
		exit(1);
	}
	return queue;
}

// runner for producer thread
void* produce(void* p)
{
	//unique seed for each thread
	unsigned int seed = time(0)+*((int*)p);
	while(1)
	{
		// if all jobs created then exit
		if(queue->job_created==queue->max_jobs)
			break;
		// create job
		job j = create_job(pthread_self(), *((int*)p), &seed);
		// random delay
		sleep(rand_r(&seed)%4);
		// wait for empty semaphore
		sem_wait(&(queue->empty));
		// wait for mutex
        sem_wait(&(queue->mutex));
        // if all jobs created then exit
        if(queue->job_created==queue->max_jobs)
        {
        	sem_post(&(queue->mutex));
			break;
        }
		if(queue->size < MAX_QUEUE_SIZE)
		{
			insert_job(j);
			printf("Produced job details:\n");
			print_job(j);		
			// increment job_created variable
			queue->job_created++;
			// signal semaphore full
			sem_post(&(queue->full));						
		}
		// signal mutex
		sem_post(&(queue->mutex));
	}
}

// runner for consumer process
void* consume(void* p)
{
	//unique seed for each thread
	unsigned int seed = time(0)+*((int*)p);
	while(1)
	{
		// random delay
		sleep(rand_r(&seed)%4);
		// wait for semaphore full
		sem_wait(&(queue->full));
		// wait to acquire mutex
		sem_wait(&(queue->mutex));
		job j;
		// flag to indicate a job is retrieved
		int job_retrieved=0;
		if(queue->size>0)
		{
			// remove highest priority job from priority queue
			j = remove_job();
			job_retrieved=1;
		}
		// signal mutex
		sem_post(&(queue->mutex));
		if(job_retrieved)
		{
			printf("Consumed job details\n");
			printf("consumer: %d,",*((int*)p));
			printf("consumer tid: %ld,",pthread_self());
			print_job(j);
			// wait for mutex
			sem_wait(&(queue->mutex));
			// increment shared variables
			queue->job_completed++;
			// signal mutex
			sem_post(&(queue->mutex));
			// signal empty semaphore
			sem_post(&(queue->empty));
			// delay caused by job compute time
			sleep(j.comp_time);

			// to ensure a consumer is killed only after it has slept or computed job
            sem_wait(&(queue->mutex));
            queue->computed++;
			sem_post(&(queue->mutex));
		};
	}
}

// create a job taking producer thread ID and number and a seed as input
// and using random integers for priority,comp_time & job_id
job create_job(pthread_t prod_tid,int prod_no,unsigned int* seed)
{
	job j;
	j.prod_tid      = prod_tid;
	j.prod_no       = prod_no;
	j.prod_priority = rand_r(seed)%MAX_PRIORITY+1;
	j.comp_time     = rand_r(seed)%4+1;
	j.job_id        = rand_r(seed)%MAX_JOB_ID+1;
	return j;
}

// swap two jobs in priority queue
void swap(job* j1,job* j2)
{
	job temp=*j1;
	*j1=*j2;
	*j2=temp;
}
// parent index
int parent(int i)
{
	return (i-1)/2;
}
// left child
int left(int i)
{
	return 2*i+1;
}
// right child
int right(int i)
{
	return 2*i+2;
}
// heapify subtree with root at given index
void heapify(int i)
{
	int l=left(i);
	int r=right(i);
	int max=i;
	if(l<queue->size && queue->priority_queue[l].prod_priority>queue->priority_queue[i].prod_priority)
		max=l;
	if(r<queue->size && queue->priority_queue[r].prod_priority>queue->priority_queue[max].prod_priority)
		max=r;
	if(max!=i)
	{
		swap(&(queue->priority_queue[i]),&(queue->priority_queue[max]));
		heapify(max);
	}
}
// insert a new job in the priority queue
void insert_job(job j)
{
	if(queue->size==MAX_QUEUE_SIZE)
	{
		printf("Overflow: Cannot insert\n");
		return;
	}
	// first insert new job at the end
	queue->size++;
	int i=queue->size-1;
	queue->priority_queue[i]=j;
	// fix the max heap property if violated
	while(i!=0 && queue->priority_queue[parent(i)].prod_priority<queue->priority_queue[i].prod_priority)
	{
		swap(&(queue->priority_queue[i]),&(queue->priority_queue[parent(i)]));
		i=parent(i);
	}
}
// extract the maximum priority job from priority queue
job remove_job()
{
	if(queue->size<=0)
	{
		job j;
		j.prod_priority=-1;
		return j;
	}
	if(queue->size == 1)
	{
		queue->size--;
		return queue->priority_queue[0];
	}
	// store the max-priority job and remove from queue
	job root = queue->priority_queue[0];
	queue->priority_queue[0] = queue->priority_queue[queue->size-1];
	queue->size--;
	heapify(0);
	return root;
}
// print the details of a job
void print_job(job j)
{
	printf("Job ID: %d,",j.job_id);
	printf("producer: %d,",j.prod_no);
	printf("producer tid: %ld,",j.prod_tid);
	printf("priority: %d,",j.prod_priority);
	printf("compute time: %d\n",j.comp_time);
}
