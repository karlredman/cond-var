//procon.h

#include <sys/types.h>

// various constants
#define MEM_SZ 1024		/* size of shared mem. */
#define MY_BUFSIZ 50		/* a buffer size */
#define MAXCLIENTS 32		/* max number of consumers */
#define MAXDATA_NUM 200		/* where to wrap the data variable "j"
				 of the producer*/


/* a structure that will be used by the producer and each of the
   consumers. It containes the things needed to keep track of one
   another and some data.
*/
typedef struct _SHARED_USE_ST
{
  int dataPos;			/* producer mem slot position */
  int data[BUFSIZ];		/* a data buffer */
  int clients;			/* number of consumers */
  int terminate;		/* tell consumers to terminate */
  pid_t pids[MAXCLIENTS];	/* list of process IDs of consumers */
  pid_t producer_pid;		/* the producer's process ID */

#ifndef __linux__
  //if it's not linux make global shared mutex and condition var
  pthread_mutex_t *mutex;
  pthread_cond_t *cv;
#endif

} shared_use_st;
