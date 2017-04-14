Linux Shared Condition Variable Emulation across processes:

Summary:
In 1999 Linux was unable to handle SHARED condition variables. The problem bugged me so much at the time that I nearly went nuts trying to find a workaround for sharing a condition variable across process at the time. If you have any inkling of what I'm talking about you might want to take a look an this solution that uses SIGNALS to emulate condition variable behavior across processes on an Intel Linux PC. I happen to know that this particular solution works well and is, in fact, being used in software I've written for several companies (usually it is used in Financial Trading and Telecommunications software).

NOTE:
(intended for Linux 2.2.14 and previous. These solutions have not
been tested on Linux kernels greater than 2.2.14)

By Karl N. Redman
contact: karl.redman@gmail.com

Last updated: 1-3-2001


	THIS PACHAGE REQUIRES:
		GNU C compiler
		linux threads or pthreads



     Quick guide to running programs:

	   1. build the project (see INSTALL)

	   2. open 3 terminals (or login as the case may be)

	   3. execute the producer program once

	   4. execute a consumer with no arguments in another terminal

	   5. execute another consumer with 1 argument in another
	   terminal

	   6. use CTRL-s and CTRL-q to suspend and continue execution
              of each of the programs.

	   7. run as many as 32 consumers at a time. Type "uptime" to
              check the load average of your machine -notice that it's
              not floored.

     Explanation:

      The included source code demonstrates the use of condition
      variables for multi-platform C programming. Please read the
      Condition Variable FAQ included with this distribution for more
      information on condition variables.  

      The enclosed programs should compile and run on most UNIX type
      platforms with GNU gcc compiler and linker tools. The use of
      preprocessor directives has been implemented for the purpose of
      showing the contrast between Linux and most other UNIX types of
      operating systems. Below is an explanation of what these
      differences are.

      The POSIX standard for condition variables across processes is
      honored by most UNIX implementations except Linux. In order to
      provide uniform behavior of the condition variable mechanism a
      different solution for Linux had to be divined. The code
      examples provided here demonstrate one solution using signaling.


      The problem:

	  Linux does not provide a condition variable mechanism that
	  is sharable between processes. Solaris and AIX, for
	  instance, allow the use of a condition variable between two
	  separate process -Linux doesn't.

	  While Linux does allow for condition variables between
	  threads spawned from a single process, the kernel contains
	  no code nor memory allocation for the condition variable
	  mechanism. I realize this paragraph is redundant, but allot
	  of people don't know this and deny it.


      The Solution:
	  
	  Use a signaling mechanism in place of a condition variable
	  for Linux when shared condition variables are used, other
	  UNIX type implementations will use condition variables. 

	  Because of the excellent signaling infrastructure of Linux a
	  POSIX style condition variable may be emulated. This
	  emulated condition variable will perform a sigsuspend() to
	  produce a wait effect and will make use of a SIGCONT
	  (continue signal) to produce a wakeup effect.

	  The process that is waiting for the change of a variable in
	  another process will suspend execution until it receives a
	  SIGCONT from some other process. Below is an explanation of
	  an example of this behavior.

	  
      The Example:
	  
	  The following explains the example code contained within
	  this package from the Linux perspective only. Please see the
	  reference section of the Condition Variable FAQ for
	  information about how POSIX condition variables normally
	  operate. 

	  The program producer.c creates and attaches to a shared
	  memory region that will be used by it and any of it's
	  consumers. This shared memory region, masked by a structure,
	  contains an array that the producer continuously updates
	  and that the consumers continuously read from. Each time
	  the producer updates an element of the array (data buffer)
	  in shared memory, it sends a signal to consumer programs;
	  thereby causing consumers that are waiting to continue.

	  In the main loop of the program, the producer first updates
	  the data in the shared memory buffer then, under Linux, it
	  sends a continue signal to each consumer which has
	  registered itself with the producer program. A consumer
	  is considered registered if it has updated the list of
	  process IDs in shared memory and incremented the shared
	  memory variable that tracks the total number of clients.

	  The code for the consumer process, consumer.c, is where all
	  the action happens. After the consumer attaches the the
	  shared memory region that was created by the producer it
	  registers it self with the producer; taking the first
	  available consumer "slot" in the process id list.

	  In the main loop of the consumer process we check to see if
	  the consumer will be reading data from a position in the
	  shared data buffer that is equal to the position where the
	  producer will be witting data. If this occurs, we block
	  the ability for other programs to interrupt the consumer
	  process. Then we tell the consumer to suspend until a
	  continue signal has been sent to it from another process,
	  presumably the producer.

	  Once a continue (SIGCONT) signal is caught by the consumer,
	  it replaces the signal mask that disallows interrupt
	  signals with the original one. The consumer then continues
	  operation by printing the data it read from the shared
	  memory region, updates the position it will read from next,
	  possibly sleeps for a little while (if desired), and
	  finally repeats the main loop.

	  It should be noted that in order to use a sleep routine
	  within the consumer process a special signal handler must be
	  installed. This signal handler must block receipt of a
	  continue signal which may be sent from the producer at any
	  time. If SIGCONT were not blocked, the consumer process
	  would wake up as soon as the producer process sent it a
	  SIGCONT.
	 
	  

     Conclusion:

	The result of all this is that you end up with one producer
	process and a bunch of consumer processes that only use as
	much processor time as is required. If you were to remove the
	condition variable mechanism here, a single consumer would
	continuously poll the producers data position, thereby
	causing the CPU to be used far more than is necessary.



