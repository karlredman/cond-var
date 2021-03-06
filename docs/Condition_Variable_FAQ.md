# Condition Variable Mini-FAQ

By Karl N. Redman

Contact Info: <karl.redman@gmail.com>

Project page: [karlredman.github.io/cond_var](https://github.com/karlredman/cond-var)

Last updated: 4-15-2017

## Example of a condition variable program:

An example of the use of condition variables is provided here: [http://karlredman.github.io/cond_var](http://karlredman.github.io/cond_var). Please note that the example provided here is a multi-platform solution that gives linux and non-linux solutions.

## What is a condition variable?

A condition variable is an atomic waiting and signaling mechanism which allows a process or thread to temporarily stop execution until a signal is received, indicating the change of a shared variable within a predicate, from some other process or thread.

What this means is that a condition variable provides a way to temporarily "stop" the execution of a program until it is signaled to continue via another process or thread as a result of the change in the value of one or more variables. The process of checking the predicate for a change and waiting for a signal happens uninterrupted (atomically) within a process or thread.

The term "condition variable" is sometimes considered misleading because the mechanism does not rely on a variable but rather on the process of signaling at the system level. The term comes from the use of the mechanism rather than the actual operation. Condition variables are most often used as a way for one process or thread to "notify" another process or thread of the change in the value of a variable which is shared between processes and/or threads.


## Practical uses for the condition variable mechanism:

* Notify a writer thread that a reader thread has filled it's data set.

* Notify consumer processes that a producer thread has updated a shared data set.

* Anytime a process or thread needs to wait for the change of one or more shared variables which are updated by another process or thread


 * It is my experience that condition variables are use rarely. It is also my experience that the operation of many programs may be made more efficient through the use of condition variables. Very often semaphores are used in place of condition variables as well; as a result of not being aware of the behavior of the condition variable mechanism.


## What's the difference between a semaphore and a condition variable?

A semaphore is used to synchronize processes and threads. It is not intended to be as an asynchronous blocking or waiting mechanism. A condition variable is most often used as an asynchronous blocking or waiting mechanism. It's purpose is to allow multiple processes and/or threads to wait on some predicate (condition) independently.



## Why not use a semaphore?

A semaphore may cause some or all processes or threads to wait for the release of a semaphore before continuing operation. The very nature of a semaphore guarantees that no other process will continue until some condition is met. There are some possibly multi platform conditions where the use of a semaphore would cause the wrong processes to wait or cease execution temporarily.

### An Example where the use of a condition variable is more appropriate than a semaphore follows:

I came across this problem in the real world while working for a financial data provider company. The situation is that there is a time critical (real time) "producer" process which receives data from a socket, unpacks it, and deposits the data into a shared memory buffer. At the same time, there are one or more "consumer" processes that read the data from the shared buffer simultaneously.

#### The specifications for this real time data delivery system are as follows:

* There is a shared "ring buffer" that the producer puts data into and the consumers read from.
	
* No consumer may read from a memory region where the producer has not deposited data; consumers may not "pass up" the producer.

* The producer is only allowed to block on a socket; it must otherwise never be stopped or slowed down by any other process or thread.

* Consumer processes may run much slower than the producer or may halt or "hang up" during execution; the producer may "pass up" any and/or all consumers.
#### Scenario:

The producer program is a daemon that puts data into a memory region that is shared and readable by other programs on a computer system. The producer program will put data into the first memory "slot" or "bin" of a set and continue filling successive "bins" until it reaches the last. Once the last memory slot is filled, the producer then starts over at the beginning and overwrites the data in the first "bin" with new data; this is referred to as a ring buffer.

The problem where a condition variable comes in is when a consumer process has read from all the memory "bins" and has caught up with the producer. We do not want the consumer to pass the producer, for it may then read data that is out of date or just bad altogether. Regardless of why the producer may have stopped putting data into memory "slots", the consumer must never check a "slot" past the producer.

There are a few solutions that we could use in this scenario. One would be to continuously poll or check the position that the producer has last written to in a loop. The problem with this solution is that the consumer program would then cause over usage of the processor (more than one consumer polling a shared value would bring just about any PC today to it's knees).

    ...

    x = csv; //current slot to check

    y = shared_mem_producer_slot //producer's current slot

    while(x < y)
    {
        //check the shared memory slot
        y = shared_mem_producer_slot //producer's current slot
    }

    //get data from slot 'x' for processing
    memcopy(data[x], shared_memory_region[x],data_length);

    ...

Another solution might be the use of a semaphore. In this scenario the producer would, when "waiting" for whatever reason, increase the value of a semaphore by the number of consumer processes currently running. The reason for using the number of consumers is so that each consumer, when or if it catches up with the producer, gets a turn at reading the data when the producer continues. One problem with this solution is that under certain implementations and uses of this scenario the producer would end up waiting for all consumers to decrement the semaphore before continuing; causing the producer to block. Another (more often) problem is that the consumers would then be synchronized; if, for instance, one of five consumers somehow locks up, then the semaphore is never released -causing most or all consumers to wait indefinitely.


Using a condition variable here allows the consumers to be asynchronously notified when the producer has placed new data into the ring buffer. When each consumer catches up with the producer it is told to cease execution until it is notified by the signaling of the condition variable mechanism. Once the producer has put new data into a slot, it sends a signal to the threads or process that are waiting; notifying each of them that new data has arrived.


