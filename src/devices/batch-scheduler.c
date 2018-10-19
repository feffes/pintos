/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "lib/random.h" //generate random numbers

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
	void getSlot(task_t task); /* task tries to use slot on the bus */
	void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
	void leaveSlot(task_t task); /* task release the slot */

struct semaphore bus_sema, task_s_p, task_r_p;
struct lock mutex;

int busDirection;


/* initializes semaphores */ 
void init_bus(void){ 
 
	random_init((unsigned int)123456789); 
  sema_init(&bus_sema, 3);
  sema_init(&task_s_p, 0);
  sema_init(&task_r_p, 0);
	lock_init(&mutex);
	busDirection=0;
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
	/*printf("Spawning tasks..\n");*/
	while(num_tasks_send > 0){
		thread_create("send", 0, senderTask, NULL);
		num_tasks_send--;
	}
	while(num_task_receive > 0){
		thread_create("send", 0, receiverTask, NULL);  
		num_task_receive--;
	}
	while(num_priority_send > 0){
		thread_create("send", 0, senderPriorityTask, NULL);  
		num_priority_send--;
	}
	while(num_priority_receive > 0){
		thread_create("send", 0, receiverPriorityTask, NULL);
		num_priority_receive--;
	}
	/*printf("Done spawning tasks\n");*/
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
	if(task.priority == NORMAL){
			
	}else{
		if(task.direction == SENDER){
			sema_up(&task_s_p);
		}else{
			sema_up(&task_r_p);
		}
	}

	//make direction semaphores more general
	struct semaphore *sameDir;
	struct semaphore *opposingDir;
  if(task.direction == SENDER){
		sameDir = &task_s_p;
		opposingDir = &task_r_p;
	}else{
		sameDir = &task_r_p;
		opposingDir = &task_s_p;
	}
	while(true){
		lock_acquire(&mutex);
 		if((bus_sema.value == 3 || busDirection == task.direction)
				&& bus_sema.value > 0 
				&& (task.priority == HIGH || ((*sameDir).value == 0 && (*opposingDir).value == 0))){
			busDirection = task.direction;
			if(task.priority == HIGH){
				if(task.direction == SENDER){
					sema_down(&task_s_p);
				}else{
					sema_down(&task_r_p);
				}
			}
			sema_down(&bus_sema);
			/*printf("GOT slot\n");*/
			lock_release(&mutex);
			break;
		}
		lock_release(&mutex);
	}
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
	/*printf("NUM OF PRIO SEND %d\n", task_s_p.value);
	printf("NUM OF PRIO RECIEVE %d\n", task_r_p.value);
	if(task.direction == SENDER){
		if(task.priority == HIGH){
			printf("SENDING DATA WITH HIGH PRIO\n");
		}else{
			printf("SENDING DATA WITH LOW PRIO\n");
		}
	}else{
		if(task.priority == HIGH){
			printf("RECIEVING DATA WITH HIGH PRIO\n");
		}else{
			printf("RECIEVING DATA WITH LOW PRIO\n");
		}
	}*/
	timer_sleep(random_ulong()%30);
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
	sema_up(&bus_sema);
}
