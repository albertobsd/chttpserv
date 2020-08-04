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
//#include "debug_bt.h"
#include "debug.h"

#define BACKLOG 16

void * thread_process(void *vargp);
void *thread_timeout(void *vargp);

int main(int argc,char **argv)	{
	int index, j = 0,s;
	pthread_attr_t attr;
	pthread_t tid;
	int i = 0;			/* Generic counter			*/
	int c = 0 ;		/* client couner	*/
	int servfd = 0;		/* Server file descriptor	*/
	int	clientfd = 0;	/* Client file descriptor	*/
	int b;				/* Size of socketserver		*/
	struct in_addr ip;	/* aux in_addr variable		*/
	int *tothread = NULL;
	struct sockaddr_in *socketserver;
	signal(SIGPIPE, SIG_IGN);
	/* END of localvariable declarations 	*/
	debug_init();
	http_serve_default_errors();	/*	Init default Error messages*/
	memset(&ip,0,sizeof(struct in_addr));
	server_config = http_serve_conf_init();
	if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 2;
	}
	inet_aton(_CONFIG[IP], &ip);
	socketserver = (struct sockaddr_in*) debug_calloc(1,sizeof(struct sockaddr_in));
	socketserver->sin_family = AF_INET;
	socketserver->sin_port   = htons((uint16_t)strtol(_CONFIG[PORT],NULL	,10));
	socketserver->sin_addr.s_addr = ip.s_addr;
	//printf("Try bind: http://%s:%s/\n",inet_ntoa(socketserver->sin_addr),_CONFIG[PORT]);

	if (bind(servfd,(struct sockaddr *) socketserver, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(3);
	}
	b = sizeof(socketserver);
	//debug_status();
	listen(servfd, BACKLOG);
	while(1)	{
		if((clientfd = accept(servfd,(struct sockaddr *)socketserver,(socklen_t*) &b)) < 0) {
			perror("accept");
			exit(5);
		}
		s = pthread_attr_init(&attr);
		if (s != 0)	{
			perror("pthread_attr_init");
			exit(6);
		}
		/*
			set stack size of the pthread to min
		*/
		s = pthread_attr_setstacksize(&attr,PTHREAD_STACK_MIN);
		if(s != 0)	{
			perror("pthread_attr_setstacksize");
			exit(7);
		}
		/*
			Set deteched of the pthread
		*/
		s = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		if(s != 0)	{
			perror("pthread_attr_setstacksize");
			exit(8);
		}
		/*
			Values to pass to the pthread
		*/
		tothread = NULL;
		tothread = (int*) debug_malloc(sizeof(int)*5);
		if(tothread == NULL)	{
			perror("debug_malloc");
			exit(8);
		}
		tothread[0] = clientfd;			/*	Client fd */
		tothread[1] = c;
		tothread[2]	= server_config->timeout;
		tothread[3]	= 1;
		s = pthread_create(&tid,&attr,thread_process,(void *)tothread);
		if(s != 0)	{
			perror("pthread_create");
		}
		pthread_attr_destroy(&attr);
		c++;
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
	HTTP_SERVE_CLIENT hsc;
	pthread_t tprocess,ttimeout;
	int *aux = (int *)vargp;
	int clientfd = 0, clientcouner = 0,s;
	if(aux != NULL)	{
		clientfd = aux[0];
		clientcouner = aux[1];
		//printf("Entrando %i\n",clientcouner);
		hsc = http_serve_client_init(clientfd,clientcouner);
		hsc->vargp = vargp;
		s = pthread_create(&tprocess,NULL,http_serve_client_do,hsc);
		if(s != 0)	{
			perror("pthread_create");
		}
		else	{
			s = pthread_create(&ttimeout,NULL,thread_timeout,vargp);
			if(s != 0)	{
				perror("pthread_create");
			}
			else	{
				pthread_join(ttimeout,NULL);
				if(aux[3] == 1)	{
					pthread_cancel(tprocess);
					//fprintf(stderr,"thread %i Cancelando pthread_cancel tprocess\n",clientcouner);
					http_serve_client_free(hsc);
				}
				else	{
					pthread_join(tprocess,NULL);
					//fprintf(stdout,"thread %i Ejecutano pthread_join tprocess, termino limpiamente\n",clientcouner);
				}
			}
		}
		debug_free(vargp);
	}
	printf("thread %i pthread_exit\n",clientcouner);
	debug_status();
	//debug_list();	
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
