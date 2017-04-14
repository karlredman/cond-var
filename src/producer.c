//producer.c  

/* 
   TITLE: producer 

   PURPOSE: demonstrate POSIX condition variable emulation between
   processes, a shared condition variable, for use in Linux. Linux
   does not allow shared condition variables.

   NOTE: see procon.h for structure and size info

   AUTHOR: Karl N. Redman (A.K.A. parasyte)

   last updated: 1-3-2000
*/

//define posix stuff
#define POSIX_SOURCE 1
#include <pthread.h>

//other headers used
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>		/* only for demonstration */

// include file for this project
#include "procon.h"

#ifndef __linux__
/*
  if we're not using linux, use normal POSIX shared condition
  variables
*/
pthread_mutex_t		clientMutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t		mutex		= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		cv		= PTHREAD_COND_INITIALIZER;
#endif

//a global flag for terminating the process
int killit = 0;

/* 
   Signal Handler: handles an interupt signal and a terminate signal 
 */ 
void signal_handler(int signal)
{
  switch(signal)
    {
    case SIGINT:
      printf("got SIGINT\n");
      killit = 1;
      break;
    case SIGTERM:
      printf("got SIGTERM\n");
      killit = 1;
      break;
    }
}

/*
  Signal Catcher: specify which signals to catch and swap handlers as
  necessary

  NOTE: This doesn't work in Linux as expected, it's not actually used
  here. See below for more detailes. (This is only here to show that
  this stuff can get kinda tricky sometimes)
*/
void catchSignal(int sigNum, int useDefault)
{
  //a common signal action structure
  struct sigaction sa;

  if(useDefault)
    sa.sa_handler = SIG_DFL;
  else
    sa.sa_handler = signal_handler;

  //install the signal handler
  if(sigaction(sigNum, &sa, NULL))
    {
      //OOPS! something bad happened
      perror("sigaction");
      exit(1);
    }
}

