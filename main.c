#include "timer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define MY_BUFFER_SIZE 2048
time_t myBuffer[MY_BUFFER_SIZE];
int count = 0;
int seeker = 0;

#define CALL_Mutex(f) mutex(#f, f)
#define CALL_Unlock(f) mutexunlock(#f, f)
#define READYQ_SIZE 200
#define ISR_SIZE 200

int flag = 0;
int readyQ_rear = - 1;
int readyQ_front = - 1;
int ISR_rear = - 1;
int ISR_front = - 1;
int semaphore = 1;
int sem_index=0;
int wait_list_sem[20];//we save the priority of the task here
int checkWL_count=0;
int tick=0;

struct TASK_TYPE{
    void (*task)();
    int priority; // 0 to 15
    int stacksize;
};

struct TASK_TYPE ready_queue[READYQ_SIZE];
struct TASK_TYPE ISR_queue[ISR_SIZE];

void Quetask(struct TASK_TYPE);
void Quetask_ISR(struct TASK_TYPE);
void createTask(void (*task)(), int, int, bool);
void NMI();
void task1();
void task2();
void task3();
void task4();
void task5();
void task6();
void task7();
void task8();
void task9();
void task10();
void task11();
void task12();
void task13();
void task14();
void idletask();
void Dispatch();
void Dispatch_ISR();
int isEmpty(int);

void TICK_Handler() {
    time_t timer = time(NULL);
    myBuffer[count++ % MY_BUFFER_SIZE] = timer;
    tick++;
    if (isEmpty(ISR_front))
      Dispatch();
    else
      Dispatch_ISR();
}

void deleteSem(){
  printf("Semaphore is deleted \n");
}

void createSem(){
  printf("Semaphore is created \n");
}

int wait_list_empty(){
  for(int i=0;i<sizeof(wait_list_sem);i++)
    if(wait_list_sem[i] > 2)
      return 0;
  return 1;
}


int sem_handler(int priority){
  if(semaphore){//if it is available& wait list is empty
    semaphore = 0;
    return 1;
  }else{
    wait_list_sem[sem_index]= priority;
    sem_index++;
    return 0;
  }
}

int highestP_Search(int p){
  int index;
  for(int i=0;i<sizeof(wait_list_sem);i++){
    if( (wait_list_sem[i]<p) && (wait_list_sem[i] > 2) )
    {
      p = wait_list_sem[i];
      index = i;
    }
  }
  return index;
}


void wait_list_handler(int priority){
  int indexHP;
  if(sem_index!=0){
      indexHP = highestP_Search(priority);
      sem_handler(wait_list_sem[indexHP]);
      printf("task %d is being executed \n", wait_list_sem[indexHP]+1 );
      wait_list_sem[indexHP]= 0;
  }
}

int isFull(int front, int rear)
{
    if( (front == rear + 1) || (front == 0 && rear == READYQ_SIZE-1)) return 1;
    return 0;
}

int isEmpty(int front)
{
    if(front == -1) return 1;
    return 0;
}

void createTask(void (*task)(), int priority, int stack_size, bool queue)
{
    struct TASK_TYPE newtask;
    newtask.task = task;
    newtask.priority = priority;
    newtask.stacksize = stack_size;
    if (queue == 1)
      Quetask(newtask);
      else
      Quetask_ISR(newtask);
}

void Quetask(struct TASK_TYPE taskname){
    if (isEmpty(readyQ_front)){
        readyQ_front = 0;
        readyQ_rear = (readyQ_rear + 1) % READYQ_SIZE;
        ready_queue[readyQ_rear] = taskname;
        //printf("enqueued in ready queue 1 \n");
    }
    else if(isFull(readyQ_front, readyQ_rear)){
        printf("queue is full \n");
    }
    else{
        if ((readyQ_front == 0) && (readyQ_rear == 0)){
            if (taskname.priority>=ready_queue[readyQ_rear].priority){
                readyQ_rear = (readyQ_rear + 1) % READYQ_SIZE;
                ready_queue[readyQ_rear] = taskname;
                //printf("enqueued in ready queue 2 \n");
            }
            else{
                ready_queue[(readyQ_rear + 1) % READYQ_SIZE] = ready_queue[readyQ_rear];
                ready_queue[readyQ_rear] = taskname;
                readyQ_rear = (readyQ_rear + 1) % READYQ_SIZE;
                //printf("enqueued in ready queue 2(2) \n");
            }
        }
        else{
            int i = readyQ_rear;
            while (i != readyQ_front){
                if (taskname.priority>=ready_queue[i].priority){
                    ready_queue[(i+1) % READYQ_SIZE] = taskname;
                    //printf("enqueued in ready queue 3 \n");
                    readyQ_rear = (readyQ_rear + 1) % READYQ_SIZE;
                    break;
                }
                else{
                    ready_queue[(i+1) % READYQ_SIZE] = ready_queue[i];
                    //printf("enqueued in ready queue 3(2) \n");
                }
                i = (i-1) % READYQ_SIZE;
            }
            if (i == readyQ_front){
                if (taskname.priority>=ready_queue[i].priority){
                    ready_queue[(i+1) % READYQ_SIZE] = taskname;
                    //printf("enqueued in ready queue 4 \n");
                    readyQ_rear = (readyQ_rear + 1) % READYQ_SIZE;
                }
                else{
                    ready_queue[(i+1) % READYQ_SIZE] = ready_queue[i];
                    ready_queue[i] = taskname;
                    readyQ_rear = (readyQ_rear + 1) % READYQ_SIZE;
                    //printf("enqueued in ready queue 4(2) \n");
                }
            }
        }
    }
}

