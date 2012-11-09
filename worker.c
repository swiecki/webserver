#include "worker.h"

void *worker_thread(void *v) {
    fprintf(stderr,"Thread 0x%0lx started.\n", (long)pthread_self());

    struct thread_args *targs = (struct thread_args*)v;
    list_t **list = targs->list;
    pthread_cond_t *poolsignal = targs->poolsignal;
    pthread_mutex_t *loglock = targs->loglock;
    pthread_mutex_t *condlock = targs->condlock;
	char *logpath = malloc(sizeof(char)*512);
	getcwd(logpath,512);
	strcat(logpath,"/weblog.txt\0");//TODO: Make this less garbage
    while(targs->stillrunning){
			fprintf(stderr, "size is %i\n", list_size(*list));
      if (list_size(*list) == 0){
				fprintf(stderr, "waiting for cv\n");
				pthread_cond_wait(poolsignal, condlock);
				fprintf(stderr, "finished waiting for cv\n");
			} else {
      	int reqsocket = list_dequeue(*list);
				fprintf(stderr,"\nsuccesfully dqueued value to %i\n", reqsocket);

      //Once you're outside the lock and have a socket, which we'll assume is called reqsocket:
      char *reqbuffer = malloc(sizeof(char)*1024);
	char *reqpath = malloc(sizeof(char)*1024);
	getcwd(reqpath,1024);
	fprintf(stderr,"testing prefix %s\n", reqpath);
      if (getrequest(reqsocket, reqbuffer, 1024)<0){
        fprintf(stderr, "Failed to retrieve request on socket %i.\n",reqsocket);
        continue;//Or something of the sort to move on with the loop
      }

	fprintf(stderr,"testing filename %s\n",reqbuffer);
	struct stat fileStat;
	strcat(reqpath,reqbuffer);
	reqpath[strlen(reqpath)-3] = '\0';//TODO: Make this less of a hack; right now there's an a appended to the path for no damn reason
	fprintf(stderr,"testing total path %s\n",reqpath);
	if (stat(reqpath,&fileStat)==0){
		//If the file exists, get its size and pass the whole thing back as a 200 response
		fprintf(stderr,"successful stat op\n");
		int respSize = 256+(int) fileStat.st_size;//TODO fix the header size
		fprintf(stderr,"size of out is %i\n",respSize);
		char response[respSize];
		char *header = "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\nContent-length: \r\n\r\n";//TODO: Needs to include filesize
		int respStream = open(reqpath,O_RDONLY);
		char contents[respSize-256];
		read(respStream,contents,respSize-256);
		strcpy(response,header);
		strcat(response,contents);
		
		//Return the data
		senddata(reqsocket,response,respSize);
		
		//close the file
		close(respStream);
	} else {
		//If it does not, return a 404 response
		char *header = "HTTP/1.0 404 Not found\r\n\r\n";
		int respSize = 256;
		senddata(reqsocket,header,respSize);
	}
	
	/*Currently commented out until it can be fully implemented.

	//Log the request, close the socket, and continue
		//Probably want to pass in the logfile name manually, but whatever
		//Lock the mutex
		pthread_mutex_lock(loglock);
		//Open the file, write to it, then close
		
		int logdesc = open(logpath,O_WRONLY|O_APPEND);
		char *entry;//TODO Make this a thing
		write(logdesc,entry,1024);
		close(logdesc);
		//Relinquish the lock
		pthread_mutex_unlock(loglock);
		close(reqsocket);
	*/
   	 }
	}
    fprintf(stderr,"Thread 0x%0lx done.\n", (long)pthread_self());
    return NULL;
}
