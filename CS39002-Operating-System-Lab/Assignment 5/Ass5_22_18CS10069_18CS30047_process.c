#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<semaphore.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<time.h>
#include<signal.h>
#include<errno.h>

#define MAX_QUEUE_SIZE 8
#define MAX_PRIORITY 10
#define MAX_JOB_ID 100000

// structure to represent job
typedef struct job{
	pid_t prod_pid;
	int prod_no;
	int prod_priority;
	int comp_time;
	int job_id;
}job;

//structure to represent shared memory
typedef struct shared_memory{
	job priority_queue[MAX_QUEUE_SIZE];
	int size;
	int job_created;
	int job_completed;
	int max_jobs;
	int computed;
	sem_t mutex;
	sem_t full;
	sem_t empty;
}shared_memory;

shared_memory* init_shm(int shmid,int max_jobs);
void producer(int shmid,int prod_no,pid_t prod_pid);
void consumer(int shmid,int cons_no,pid_t cons_pid);
job create_job(pid_t prod_pid,int prod_no);
void print_job(job j);

void swap(job* j1,job* j2);
int parent(int i);
int left(int i);
int right(int i);
void heapify(shared_memory* shm,int i);
void insert_job(shared_memory* shm,job j);
job remove_job(shared_memory* shm);

int main(){

	// starting point for generating random numbers for parent process
	srand(time(0));
	// input
	int NP,NC,max_jobs;
	printf("Enter no of Producers:\n");
	scanf("%d",&NP);
	printf("Enter no of Consumers:\n");
	scanf("%d",&NC);
	printf("Enter max no of jobs:\n");
	scanf("%d",&max_jobs);

	//array to hold producer pids
	pid_t prods[NP];
	//array to hold consumer pids
	pid_t conss[NC];

	// generate a unique key using ftok
	key_t key = ftok("/dev/random",'c');
	// create shared memory
	int shmid = shmget(key,sizeof(shared_memory),0666|IPC_CREAT);
	if(shmid<0){
		printf("Errno=%d\n",errno);
		printf("Error in creating shared memory. Exitting..\n");
		exit(1);
	}
	// attach and initialize the shared memory
	shared_memory* shm = init_shm(shmid,max_jobs);

	time_t start = time(0);//start time

	pid_t pid;
	// create all producers
	for(int i=1;i<=NP;i++)
	{
		pid=fork();
		if(pid<0){
			printf("Error in creating producer process. Exitting..\n");
			exit(1);
		}
		else if(pid==0)// in producer process
		{
			// different seed for each producer process
			srand(time(0)+i);
			int prod_pid=getpid();
			producer(shmid,i,prod_pid);
			// terminate child process
			// so that only original process can fork child processes
			return 0;
		}
		else
		{
			//include the producer in prods array to kill later
			prods[i-1]=pid;
		}
	}
	// create all consumers
	for(int i=1;i<=NC;i++)
	{
		pid=fork();
		if(pid<0){
			printf("Error in creating consumer process. Exitting..\n");
			exit(1);
		}
		else if(pid==0)// in consumer process
		{
			// different seed for each consumer process
			srand(time(0)+NP+i);
			int cons_pid=getpid();
			consumer(shmid,i,cons_pid);
			// terminate child process
			// so that only original process can fork child processes
			return 0;
		}
		else
		{
			//include the consumer in consss array to kill later
			conss[i-1]=pid;
		}
	}

	// loop till all jobs are created and consumed
	while(1){
		// acquire lock so that while checking, state change not possible
		sem_wait(&(shm->mutex));
		// shm->computed ensures that consumer gets killed only after it has computed/slept
		if(shm->job_created>=max_jobs && shm->job_completed>=max_jobs && shm->computed>=max_jobs)
		{
			time_t end = time(0);
			int time_taken = end-start;
			printf("Time taken to run %d jobs= %d seconds\n",max_jobs,time_taken);
			// kill all child processes
			for(int i=0;i<NP;i++)
				kill(prods[i],SIGTERM);
			for(int i=0;i<NC;i++)
				kill(conss[i],SIGTERM);
			sem_post(&(shm->mutex));
			break;		
		}
		sem_post(&(shm->mutex));
	}
	//destroy mutex semaphore
	sem_destroy(&(shm->mutex));
	//sem_destroy(&(shm->full));
	//sem_destroy(&(shm->empty));
	shmdt(shm);//detach shared memory segment
	shmctl(shmid,IPC_RMID,0);//mark shared memory segment to be destroyed
	return 0;
}