void Quetask_ISR(struct TASK_TYPE taskname){
    if (isEmpty(ISR_front)){
        ISR_front = 0;
        ISR_rear = (ISR_rear + 1) % ISR_SIZE;
        ISR_queue[ISR_rear] = taskname;
        //printf("enqueued in ISR queue 1 \n");
    }
    else if(isFull(ISR_front, ISR_rear)){
        printf("queue is full \n");
    }
    else{
        if ((ISR_front == 0) && (ISR_rear == 0)){
            if (taskname.priority>=ISR_queue[ISR_rear].priority){
                ISR_rear = (ISR_rear + 1) % ISR_SIZE;
                ISR_queue[ISR_rear] = taskname;
                //printf("enqueued in ISR queue 2 \n");
            }
            else{
                ISR_queue[(ISR_rear + 1) % ISR_SIZE] = ISR_queue[ISR_rear];
                ISR_queue[ISR_rear] = taskname;
                ISR_rear = (ISR_rear + 1) % ISR_SIZE;
                //printf("enqueued in ISR queue 2(2) \n");
            }
        }
        else{
            int i = ISR_rear;
            while (i != ISR_front){
                if (taskname.priority>=ISR_queue[i].priority){
                    ISR_queue[(i+1) % ISR_SIZE] = taskname;
                    //printf("enqueued in ISR queue 3 \n");
                    ISR_rear = (ISR_rear + 1) % ISR_SIZE;
                    break;
                }
                else{
                    ISR_queue[(i+1) % ISR_SIZE] = ISR_queue[i];
                    //printf("enqueued in ISR queue 3(2) \n");
                }
                i = (i-1) % ISR_SIZE;
            }
            if (i == ISR_front){
                if (taskname.priority>=ISR_queue[i].priority){
                    ISR_queue[(i+1) % ISR_SIZE] = taskname;
                    //printf("enqueued in ISR queue 4 \n");
                    ISR_rear = (ISR_rear + 1) % ISR_SIZE;
                }
                else{
                    ISR_queue[(i+1) % ISR_SIZE] = ISR_queue[i];
                    ISR_queue[i] = taskname;
                    ISR_rear = (ISR_rear + 1) % ISR_SIZE;
                    //printf("enqueued in ISR queue 4(2) \n");
                }
            }
        }
    }
}

void mutex(const char *name, void (*f)(void)) {
    flag = 1;
    printf("This task is locked %s()\n", name);
    // f();
}
void mutexunlock(const char *name, void (*f)(void)) {
    flag = 0;
    printf("This task is unlocked %s()\n", name);
    // f();
}

void idletask(){
    printf("idle \n");
}
void NMI(){
    printf("This is the highest priority task, NMI \n");
}

void task1(){
        if (flag ==0)
    {
        CALL_Mutex(task1);
    }
    FILE *fptr;
    printf("task1 is being executed \n");
    fptr = fopen("saveread.txt","w");
    char c[512] = "Terry didn't consider himself particularly unusual. Sure, he spent his teenage years as a willing and sometimes absurdly cheerful social outcast, upon adulthood immediately transitioned to playing side-kick to a magic-savvy private investigator, accidentally became the confidant of an apparently ageless time-traveler, and just recently declared war on a corporation widely recognized as one of the top ten charitable organizations in the world, but he figured most people had a few weird phases in their lives.";
    for (int i = 0; i<512; i++)
    {
      fprintf(fptr, "%c", c[i]);
    }
    fclose(fptr);
    if ( flag == 1){
      CALL_Unlock(task1);
    }
}

void task2(){
      if(sem_handler(1)){
    printf("task2 is being executed \n");
    FILE *fptr;
    fptr = fopen("printer.txt","w");
    char c[44] = "This one acquires the semaphore 2nd (task 2)";
    for (int i = 0; i<44; i++)
    {
      fprintf(fptr, "%c", c[i]);
    }
    fclose(fptr);
    semaphore = 1;
  }else{
    printf("task2 is waiting for semaphore \n");
    wait_list_handler(1);
    semaphore = 1;
  }
}

void task3(){
      if(sem_handler(2)){
    printf("task3 is being executed \n");
    FILE *fptr;
    fptr = fopen("printer.txt","w");
    char c[44] = "This one acquires the semaphore 3rd (task 3)";
    for (int i = 0; i<44; i++)
    {
      fprintf(fptr, "%c", c[i]);
    }
    fclose(fptr);
    semaphore = 1;
  }else{
    printf("task3 is waiting for semaphore \n");
    wait_list_handler(2);
    semaphore = 1;
  }
}

