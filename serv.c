#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<stdio.h>
/*
	custom functions
*/
#include "util.h"
#include "http_headers.h"
#include "http_request.h"
#include "http_serve_client.h"
#include "http_serve_conf.h"

#define BACKLOG 16

/*
	Server defined proccess funtion:
*/
void *thread_process(void *vargp);
void *thread_timeout(void *vargp);
pthread_attr_t* create_thread_attributes(size_t stack_size, int detached);


int main(int argc,char **argv)	{
	int index, j = 0,s;
	pthread_attr_t *attr;
	pthread_t tid;
	int i = 0;			/* Generic counter			*/
	int clien_counter = 0 ;		/* client couner	*/
	int servfd = 0;		/* Server file descriptor	*/
	int	clientfd = 0;	/* Client file descriptor	*/
	int size_sockserver;				/* Size of socketserver		*/
	struct in_addr ip;	/* aux in_addr variable		*/
	int *tothread = NULL;
	struct sockaddr_in *socketserver;
	signal(SIGPIPE, SIG_IGN);
	/* END of localvariable declarations 	*/
	http_serve_default_errors();	/*	Init default Error messages*/
	memset(&ip,0,sizeof(struct in_addr));
	server_config = http_serve_conf_init();
	if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_error(__FILE__, __LINE__, "Failed to generate socket");
		exit(EXIT_FAILURE);
	}
	inet_aton(_CONFIG[IP], &ip);
	socketserver = (struct sockaddr_in*) calloc(1,sizeof(struct sockaddr_in));
	socketserver->sin_family = AF_INET;
	socketserver->sin_port   = htons((uint16_t)strtol(_CONFIG[PORT],NULL,10));
	socketserver->sin_addr.s_addr = ip.s_addr;
	//printf("Try bind: http://%s:%s/\n",inet_ntoa(socketserver->sin_addr),_CONFIG[PORT]);

	if (bind(servfd,(struct sockaddr *) socketserver, sizeof(struct sockaddr)) < 0) {
		log_error(__FILE__, __LINE__, "Failed to bind socket");
		exit(EXIT_FAILURE);
	}
	size_sockserver = sizeof(sizeof(struct sockaddr_in));
	listen(servfd, BACKLOG);
	while(1)	{
		if((clientfd = accept(servfd,(struct sockaddr *)socketserver,(socklen_t*) &size_sockserver)) < 0) {
			log_error(__FILE__, __LINE__, "Failed to accept client");
			exit(EXIT_FAILURE);
		}
		
		attr = create_thread_attributes(PTHREAD_STACK_MIN,1);
		
		if(attr == NULL) {
			log_error(__FILE__, __LINE__, "Failed in create_thread_attributes");
			exit(EXIT_FAILURE);
		}
		
		/*
			Values to pass to the pthread
		*/
		
		tothread = NULL;
		tothread = (int*) malloc(sizeof(int)*5);
		if(tothread == NULL)	{
			log_error(__FILE__, __LINE__, "Failed in function malloc");
			exit(EXIT_FAILURE);
		}
		tothread[0] = clientfd;						/*	Client file descriptor */
		tothread[1] = clien_counter;				/*	Clien counter*/
		tothread[2]	= server_config->timeout;		/* Server timeout for Client requests*/
		tothread[3]	= 1;
		s = pthread_create(&tid,attr,thread_process,(void *)tothread);
		if(s != 0)	{
			log_error(__FILE__, __LINE__, "Failed to create pthread process for client");
		}
		pthread_attr_destroy(attr);
		free(attr);
		clien_counter++;
	}
	//printf("Saliendo\n");
	return 0;
}
/*
 *  Main thread process,
 *  vargp  is a pointer passed from	pthread_create function.
 *	It contains a pointer to int* array now with just one item
 *	(int*) vargp [0] client socket file descriptor
 */
