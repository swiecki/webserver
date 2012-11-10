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
#include "worker.h"

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
            continue;
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
		/*This block is commented out because in theory this is where we would join the threads, but in practice only one thread was ever exiting properly, and the rest were hanging. As such, we have two memory leaks per thread run. One of the frees for these threads is accomplished outside the while loop in tha thread, while the other is implicit in the thread join. 
		fprintf(stderr,"%i",still_running);
		int p = 0;
		for(;p<numthreads;p++){
				pthread_mutex_lock(&condlock);
				pthread_cond_broadcast(&poolsignal);
				pthread_join(threads[p],NULL);
				pthread_mutex_unlock(&condlock);
		}
		*/
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
