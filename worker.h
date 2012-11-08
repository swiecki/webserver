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

void *worker_thread(void *v);
