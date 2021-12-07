#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<string.h>

#define BIG_SIZE (1<<10)
#define CONSUMER_NUM 2
#define PRODUCER_NUM (1<<2) // It must be a factor of BIG_SIZE

int Big_Buffer[BIG_SIZE];
int Max_Buffer[PRODUCER_NUM];
int Min_Buffer[PRODUCER_NUM];

int maximum=0;
int minimum=2147483647;

// progress of max/min buffer
int max_count = 0;
int min_count = 0;

// semaphore to access max/min buffer
sem_t max_mutex;
sem_t min_mutex;
// semaphore to avoid consumer getting nothing in buffer
sem_t max_full;
sem_t min_full;


void *Producer(void* id)
{
    // define Producer's range in big buffer
    int value;
    value=*(int*)id;
    
    int lo=value*(BIG_SIZE/PRODUCER_NUM);
    int hi=lo+(BIG_SIZE/PRODUCER_NUM);
    int temp_max = 0, temp_min = 2147483647;
    // find temporary max/min
    for(int i=lo;i<hi;i++)
    {
        int temp=Big_Buffer[i];
        temp_max=temp_max>temp?temp_max:temp;
        temp_min=temp_min<temp?temp_min:temp;
    }
    printf("\033[92mProducer %d\033[0m: Temporary max=%d\tmin=%d\n", value, temp_max, temp_min);

    // puts in max bufer
    sem_wait(&max_mutex);
        Max_Buffer[max_count]=temp_max;
        printf("\033[92mProducer %d\033[0m: Put \033[93;1m%d\033[0;1m \tinto max-buffer at %d\n", value, temp_max, max_count++);
    sem_post(&max_mutex);
    sem_post(&max_full);

    // puts in min bufer
    sem_wait(&min_mutex);
        Min_Buffer[min_count]=temp_min;
        printf("\033[92mProducer %d\033[0m: Put \033[94;1m%d\033[0;1m \tinto min-buffer at %d\n", value, temp_min, min_count++);
    sem_post(&min_mutex);
    sem_post(&min_full);

    pthread_exit(NULL);
}

void *Consumer(void* task)
{
    if(!strcmp((char*)task,"max"))
    {
        // my task is maintain max
        for(int i=0;i<PRODUCER_NUM;i++)
        {
            sem_wait(&max_full);
            sem_wait(&max_mutex);
                maximum=Max_Buffer[i]>maximum?Max_Buffer[i]:maximum;
                printf("\033[95mConsumer\033[0m: Updated! \033[93;1mmaximum=%d\033[0;1m\n", maximum);
            sem_post(&max_mutex);
        }
    }
    else if(!strcmp((char*)task,"min"))
    {
        // my task is maintain min
        for(int i=0;i<PRODUCER_NUM;i++)
        {
            sem_wait(&min_full);
            sem_wait(&min_mutex);
                minimum=Min_Buffer[i]<minimum?Min_Buffer[i]:minimum;
                printf("\033[95mConsumer\033[0m: Updated! \033[94;1mminimum=%d\033[0;1m\n", minimum);
            sem_post(&min_mutex);
        }
    }
    pthread_exit(NULL);
}

int main()
{
    printf("\n");
    // create Big Buffer and put random numbers in
    srand(time(NULL));
    for(int i=0;i<BIG_SIZE;i++)
        Big_Buffer[i]=rand();

    // initialize semaphores
    sem_init(&max_mutex,    0,  1);
    sem_init(&min_mutex,    0,  1);

    sem_init(&max_full,     0,  0);
    sem_init(&min_full,     0,  0);

    // create threads
    pthread_t myProducer[PRODUCER_NUM];
    pthread_t myConsumer[CONSUMER_NUM];
    char task[CONSUMER_NUM][5]={"max", "min"};
    int  tasks[PRODUCER_NUM];

    for(int i=0;i<PRODUCER_NUM;i++)
        tasks[i]=i;

    for(int i=0;i<CONSUMER_NUM;i++)
        pthread_create(&(myConsumer[i]), NULL, Consumer, &task[i] );
    
    for(int i=0;i<PRODUCER_NUM;i++)
        pthread_create(&(myProducer[i]), NULL, Producer, &tasks[i]);
    
    // wait my consumers to finish their tasks
    for(int i=0;i<CONSUMER_NUM;i++)
        pthread_join(myConsumer[i], NULL);
    
    // print outcome
    printf("\nSuccess! maximum=%d and minimum=%d\n\n", maximum, minimum);
}