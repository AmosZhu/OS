#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>



#define N 5     //Number of philospher
#define LEFT ((i+N-1)%N)
#define RIGHT ((i+1)%N)
#define SEM_MODE S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP

#define RESULT_CHECK(x) ({\
    if(x<0){\
        printf("reason %s\n",strerror(errno));\
        exit(-1);}})

typedef int semaphore;
typedef enum _status
{
    THINKING,
    EATING,
    HUNGRY,
} pStatus;


struct philospher
{
    pStatus status;
    semaphore sem;
};

union semun
{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
};


static struct philospher person[N];
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;


static semaphore sem_create();
static int sem_p(semaphore sem_id);
static int sem_v(semaphore sem_id);
static int sem_rm(semaphore sem_id);
static void take_fork(int i);
static void put_fork(int i);
static void test(int i);
static void eating(void)
{
    usleep(100);//eating 100ms
}

static void* thr_func1(void* arg)
{
    while(1)
    {
        take_fork(0);
        eating();
        put_fork(0);
        usleep(10);
    }
}

static void* thr_func2(void* arg)
{
    while(1)
    {
        take_fork(1);
        eating();
        put_fork(1);
        usleep(10);
    }
}

static void* thr_func3(void* arg)
{
    while(1)
    {
        take_fork(2);
        eating();
        put_fork(2);
        usleep(10);
    }
}

static void* thr_func4(void* arg)
{
    while(1)
    {
        take_fork(3);
        eating();
        put_fork(3);
        usleep(10);
    }
}

static void* thr_func5(void* arg)
{
    while(1)
    {
        take_fork(4);
        eating();
        put_fork(4);
        usleep(10);
    }
}

int main(void)
{
    int i=0;
    pthread_t tid1,tid2,tid3,tid4,tid5;
    sigset_t sig,osig;

    for(i=0; i<N; i++)
    {
        person[i].status=THINKING;
        person[i].sem=sem_create();
    }

    alarm(5);

    sigfillset(&sig);
    sigdelset(&sig,SIGALRM);
    sigprocmask(SIG_BLOCK,&sig,&osig);
    pthread_create(&tid1,NULL,thr_func1,NULL);
    pthread_create(&tid2,NULL,thr_func2,NULL);
    pthread_create(&tid3,NULL,thr_func3,NULL);
    pthread_create(&tid4,NULL,thr_func4,NULL);
    pthread_create(&tid5,NULL,thr_func5,NULL);

    sigsuspend(&sig);

    sigprocmask(SIG_SETMASK,&osig,NULL);

    exit(0);
}


void take_fork(int i)
{
    if(person[i].status==EATING)
        return;
    pthread_mutex_lock(&mutex);
    person[i].status=HUNGRY;
    printf("phiospher %d hungry\n",i);
    test(i);
    pthread_mutex_unlock(&mutex);
    sem_p(person[i].sem);
}

void put_fork(int i)
{
    pthread_mutex_lock(&mutex);
    person[i].status=THINKING;
    printf("philospher %d is thinking now\n",i);
    test(LEFT);
    test(RIGHT);
    pthread_mutex_unlock(&mutex);
}

void test(int i)
{
    if((person[i].status==HUNGRY)&&(person[LEFT].status!=EATING)&&(person[RIGHT].status!=EATING))
    {
        person[i].status=EATING;
        sem_v(person[i].sem);
        printf("philospher %d is eating\n",i);
    }
    else
    {
        printf("philospher %d cannot eating and wait\n",i);
    }
    return;
}

int sem_create()
{
    int nsem=1;
    int sem_id;
    union semun arg;
    if((sem_id=semget(IPC_PRIVATE,nsem,IPC_CREAT|SEM_MODE))<0)
    {
        printf("semget error,reason %s\n",strerror(errno));
        return -1;
    }

    arg.val=0;
    if(semctl(sem_id,0,SETVAL,arg)<0)
    {
        printf("semctl set val failed, reason %s\n",strerror(errno));
        return -1;
    }

    return sem_id;
}

int sem_p(int sem_id)
{
    struct sembuf buf;
    buf.sem_num=0;
    buf.sem_op=-1;
    buf.sem_flg&=~IPC_NOWAIT;
    buf.sem_flg|=SEM_UNDO;

    return semop(sem_id,&buf,1);
}

int sem_v(int sem_id)
{
    struct sembuf buf;
    buf.sem_num=0;
    buf.sem_op=1;
    buf.sem_flg&=~IPC_NOWAIT;
    buf.sem_flg|=SEM_UNDO;

    return semop(sem_id,&buf,1);
}

int sem_rm(int sem_id)
{
    return semctl(sem_id,0,IPC_RMID);
}

