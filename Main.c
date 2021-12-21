#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<readline/readline.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/time.h>
#include<semaphore.h>
#include"Queue.h"

#define n_clerk 5
#define n_queue 2

int queue_size[n_queue];   
int queue_status[n_queue];  
int size_biz = 0;  
int size_econ = 0;  
int size_cst_served = 0;    
int time_trans = 100000;
int n_customer; 
double t_wait_econ, t_wait_biz;
pthread_cond_t queue_cond[2];
pthread_cond_t clerk_cond[5];
pthread_mutex_t time_mutex;
pthread_mutex_t queue_mutex_size; 
pthread_mutex_t serv_mutex;
// pthread_mutex_t queue_mutex; 
// pthread_mutex_t queue_mutex;
pthread_mutex_t queue_mutex[2];
pthread_mutex_t clerk_mutex[5];
static struct timeval init_time;
struct Customer* queue_biz = NULL;
struct Customer* queue_econ = NULL;

double timing();
void *clerk_entry(void* clerk_id);
void *customer_entry(void* cus_info);

int main(int argc, char **argv){
    
    size_biz = 0;
    size_econ = 0;
    queue_size[0] = 0;
    queue_size[1] = 0;
    queue_status[0] = 0;
    queue_status[1] = 0;
    time_trans = 100000;
    char buffer[256];
    int i, id, type;
    long int t_arrival, t_service;
    struct Customer *temp;
    double wait_sum,wait_econ,wait_biz;
    //receive and extract info from the input file
    if(argc !=2){
        printf("Invalid input.\n");
        exit(EXIT_FAILURE);
    }
    FILE *handle = fopen(argv[1], "r");
    if (handle == NULL)
    {
        printf("Invalid input.\n");
        exit(EXIT_FAILURE);
    }

    fgets(buffer, 256, handle); 
    n_customer = atoi(buffer);
    for (int i = 0 ; i < n_customer ; i++){
        fgets(buffer, 256, handle);
        sscanf(	buffer, "%d:%d,%ld,%ld", &id, &type, &t_arrival, &t_service);
        temp = (struct Customer*) malloc(sizeof(struct Customer));
        temp->id = id;  
        temp->type = type;  
        temp->t_arrival = t_arrival;  
        temp->t_service = t_service;

        //create queue for each customer type
        if (type == 0){ //0 for economy
            enqueue_econ(temp);
            size_econ++;
        }else if(type == 1){//1 for business
            enquque_biz(temp);
            size_biz++;
        }
    }
    fclose(handle);

    pthread_t thr_clerk[n_clerk];
    pthread_t thr_biz[n_customer];
    pthread_t thr_econ[n_customer];  
    pthread_mutex_init(&time_mutex,NULL);
    pthread_mutex_init(&queue_mutex[0], NULL);
    pthread_mutex_init(&queue_mutex[1], NULL);
    pthread_mutex_init(&queue_mutex_size, NULL);
    pthread_mutex_init(&serv_mutex, NULL);
    for (int i = 0; i < n_clerk; i++)
    {
        pthread_mutex_init(&clerk_mutex[i], NULL);
    }

    struct Customer *temp_econ = queue_econ;
    struct Customer *temp_biz = queue_biz;

    gettimeofday(&init_time, NULL);

    //create the threads for customers
    for (int i = 0; i < size_biz; i++){
        pthread_create(&thr_biz[i], NULL, customer_entry, (void *)temp_biz);
        temp_biz = temp_biz->next;
    }
    usleep(20); // wouldn't hurt our simulation
    for (int i = 0; i < size_econ; i++){
        pthread_create(&thr_econ[i], NULL, customer_entry,(void *)temp_econ);
        temp_econ = temp_econ->next;
    }
    
    //create the threads for clerks
    for (int i = 0; i < n_clerk; i++){
        int *clerk_temp = malloc(sizeof(*clerk_temp));
        *clerk_temp = i + 1;
        printf("the clerk ID is %d\n", i + 1);
        pthread_create(&thr_clerk[i-1], NULL, clerk_entry, (void*) (clerk_temp));
    }

    //return thread
    for (int i = 0; i < size_econ; i++){
        if (pthread_join(thr_econ[i], NULL))
        {
            fprintf(stderr, "Thread join error.\n");
        }
        
    }
    for(int i=0; i < size_biz; i++){
        if (pthread_join(thr_biz[i],NULL))
        {
            fprintf(stderr, "Thread join error.\n");
        }
        
    }

    //collect the stat information
    if(t_wait_biz == 0){
        wait_biz = 0;
    }else{
        wait_biz = t_wait_biz / size_biz;
    }
    if(t_wait_econ == 0){
        wait_econ = 0;
    }else{
        wait_econ = t_wait_econ / size_econ;
    }
    if((t_wait_biz == 0) && (t_wait_econ == 0)){
        wait_sum = 0;
    }else{
        wait_sum = (t_wait_econ+t_wait_biz) / n_customer;
    }
  
    printf("The average waiting time for all customers in the system is: %.2f seconds. \n",wait_sum);
    printf("The average waiting time for all business-class customers is: %.2f seconds. \n",wait_biz);
    printf("The average waiting time for all economy-class customers is: %.2f seconds. \n",wait_econ);
    exit(EXIT_SUCCESS);
    return 0;
}