void task4(){
      if(sem_handler(3)){
    printf("task4 is being executed \n");
    FILE *fptr;
    fptr = fopen("printer.txt","w");
    char c[44] = "This one acquires the semaphore 1st (task 4)";
    for (int i = 0; i<44; i++)
    {
      fprintf(fptr, "%c", c[i]);
    }
    fclose(fptr);
    semaphore = 1;
  }else{
    printf("task4 is waiting for semaphore \n");
    wait_list_handler(3);
    semaphore = 1;
  }
}

void task5(){
  if(sem_handler(4)){
    printf("task5 is being executed \n");
    task7();
    semaphore = 1;
  }else{
    printf("task5 is waiting for semaphore \n");
    wait_list_handler(4);
    semaphore = 1;
  }
}
void task6(){
  if(sem_handler(5)){
    printf("task6 is being executed \n");
    semaphore = 1;
  }else{
    printf("task6 is waiting for semaphore \n");
    wait_list_handler(5);
    semaphore = 1;
  }
}

void task7(){
  if(sem_handler(6)){
    printf("task7 is being executed \n");
    task4();
    semaphore = 1;
  }else{
    printf("task7 is waiting for semaphore \n");
    wait_list_handler(6);
    semaphore = 1;
  }
}

void task8(){
  if(sem_handler(7)){
    printf("task8 is being executed \n");
    task9();
    semaphore = 1;
  }else{
    printf("task8 is waiting for semaphore \n");
    wait_list_handler(7);
    semaphore = 1;
  }
}
void task9(){
  if(sem_handler(8)){
    printf("task9 is being executed \n");
    semaphore = 1;
  }else{
    printf("task9 is waiting for semaphore \n");
    wait_list_handler(8);
    semaphore = 1;
  }
}

void task10(){
  if(sem_handler(9)){
    printf("task10 is being executed \n");
    semaphore = 1;
  }else{
    printf("task10 is waiting for semaphore \n");
    wait_list_handler(9);
    semaphore = 1;
  }
}

void task11(){
  if(sem_handler(10)){
    printf("task11 is being executed \n");
    task10();
    semaphore = 1;
  }else{
    printf("task11 is waiting for semaphore \n");
    wait_list_handler(10);
    semaphore = 1;
  }
}
void task12(){
   if (flag == 0)
    {
        CALL_Mutex(task12);
    }
    printf("task12 is being executed \n");
   if ( flag == 1){
        CALL_Unlock(task12);
    }
}

void task13(){
   if (flag ==0)
    {
        CALL_Mutex(task13);
    }
    printf("task13 is being executed \n");
   if ( flag == 1){
        CALL_Unlock(task13);
    }

}

void task14(){//saving vs reading mutex
    if (flag ==0)
    {
        CALL_Mutex(task14);
    }
    FILE *fptr;
    printf("task14 is being executed \n");
    fptr = fopen("saveread.txt","r");
    char c[255];
    while(fscanf(fptr, "%s", c)!=EOF){printf("%s \n", c);}
    fclose(fptr);
    if ( flag == 1){
      CALL_Unlock(task14);
    }
}


void Dispatch(){
    if(isEmpty(readyQ_front)) {
        printf("Queue is empty \n");
        return;
    }
    else {
        (ready_queue[readyQ_front].task)();
        if (readyQ_front == readyQ_rear){
            readyQ_front = -1;
            readyQ_rear = -1;
        }
        else {
            readyQ_front = (readyQ_front + 1) % READYQ_SIZE;
        }
    }

}
void Dispatch_ISR(){
    if(isEmpty(ISR_front)) {
        printf("Queue is empty \n");
        return;
    }
    else {
        (ISR_queue[ISR_front].task)();
        if (ISR_front == ISR_rear){
            ISR_front = -1;
            ISR_rear = -1;
        }
        else {
            ISR_front = (ISR_front + 1) % ISR_SIZE;
        }
    }

}


void check_waitlist(){
  for(int i=3;i<11;i++)
   wait_list_handler(i);
}

int main(void) {
    fakeuc_enableTimerInterrupts();
    createSem();
  for(;;){
            if (seeker < count) {
            struct tm* tm_info = localtime(&myBuffer[seeker++ % MY_BUFFER_SIZE]);
            char buffer[16];
            strftime(buffer, 16, "[%H:%M:%S]", tm_info);
            //printf("Called @ %s\n", buffer);
      if (tick == 1)
      {
        createTask(task4, 4, 20,1);
        createTask(task14, 14, 20,1);
        }
        if (tick == 2)
      {
        createTask(task1, 1, 20,0);
        createTask(task2,2,20,0);
        createTask(task3,3,20,0);
      }
    if (isEmpty(readyQ_front))
        createTask(idletask, 15, 0,1);
    check_waitlist();
  }
  }

  return 0;
}
