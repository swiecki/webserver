#include <pthread.h>
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "network.h"

struct thread_args {                                                                                   
  int *stillrunning;                                                                                   
  list_t **list;
  pthread_mutex_t *loglock;
  pthread_mutex_t *condlock;
  pthread_cond_t *poolsignal;
};  

// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}
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
    while(still_running){
        fprintf(stderr, "sr is %i\n", still_running);
        if (list_size(*list) == 0){
            pthread_mutex_lock(condlock);
            while(list_size(*list)== 0 && still_running == TRUE){
                fprintf(stderr, "sr is %i\n", still_running);
                pthread_cond_wait(poolsignal, condlock);
            }
            if(still_running == FALSE){
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


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

void runserver(int numthreads, unsigned short serverport) {

		char *logpath = malloc(sizeof(char)*512);
		getcwd(logpath,512);
		strcat(logpath,"/weblog.txt");//TODO: Make this less garbage

		int fd = open(logpath, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
		close(fd);
		free(logpath);        
		list_t *thelist = (list_t*)malloc(sizeof(list_t));
		list_init(thelist);
		
		//Initialize mutex lock and condition variable
		pthread_mutex_t loglock;
		pthread_mutex_init(&loglock,NULL);
		pthread_mutex_t condlock;
		pthread_mutex_init(&condlock,NULL);
		pthread_cond_t poolsignal;
		pthread_cond_init(&poolsignal,NULL);

		struct thread_args targs;
		targs.stillrunning = &still_running;
		targs.list = &thelist;
		targs.loglock = &loglock;
		targs.condlock = &condlock;
		targs.poolsignal = &poolsignal;


		pthread_t threads[numthreads];
		int i = 0;
		for (; i < numthreads; i++){
			if (0 > pthread_create(&threads[i], NULL, worker_thread, (void*)&targs)){
				printf("Error initializing thread");
			}
		}

    int main_socket = prepare_server_socket(serverport);
    if (main_socket < 0) {
        exit(-1);
    }
    signal(SIGINT, signal_handler);

    struct sockaddr_in client_address;
    socklen_t addr_len;

    fprintf(stderr, "Server listening on port %d.  Going into request loop.\n", serverport);
    while (still_running) {
        struct pollfd pfd = {main_socket, POLLIN};
        int prv = poll(&pfd, 1, 10000);

        if (prv == 0) {
            continue;
        } else if (prv < 0) {
            PRINT_ERROR("poll");
            still_running = FALSE;
				pthread_cond_broadcast(&poolsignal);
            break;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

					pthread_mutex_lock(&condlock);
					//fprintf(stderr,"about to enqueue\n");
					list_enqueue(thelist,new_sock, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
					pthread_cond_signal(&poolsignal);
					//fprintf(stderr,"successfully enqueued\n");
					pthread_mutex_unlock(&condlock);
				
        }
    }
		/*This block is commented out because in theory this is where we would join the threads, but in practice only one thread was ever exiting properly, and the rest were hanging. As such, we have two memory leaks per thread run. One of the frees for these threads is accomplished outside the while loop in tha thread, while the other is implicit in the thread join. */
		fprintf(stderr,"still running is %i\n",still_running);
		int p = 0;
		for(;p<numthreads;p++){
				pthread_join(threads[p],NULL);
		}
		
		pthread_mutex_destroy(&loglock);
		pthread_mutex_destroy(&condlock);
		pthread_cond_destroy(&poolsignal);
    fprintf(stderr, "Server shutting down.\n");
		list_clear(thelist);
		free(thelist);
    close(main_socket);
}


int main(int argc, char **argv) {
    unsigned short port = 3000;
    int num_threads = 1;

    int c;
    while (-1 != (c = getopt(argc, argv, "p:t:h"))) {
        switch(c) {
            case 'p':
                port = atoi(optarg);
                if (port < 1024) {
                    usage(argv[0]);
                }
                break;

            case 't':
                num_threads = atoi(optarg);
                if (num_threads < 1) {
                    usage(argv[0]);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
                break;
        }
    }

    runserver(num_threads, port);
    
    fprintf(stderr, "Server done.\n");
    exit(0);
}
