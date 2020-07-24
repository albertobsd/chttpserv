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
#include "conf.h"
#include "http_headers.h"
#include "http_request.h"
#include "http_serve_client.h"
#include "debug.h"

#define BACKLOG 16

void * thread_process(void *vargp);
/*
int read_method(HTTP_REQUEST client_request,char *buffer,int *offset);
int read_headers(HTTP_REQUEST client_request,char *buffer,int *offset);
int read_buffer(int fd,char *buffer,int length);
int check_for_file_and_send(HTTP_REQUEST client_request,int client_fd);
int send_file(int cliend_fd,char *path);
*/

int main(int argc,char **argv)	{
	int index, j = 0,s;
	pthread_attr_t attr;
	pthread_t tid;
	int i = 0;			/* Generic counter			*/
	int servfd = 0;		/* Server file descriptor	*/
	int	clientfd = 0;	/* Client file descriptor	*/
	int b;				/* Size of socketserver		*/
	struct in_addr ip;	/* aux in_addr variable		*/
	int *tothread = NULL;
	struct sockaddr_in *socketserver;
	/* END of localvariable declarations 	*/
	debug_init();
	memset(&ip,0,sizeof(struct in_addr));
	read_config();
	debug_status();
	while(i < CONFIG_VAR_LENGTH_CONST)	{
		printf("_CONFIG[%s] = %s\n",variables_config[i],_CONFIG[i]);
		i++;
	}
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
	listen(servfd, BACKLOG);
	while(1)	{
		b = sizeof(socketserver);
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
		s = pthread_create(&tid,&attr,thread_process,(void *)tothread);
		if(s != 0)	{
			perror("pthread_create");
			exit(9);
		}
		pthread_attr_destroy(&attr);
	}
	return 0;
}

void * thread_process(void *vargp)	{
	int *aux = (int *)vargp;
	int clientfd = 0,s;
	printf("Procesar Peticion\n");
	if(aux != NULL)	{
		clientfd =aux[0];
		debug_free(aux);
		http_serve_client_do(clientfd);
	}
	debug_status();
	printf("Saliendo!\n");
	pthread_exit(NULL);
}