double timing(){
    struct timeval cur_time;
    double cur_secs, init_secs;
    pthread_mutex_lock(&time_mutex);
    init_secs = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
    pthread_mutex_unlock(&time_mutex);
    gettimeofday(&cur_time, NULL);
    cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);
    return cur_secs - init_secs;
}

void * clerk_entry(void* clerk_id){
    int flag = -1; //flag for what the clerk faces to. 0: econ, 1: biz, -1:none
    int id = *(int *) clerk_id;
    
    while(1){
        //break the loop if every customer finishes service
        pthread_mutex_lock(&serv_mutex);
        if(size_cst_served >= n_customer){ 
            pthread_mutex_unlock(&serv_mutex);
            break;
        }
        pthread_mutex_unlock(&serv_mutex);

        //queue length lock for protecting the queue length from arbitrarily changing
        pthread_mutex_lock(&queue_mutex_size);
        if(queue_size[1] != 0){  
            size_cst_served++;
            queue_size[1]--;
            flag = 1;   
        }else if(queue_size[0] != 0){ 
            size_cst_served++;
            queue_size[0]--;
            flag = 0;
        }
        pthread_mutex_unlock(&queue_mutex_size);
                
        if (flag == -1){continue;}
        printf("Clerk %d is sending signal.\n", id);

        //clerk signals the queue, takes the lock and wait until the customer calls
        pthread_mutex_lock(&queue_mutex[flag]);      
        queue_status[flag] = id;
        pthread_cond_signal(&queue_cond[flag]);       
        pthread_mutex_unlock(&queue_mutex[flag]);
        pthread_mutex_lock(&clerk_mutex[id - 1]);
        pthread_cond_wait(&clerk_cond[id - 1],&clerk_mutex[id - 1]);
        pthread_mutex_unlock(&clerk_mutex[id - 1]);

     

     
    }
    pthread_exit(NULL);
    return NULL;
}

void *customer_entry(void* cus_info){
    struct Customer *myInfo =  (struct Customer *) cus_info;
    usleep(myInfo->t_arrival * time_trans); // sleep as long as it's arrival time

    int clerk;
    double start, end;
    fprintf(stdout,"A customer arrives: customer ID %2d. \n", myInfo->id);

        //lock to prevent the queue length from arbitrarily changing
        pthread_mutex_lock(&queue_mutex_size);
        queue_size[myInfo->type]++;
        fprintf(stdout,"A customer enters a queue: the queue ID %d, and length of the queue %2d. \n",myInfo->type, queue_size[0]);
        pthread_mutex_unlock(&queue_mutex_size);
        start = timing();
        pthread_mutex_lock(&queue_mutex[myInfo->type]);
        
        //wait here to be called by clerk
        pthread_cond_wait(&queue_cond[myInfo->type], &queue_mutex[myInfo->type]);
        clerk = queue_status[myInfo->type];
        queue_status[myInfo->type] = 0;
        pthread_mutex_unlock(&queue_mutex[myInfo->type]);
        
        //timing and report
        end = timing();
        t_wait_econ += (end - start);
        start = timing();
        fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %d.\n",
                                                                start ,myInfo->id, clerk);
        
        //lock the corresponding's clerk mutex
        pthread_mutex_lock(&clerk_mutex[clerk - 1]);

        //sleep as long time as the service time
        usleep(myInfo->t_service * time_trans);
        end = timing();
        fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %d.\n",
                                                                 end, myInfo->id, clerk);
        //call to the clerk that ends serving
        pthread_cond_signal(&clerk_cond[clerk - 1]);
        pthread_mutex_unlock(&clerk_mutex[clerk - 1]);
    
    pthread_exit(NULL);
    return NULL;
}