/******************************************************
			    MAIN
******************************************************/
//takes no arguments
int main()
{

  /*## define and declare stuff ##*/
  void *shared_memory = (void *)0;/* shared mem pointer */
  shared_use_st *shared_stuff;	/* my shared mem layout */
  char buffer[MY_BUFSIZ];	/* a buffer */
  int shmid;			/* the shared mem ID */
  int j = 0, i = 0;		/* some variables */

  /* size of shared memory (5 structures) */
  int memsize = sizeof(shared_use_st) *5;

  struct timespec timerx;	/* a timer for demonstration */
  sigset_t newmask;		/* a signal mask */


  /*## INITIALIZE SHARED MEMORY ##*/

  //get the shared memory address (use an uncommon key)
  shmid = shmget((key_t)1234, memsize, 0666 |IPC_CREAT);
  if(shmid == -1)
    {
      //FAILED!!
      fprintf(stderr, "shmget failed\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      //print the shared memory ID we are using
      printf("shmid = %d\n", shmid);
    }

  //INSTALL SIGNAL HANDLER
  /* THESE DON'T WORK FOR THIS APP. for Linux. It would normaly work
     -in linux it doesn't.
     catchSignal( SIGINT, 0);
     catchSignal( SIGTERM, 0);
  */

  /* INSTALL signal handler for interupt and terminate signals. This
     allows us to interupt (ctrl-s,q) the process and use terminate
     signals (ctrl-c)
  */
  //MUST CATCH BOTH SIGNALS  
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);


  /*ATTACH to shared memory */
  shared_memory = shmat(shmid, (void *)0, 0);
  if(shared_memory == (void *)-1)
    {
      fprintf(stderr, "shmat failed\n");
      exit(EXIT_FAILURE);
    }
  else
    printf("shared_memory = %d\n", shared_memory);
      
  //print where we attached to
  printf("Memory attached at %x\n", (int)shared_memory);

  /* set our pointer to the shared memory region we want.
     Note: the use of +4 here is required here and is multiplatform
     compatible. The +4 is an overhead thing.
  */
  shared_stuff = (shared_use_st *) ((char *)shared_memory +
				    (sizeof(struct shmid_ds) + 4));

#ifndef __linux__
  //not linux, set shared condition variable and mutex
  shared_stuff->mutex = &mutex;
  shared_stuff->clientMutex = &clientMutex;
  shared_stuff->cv = &cv;
#endif

  //setup ring buffer 
  memset(shared_stuff->data, 0, sizeof(shared_stuff->data[0]));

  /* init client counter: keeps track of the number of consumer
     processes that will be reading the shared memory created by this
     producer program
  */
  shared_stuff->clients = 0;
  
  /* init termination: an indicator to consumer programs that they
     should terminate or detach from shared memory (the producer is no
     longer in service)
  */
  shared_stuff->terminate=0;

  /* set positioner to zero: used as the element number (bin) where
     the producer will next place data into. Alos, consumer programs
     will use this to make sure they don't pass the producer up.
  */
  shared_stuff->dataPos = 0;

  /* set pids to 0: tracks consumer process IDs. Used to notify
     consumer processes of data uptates
  */
  memset(shared_stuff->pids, 0, sizeof(shared_stuff->pids));


  /*## DO WORK ##*/


  /* this variable is used to represent data. it could be anything,
     even a structure
  */
  j = 0; 


  /* start a loop: while we haven't been signaled to terminate....
   */
  while(!killit && !shared_stuff->terminate)
    {
      /* print the bin were writing to and the data respecively
       */
      printf("datapos = %d, j = %d\n",shared_stuff->dataPos, j);

      
      //change the data in the ring buffer using the j variable
      shared_stuff->data[shared_stuff->dataPos]=j;

	if(shared_stuff->dataPos >= MY_BUFSIZ)
	   shared_stuff->dataPos=0;
        else
	   shared_stuff->dataPos++;

	if(j >= MAXDATA_NUM)
	  {
	   j=0;
	  }
	else
	   j++;

#ifndef __linux__
      //broadcast to all threads -unblock cond. var.

      /* this will tell all consumer processes to stop waiting if
	 they're stoped at a condition variable
      */
      pthread_cond_broadcast(shared_stuff->cv);
#else
      /* LINUX USAGE */

      //BEHAVE LIKE A CONDITION VARIABLE
      /* send a SIGCONT signal to all consumers (whether they are
	 blocked/suspended or not)
      */


	//BEHAVE LIKE A CONDITION VARIABLE
	/* send a SIGCONT signal to all consumers (whether they are
	   blocked/suspended or not)
	*/
	if(shared_stuff->clients)
	  {
	    printf("sending signals\n");
	    for(i = 0; i < MAXCLIENTS; i++)
	      {
		if(shared_stuff->pids[i] != 0) //dont send to pid 0 !!!
		  kill(shared_stuff->pids[i], SIGCONT);
	      }
	  }
#endif
       
	
	//slow down the producer for kicks and gigles
#ifdef __linux__ //I can't remember if nanosleep is portable for linux

	//value must be set every time for linux
	timerx.tv_sec = 0;
	timerx.tv_nsec = 200000000;
	
	//sleep
	nanosleep(&timerx,0);
#endif

	//further slow down the producer
	//-causes the consumers to block after
	//catching up with the producer
	if(!(j % 20))	//every 20
	   {
	     printf("sleeping\n");
	     sleep(1);
	   }

    }//end while


  /* OK, now shutdown and cleanup (a terminate signal was received
     probably) 
  */
 
  //kill consumers (they'll handle their own sigint)
  printf("signal recieved, killing consumers and cleaning up\n");

  /* 
     loop through the list of consumers and send an interrupt signal
     to each one
  */
  for(i = 0; i < MAXCLIENTS; i++)
    {
      if(shared_stuff->pids[i] != 0)
	{
	  kill(shared_stuff->pids[i], SIGINT);
	}
    }



  //detach from shared memory and delete it
  printf("detaching from shared_memory\n");
  if(shmdt(shared_memory) == -1)
    {
      fprintf(stderr, "shmdt failed\n");
      exit(EXIT_FAILURE);
    }
  
  printf("deleting shared memory\n");
  if(shmctl(shmid, IPC_RMID, NULL) == -1)
    {
      fprintf(stderr, "shmctl IPC_RMID failed\n");
      exit(EXIT_FAILURE);
    }

  printf("ending program\n");
  
  exit(EXIT_SUCCESS);
}
