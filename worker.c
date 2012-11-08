#include "worker.h"

void *worker_thread(void *v) {
    fprintf(stderr,"Thread 0x%0lx started.\n", (long)pthread_self());

    struct thread_args *targs = (struct thread_args*)v;
    list_t **list = targs->list;
    pthread_cond_t *poolsignal = targs->poolsignal;
    pthread_mutex_t *loglock = targs->loglock;
    pthread_mutex_t *condlock = targs->condlock;
                                                                                                       
    while(targs->stillrunning){
			fprintf(stderr, "size is %i\n", list_size(*list));
      if (list_size(*list) == 0){
				fprintf(stderr, "waiting for cv\n");
				pthread_cond_wait(poolsignal, condlock);
				fprintf(stderr, "finished waiting for cv\n");
			} else {
      	int reqsocket = list_dequeue(*list);
				fprintf(stderr,"\nsuccesfully dqueued value to %i\n", reqsocket);
			/*
      //Once you're outside the lock and have a socket, which we'll assume is called reqsocket:
      char *reqbuffer = malloc(sizeof(char)*1024);
      if (getrequest(reqsocket, reqbuffer, 1024)<0){
        fprintf(stderr, "Failed to retrieve request on socket %i.\n",reqsocket);
        continue;//Or something of the sort to move on with the loop
      }

      struct stat fileStat;
      stat(reqbuffer,&fileStat);
      //Construct the full filepath
      //If the file exists, get its size and pass the whole thing back as a 200 response
      //If it does not, return a 404 response
      //Log the request*/
			}
    }
    fprintf(stderr,"Thread 0x%0lx done.\n", (long)pthread_self());
    return NULL;
}
