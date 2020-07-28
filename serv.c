#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>

/*
	custom functions
*/
#include "util.h"
#include "http_headers.h"
#include "http_request.h"
#include "http_serve_client.h"
#include "http_serve_conf.h"
#include "debug.h"

#define BACKLOG 16

void * thread_process(void *vargp);

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
	/* END of localvariable declarations 	*/
	debug_init();
	http_serve_default_errors();	/*	Init default Error messages*/
	memset(&ip,0,sizeof(struct in_addr));
	server_config = http_serve_conf_init();
	debug_status();
	if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 2;
	}
	inet_aton(_CONFIG[IP], &ip);
	socketserver = debug_calloc(1,sizeof(struct sockaddr_in));
	socketserver->sin_family = AF_INET;
	socketserver->sin_port   = htons((uint16_t)strtol(_CONFIG[PORT],NULL	,10));
	socketserver->sin_addr.s_addr = ip.s_addr;
	printf("Try bind: http://%s:%s/\n",inet_ntoa(socketserver->sin_addr),_CONFIG[PORT]);

	if (bind(servfd,(struct sockaddr *) socketserver, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(3);
	}
	b = sizeof(socketserver);
	listen(servfd, BACKLOG);
	while(1)	{
		if((clientfd = accept(servfd,(struct sockaddr *)socketserver, &b)) < 0) {
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
		tothread = debug_malloc(sizeof(int)*2);
		if(tothread == NULL)	{
			perror("debug_malloc");
			exit(8);
		}
		tothread[0] = clientfd;			/*	Client fd */
		tothread[1] = c;
		s = pthread_create(&tid,&attr,thread_process,(void *)tothread);
		if(s != 0)	{
			perror("pthread_create");
		}
		pthread_attr_destroy(&attr);
		c++;
	}
	printf("Saliendo\n");
	return 0;
}

/*
 *  Main thread process,
 *  vargp  is a pointer passed from	pthread_create function.
 *	It contains a pointer to int* array now with just one item
 *	(int*) vargp [0] client socket file descriptor
 */
void * thread_process(void *vargp)	{
	int *aux = (int *)vargp;
	int clientfd = 0, s,clientcouner = 0;
	if(aux != NULL)	{
		clientfd = aux[0];
		clientcouner = aux[1];
		debug_free(vargp);
		/*	free the vargp pointer is no longer needed because
		we already save the clienfd*/
		http_serve_client_do(clientfd,clientcouner);
	}
	debug_status();
	printf("Saliendo\n");
	pthread_exit(NULL);
}
