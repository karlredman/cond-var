//consumer.c  -creates shared memory, and displays the contents

#define POSIX_SOURCE 1
#include <pthread.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

// my include file
#include "procon.h"

void catchSignal(int sigNum, int useDefault);
void signal_handler(int signal);

int killit = 0;

void signal_handler(int signal)
{
  switch(signal)
    {
    case SIGINT:	//a regular interrupt signal
      killit = 1;
      break;
    case SIGCONT:	//must be caught for sigsuspend to work
      break;
    }
}


void catchSignal(int sigNum, int useDefault)
{
  //this stuff doesn't work, i haven't figured out why yet
  struct sigaction sa;

  if(useDefault)
    sa.sa_handler = SIG_DFL;
  else
    sa.sa_handler = signal_handler;

  if(sigaction(sigNum, &sa, NULL))
    {
      perror("sigaction");
      exit(1);
    }
}

int main(int argc, char argv[])
{
  
  /*## initialize stufff ##*/

  shared_use_st *shared_stuff;
  int clientDataPos = 0;
  int shmid;
  void *shared_memory = (void *)0;
  int producer_dataPos = 0;
  int thisClient = 0;
  int doSleep = 1;

  //various signal masks
  //more maskes than are truely necessary
  sigset_t zeromask, newmask, oldmask, othermask;

  //used to demonstrate blocking
  struct timespec timerx;

  //take care of command line
  //any second argument will do
  if(argc > 1)		//to sleep or not to sleep
    doSleep = 0;	//turn off demo-sleep mode
  
  //install signal handlers
  catchSignal( SIGINT, 0);
  catchSignal( SIGCONT, 0);

#ifdef __linux__
  sigemptyset(&zeromask);
  sigemptyset(&newmask);
  sigemptyset(&othermask);
  
  //make a SIGINT critical section signal set
  sigaddset(&newmask, SIGINT);

  //make a SIGCONT critical sec. sig. set (for sleeping)
  sigaddset(&othermask, SIGCONT);
#endif
  
  //get shared mem -don't connect if it's not there from producer
  shmid = shmget((key_t)1234, MEM_SZ, 0666);
  if(shmid == -1)
    {
      fprintf(stderr, "shmget failed, is producer running?\n");
      exit(EXIT_FAILURE);
    }
  
  //attach to shared memory
  shared_memory = shmat(shmid, (void *)0, 0);
  if(shared_memory == (void *)-1)
    {
      fprintf(stderr, "shmat failed\n");
      exit(EXIT_FAILURE);
    }

  //point to our structure
  shared_stuff = (shared_use_st *) ((char *)shared_memory +
				    (sizeof(struct shmid_ds) + 4));
  
  printf("Memory attached at %x\n", (int)shared_memory);
  
  //increment client and set pid for producer to see
  if(shared_stuff->clients+1 >= MAXCLIENTS)
    {
      printf("MAXCLIENTS exceeded\n");
      killit = 1;			//terminate
    }
  else
    {      
      shared_stuff->clients++;

      //get pid and put it into shared memory
      for(thisClient = 0; thisClient < MAXCLIENTS; thisClient++)
	{
	  //take the first available slot
	  if( shared_stuff->pids[thisClient] == 0 )
	    {
	      shared_stuff->pids[thisClient] = getpid(); 
	      printf("pid of client %d = %d\n",thisClient, shared_stuff->pids[thisClient]);
	      break;
	    }
	  printf("pid of client %d = %d\n",thisClient, shared_stuff->pids[thisClient]);
	} //end for

      /* ....and yes, it is possible for two consumers to find the
	 same slot at the same time here but it is extremely unlikely.
      */
    } //end if(...MAXCLIENTS)

  /*## do work ##*/
  while(!killit)
    {
	  
#ifndef __linux__
      //set up condition variable and mutex
      pthread_mutex_lock(shared_stuff->mutex);
      while(shared_stuff->dataPos == clientDataPos)
#else
	if(shared_stuff->dataPos == clientDataPos)
#endif
	      
	  {
#ifndef __linux__
	    //wait for producer to send us a signal 
	    pthread_cond_wait(shared_stuff->cv,
			      shared_stuff->mutex);
#else

	    /* emulate what a condition variable does (sort-of)
	     */

	    //block SIGINT -during the suspend
	    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	    //suspend on all signals until a signal is caught
	    sigsuspend(&zeromask);
	    printf("return from sigsuspend\n");

	    //replace blocking mask with the original
	    sigprocmask(SIG_SETMASK, &oldmask, NULL);
		
#endif
	  }
#ifndef __linux__
      //unset associated mutex
      pthread_mutex_unlock(shared_stuff->mutex);
#endif
	  
      //output data
      printf("data[CDataPos] = %d, CDataPos = %d, PDataPos = %d, thisClient = %d\n",
	     shared_stuff->data[clientDataPos], clientDataPos,
	     shared_stuff->dataPos, thisClient);

      //wrap the data position if neccessary
      if(clientDataPos >= MY_BUFSIZ)
	clientDataPos = 0;
      else
	clientDataPos++;
      

#ifdef __linux__ //I can't remember if nanosleep is portable from linux

	//slow down the producer for kicks and gigles

      //block SIGCONT -begin critical section
      sigprocmask(SIG_BLOCK, &othermask, &oldmask);	

	//value must be set every time for linux
	timerx.tv_sec = 0;
	timerx.tv_nsec = 90000000;
	
	//sleep
	nanosleep(&timerx,0);

      //further slow down the consumer 
      //demonstrates critical section and blocking
      if(!(clientDataPos % 10) && doSleep)	//every 10
	{
	    //suspend on all signals until a signal is caught
	    printf("sleeping\n");
	    sleep(1);
	}      

      //replace original blocked mask with the original
      sigprocmask(SIG_SETMASK, &oldmask, NULL);
#endif
  
    }//end while

  //cleanup
  printf("program terminating, cleaning up\n");

  if(shared_stuff->pids[thisClient] == getpid())
    {
      shared_stuff->pids[thisClient] = 0;	//remove client from list
      shared_stuff->clients--;			//decriment # of clients
    }

  //detach from shared memory
  printf("detaching from shared_memory\n");
  if(shmdt(shared_memory) == -1)
    {
      fprintf(stderr, "shmdt failed\n");
      exit(EXIT_FAILURE);
    }

  printf("exiting program\n");
      
  //normal exit
  exit(EXIT_SUCCESS);
      
}//end main
