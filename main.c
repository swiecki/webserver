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
	list_t *list;
	pthread_mutex_t *loglock;
	pthread_cond_t *poolsignal;
};

void *worker_thread(void *v) {
    fprintf(stderr,"Thread 0x%0lx started.\n", (long)pthread_self());
    
    struct thread_args *targs = (struct thread_args*)v;

	//try to get a job from the queue and execute it in a loop
	
	//Once you're outside the lock and have a socket, which we'll assume is called reqsocket:
	char *reqbuffer = malloc(sizeof(char)*1024);
	if (getrequest(reqsocket, &reqbuffer, 1024)<0){
		fprintf(stderr, "Failed to retrieve request on socket %i.\n",reqsocket);
		continue;//Or something of the sort to move on with the loop
	}
	
    
    fprintf(stderr,"Thread 0x%0lx done.\n", (long)pthread_self());    
    return NULL;
}

// global variable; can't be avoided because
// of asynchronous signal interaction
int still_running = TRUE;
void signal_handler(int sig) {
    still_running = FALSE;
}


void usage(const char *progname) {
    fprintf(stderr, "usage: %s [-p port] [-t numthreads]\n", progname);
    fprintf(stderr, "\tport number defaults to 3000 if not specified.\n");
    fprintf(stderr, "\tnumber of threads is 1 by default.\n");
    exit(0);
}

void runserver(int numthreads, unsigned short serverport) {

		list_t *thelist = (list_t*)malloc(sizeof(list_t));
		list_init(thelist);
		
		//Initialize mutex lock and condition variable
		pthread_mutex_t *loglock = NULL;
		pthread_mutex_init(&loglock,NULL);
		pthread_cond_t *poolsignal = NULL;
		pthread_cond_init(&poolsignal,NULL);

		struct thread_args targs = {&thelist,&loglock,&poolsignal}; //pointer to linked list head goes in there and cond variable and mutex
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
            continue;
        }
        
        addr_len = sizeof(client_address);
        memset(&client_address, 0, addr_len);

        int new_sock = accept(main_socket, (struct sockaddr *)&client_address, &addr_len);
        if (new_sock > 0) {
            
            fprintf(stderr, "Got connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

           ////////////////////////////////////////////////////////
           /* You got a new connection.  Hand the connection off
            * to one of the threads in the pool to process the
            * request.
            *
            * Don't forget to close the socket (in the worker thread)
            * when you're done.
            */
           ////////////////////////////////////////////////////////
					//throw connection in the queue, signal condition variable

        }
    }
    fprintf(stderr, "Server shutting down.\n");
        
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