void *thread_process(void *vargp)	{
	pthread_attr_t *attr;
	HTTP_SERVE_CLIENT hsc;
	pthread_t tprocess,ttimeout;
	int *aux = (int *)vargp;
	int clientfd = 0, clientcouner = 0,s;
	if(aux != NULL)	{
		clientfd = aux[0];
		clientcouner = aux[1];
		hsc = http_serve_client_init(clientfd,clientcouner);
		hsc->vargp = vargp;
		
		attr = create_thread_attributes(PTHREAD_STACK_MIN,0);
		if(attr == NULL) {
			log_error(__FILE__, __LINE__, "Failed in create_thread_attributes function");
			exit(EXIT_FAILURE);
		}
		
		s = pthread_create(&tprocess,NULL,http_serve_client_do,hsc);
		if(s != 0)	{
			log_error(__FILE__, __LINE__, "Failed to create pthread process for client");
			pthread_attr_destroy(attr);
			free(attr);
		}
		else	{
			pthread_attr_destroy(attr);
			free(attr);

			attr = create_thread_attributes(PTHREAD_STACK_MIN,0);
			if(attr == NULL) {
				log_error(__FILE__, __LINE__, "Failed in create_thread_attributes function");
				exit(EXIT_FAILURE);
			}
			
			s = pthread_create(&ttimeout,attr,thread_timeout,vargp);
			if(s != 0)	{
				log_error(__FILE__, __LINE__, "Failed to create pthread process for timeout");
				exit(EXIT_FAILURE);
			}
			else	{
				pthread_join(ttimeout,NULL);
				if(aux[3] == 1)	{
					pthread_cancel(tprocess);
					fprintf(stderr,"thread %i Canceling pthread_cancel tprocess\n",clientcouner);
					http_serve_client_free(hsc);
				}
				else	{
					pthread_join(tprocess,NULL);
					fprintf(stdout,"thread %i executing pthread_join tprocess, success!\n",clientcouner);
				}
			}
			pthread_attr_destroy(attr);
			free(attr);
		}
		free(vargp);
	}
	printf("thread %i pthread_exit\n",clientcouner);
	pthread_exit(NULL);
}

/*
	This function 'thread_timeout' help to close the main thread process
	if it exceeds the timeout value in seconds-
	vargp is an integer pointer to array with
	vargp[2] timeout in seconds
	vargp[3] status of the main thread: 1 if main thread is still running
																			0 if th main thread already exit
*/
void *thread_timeout(void *vargp)	{
	int *aux = (int *)vargp;
	int timeout;
	if(aux != NULL)	{
		do {
			//fprintf(stderr,"thread %i timeout: sleep\n",aux[1]);
			sleep(1);
			aux[2]--;
		} while(aux[2] > 0 && aux[3] == 1 );
	}
	pthread_exit(NULL);
}

pthread_attr_t* create_thread_attributes(size_t stack_size, int detached) {
    pthread_attr_t *attr = (pthread_attr_t *) malloc(sizeof(pthread_attr_t));
    if (attr == NULL) {
		log_error(__FILE__, __LINE__, "Failed to allocate memory for pthread_attr_t");
        return NULL;
    }
    // Initialize attributes object
    if (pthread_attr_init(attr) != 0) {
		log_error(__FILE__, __LINE__, "Failed pthread_attr_init");
        free(attr);
        return NULL;
    }

    // Set stack size attribute
    if (pthread_attr_setstacksize(attr, stack_size) != 0) {
		log_error(__FILE__, __LINE__, "Failed pthread_attr_setstacksize");
        pthread_attr_destroy(attr);
        free(attr);
        return NULL;
    }

    // Set detach state
    int detach_state = (detached) ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;
    if (pthread_attr_setdetachstate(attr, detach_state) != 0) {
		log_error(__FILE__, __LINE__, "Failed pthread_attr_setdetachstate");
        pthread_attr_destroy(attr);
        free(attr);
        return NULL;
    }
    return attr;
}