// attaches shared memory to a memory segment
// and initializes the data structure members
shared_memory* init_shm(int shmid,int max_jobs)
{
	shared_memory* shm = (shared_memory*)shmat(shmid,NULL,0);
	shm->size = 0;
	shm->max_jobs = max_jobs;
	shm->job_created = 0;
	shm->job_completed = 0;
	shm->computed = 0;

	// initialize the semaphore mutex
	//binary semaphore for access to jobs_created, jobs_completed, insertion & retrieval of jobs
	int sema = sem_init(&(shm->mutex),1,1);
	//counting semaphore to check if the priority_queue is full
	int full_sema = sem_init(&(shm->full),1,0);
	//counting semaphore to check if the priority_queue is empty
	int empty_sema= sem_init(&(shm->empty),1,MAX_QUEUE_SIZE);
	if(sema<0||full_sema<0||empty_sema<0){
		printf("Error in initializing semaphore. Exitting..\n");
		exit(1);
	}
	return shm;
}
// producer function
void producer(int shmid,int prod_no,pid_t prod_pid)
{
	shared_memory* shmp=(shared_memory*)shmat(shmid,NULL,0);
	while(1)
	{
		// if all jobs created, exit
		if(shmp->job_created==shmp->max_jobs)
			break;
		// create job
		job j = create_job(prod_pid,prod_no);
		// random delay
		sleep(rand()%4);
		// wait for empty semaphore
		sem_wait(&(shmp->empty));
		// wait for mutex
		sem_wait(&(shmp->mutex));
		// if all jobs created, exit
		if(shmp->job_created==shmp->max_jobs)
		{
			sem_post(&(shmp->mutex));
			break;
		}
		if(shmp->size < MAX_QUEUE_SIZE)
		{
			insert_job(shmp,j);
			printf("Produced job details:\n");
			print_job(j);
			// increment shared variable
			shmp->job_created++;
			// signal the full semaphore		
			sem_post(&(shmp->full));						
		}
		// signal mutex
		sem_post(&(shmp->mutex));
	}
	// detach this process from shared memory
	shmdt(shmp);
}
// consumer function
void consumer(int shmid,int cons_no,pid_t cons_pid)
{
	shared_memory* shmc=(shared_memory*)shmat(shmid,NULL,0);
	while(1)
	{
		// random delay
		sleep(rand()%4);
		// wait for the full semaphore
		sem_wait(&(shmc->full));
		// wait to acquire mutex
		sem_wait(&(shmc->mutex));
		job j;
		// flag to indicate a job is retrieved
		int job_retrieved=0;
		if(shmc->size>0)
		{
			// remove highest priority job from priority queue
			j = remove_job(shmc);
			job_retrieved=1;
		}
		// signal mutex
		sem_post(&(shmc->mutex));
		if(job_retrieved)
		{
			printf("Consumed job details\n");
			printf("consumer: %d,",cons_no);
			printf("consumer pid: %d,",cons_pid);
			print_job(j);
			// wait for mutex
			sem_wait(&(shmc->mutex));
			// increment shared variable
			shmc->job_completed++;
			// signal mutex
			sem_post(&(shmc->mutex));
			// signal empty semaphore
			sem_post(&(shmc->empty));
			// delay caused by job compute time
			sleep(j.comp_time);
			// to ensure consumer is killed only after it has slept/computed job
			sem_wait(&(shmc->mutex));
			shmc->computed++;
			sem_post(&(shmc->mutex));
		};
	}
	// detach from shared memory
	shmdt(shmc);
}
// create a job taking producer process ID and number as input
// and using random integers for priority,comp_time & job_id
job create_job(pid_t prod_pid,int prod_no)
{
	job j;
	j.prod_pid      = prod_pid;
	j.prod_no       = prod_no;
	j.prod_priority = rand()%MAX_PRIORITY+1;
	j.comp_time     = rand()%4+1;
	j.job_id        = rand()%MAX_JOB_ID+1;
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
void heapify(shared_memory* shm,int i)
{
	int l=left(i);
	int r=right(i);
	int max=i;
	if(l<shm->size && shm->priority_queue[l].prod_priority>shm->priority_queue[i].prod_priority)
		max=l;
	if(r<shm->size && shm->priority_queue[r].prod_priority>shm->priority_queue[max].prod_priority)
		max=r;
	if(max!=i)
	{
		swap(&(shm->priority_queue[i]),&(shm->priority_queue[max]));
		heapify(shm,max);
	}
}
// insert a new job in the priority queue
void insert_job(shared_memory* shm,job j)
{
	if(shm->size==MAX_QUEUE_SIZE)
	{
		printf("Overflow: Cannoot insert\n");
		return;
	}
	// first insert new job at the end
	shm->size++;
	int i=shm->size-1;
	shm->priority_queue[i]=j;
	// fix the max heap property if violated
	while(i!=0 && shm->priority_queue[parent(i)].prod_priority<shm->priority_queue[i].prod_priority)
	{
		swap(&(shm->priority_queue[i]),&(shm->priority_queue[parent(i)]));
		i=parent(i);
	}
}
// extract the maximum priority job from priority queue
job remove_job(shared_memory* shm)
{
	if(shm->size<=0)
	{
		job j;
		j.prod_priority=-1;
		return j;
	}
	if(shm->size==1)
	{
		shm->size--;
		return shm->priority_queue[0];
	}
	// store the max-priority job and remove from queue
	job root = shm->priority_queue[0];
	shm->priority_queue[0] = shm->priority_queue[shm->size-1];
	shm->size--;
	heapify(shm,0);
	return root;
}
// print the details of a job
void print_job(job j)
{
	printf("Job ID: %d,",j.job_id);
	printf("producer: %d,",j.prod_no);
	printf("producer pid: %d,",j.prod_pid);
	printf("priority: %d,",j.prod_priority);
	printf("compute time: %d\n",j.comp_time);
}