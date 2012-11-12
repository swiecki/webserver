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
	strcat(logpath,"/weblog.txt");
	fprintf(stderr, "%s\n", logpath);
	while(*(targs->stillrunning)){
		fprintf(stderr, "sr is %i\n", *(targs->stillrunning));
		if (list_size(*list) == 0){
			pthread_mutex_lock(condlock);
			while(list_size(*list)== 0 && *(targs->stillrunning) == TRUE){
				fprintf(stderr, "sr is %i\n", *(targs->stillrunning));
				pthread_cond_wait(poolsignal, condlock);
			}
			if(*(targs->stillrunning) == FALSE){
				break;
			}
			pthread_mutex_unlock(condlock);
		} 
		else {
			struct __list_node *node = list_dequeue(*list);
			int reqsocket = node->data;
			int port = node->port;
			char *ip = node->ip;
			free(node);
			//Once you're outside the lock and have a socket, which we'll assume is called reqsocket:
			char *reqbuffer = malloc(sizeof(char)*1024);
			char reqpath[1024];
			int reqtype = 0;
			int respsize = 0;
			char header[1024];
			getcwd(reqpath,1024);
			fprintf(stderr,"testing prefix %s\n", reqpath);
			fprintf(stderr,"testing filename %s\n",reqbuffer);
      if (getrequest(reqsocket, reqbuffer, 1024)<0){
        fprintf(stderr, "Failed to retrieve request on socket %i.\n",reqsocket);
        continue;//Or something of the sort to move on with the loop
      }
			fprintf(stderr,"testing filename %s\n",reqbuffer);
			struct stat fileStat;
			strcat(reqpath,reqbuffer);
			if (stat(reqpath,&fileStat)==0){
				//If the file exists, get its size and pass the whole thing back as a 200 response
				//set up header
				sprintf( header, HTTP_200, (int) fileStat.st_size);
				respsize = strlen(header)*sizeof(char) + fileStat.st_size;
				char *response = malloc(strlen(header)*sizeof(char) + fileStat.st_size);
				reqtype = 200;
				int respStream = open(reqpath,O_RDONLY);
				char contents[fileStat.st_size];
				read(respStream,contents,fileStat.st_size);
				strcpy(response,header);
				fprintf(stderr,"\n %i, %i %s\n", sizeof(response), sizeof(contents), response);
				strcat(response,contents);
		
				//Return the data
				senddata(reqsocket,response,strlen(response)*sizeof(char));
				free(response);
				free(reqbuffer);
				//close the file
				close(respStream);
				} else {
				//If it does not, return a 404 response
				sprintf( header, HTTP_404);
				respsize = strlen(header)*sizeof(char);
				reqtype = 404;
				int respSize = 256;
				senddata(reqsocket,header,respSize);
				}	

		//Log the request, close the socket, and continue
		//Lock the mutex
		pthread_mutex_lock(loglock);
		//Open the file, write to it, then close
		time_t current_time;
		char *c_time_string;
		current_time = time(NULL);
		c_time_string = ctime(&current_time);
		c_time_string[strlen(c_time_string)-1] = '\0';
		int logdesc = open(logpath,O_WRONLY|O_APPEND);
		char entry[1024];
		sprintf( entry, "%s:%i %s \"GET %s\" %i %i \n",ip,port,c_time_string,reqpath,reqtype,respsize);
		
		write(logdesc,entry,strlen(entry));
		close(logdesc);
		//Relinquish the lock
		pthread_mutex_unlock(loglock);
		close(reqsocket);
	
   	 }
	}
    fprintf(stderr,"Thread 0x%0lx done.\n", (long)pthread_self());
		free(logpath);
    return NULL;
}
