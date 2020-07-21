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
#include"util.h"
#include"conf.h"
#include "http_headers.h"
#include "http_request.h"

#define BACKLOG 16

void * procesar_peticion(void *vargp);
int read_method(HTTP_REQUEST client_request,char *buffer,int *offset);
int read_headers(HTTP_REQUEST client_request,char *buffer,int *offset);
int read_buffer(int fd,char *buffer,int length);

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
	memset(&ip,0,sizeof(struct in_addr));
	read_config();
	while(i < CONFIG_VAR_LENGTH_CONST)	{
		printf("_CONFIG[%s] = %s\n",variables_config[i],_CONFIG[i]);
		i++;
	}
	if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 2;
	}
	inet_aton(_CONFIG[IP], &ip);
	socketserver = calloc(1,sizeof(struct sockaddr_in));
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
		tothread = malloc(sizeof(int)*1);
		if(tothread == NULL)	{
			perror("malloc");
			exit(8);
		}
		tothread[0] = clientfd;			/*	Client fd */
		s = pthread_create(&tid,&attr,procesar_peticion,(void *)tothread);
		if(s != 0)	{
			perror("pthread_create");
			exit(9);
		}
		pthread_attr_destroy(&attr);
	}
	return 0;
}

void * procesar_peticion(void *vargp)	{
	int *aux = (int *)vargp;
	FILE *client = NULL;
	int maxrequestsize = 0,offset = 0,realrequestsize = 0;
	int clientfd = 0,s;
	char *buffer;
	HTTP_REQUEST client_request;
	printf("Procesar Peticion\n");
	if(aux != NULL)	{
		clientfd = aux[0];
		free(aux);
		client = fdopen(clientfd,"rb+");
		if(client != NULL)	{
			maxrequestsize = strtol(_CONFIG[MAXREQUESTSIZE],NULL,10);
			printf("maxrequestsize :%i \n",maxrequestsize);
			buffer = (char*) malloc(maxrequestsize);
			if(buffer == NULL)	{
				perror("malloc");
				exit(-1);
			} 
			client_request = http_request_init();
			realrequestsize = read_buffer(clientfd,buffer,maxrequestsize);
			if(realrequestsize < maxrequestsize)	{
				buffer = realloc(buffer,realrequestsize+1);
				if(buffer == NULL)	{
					perror("realloc");
					http_request_free(client_request);
					free(buffer);
					fclose(client);	
					close(clientfd);
					pthread_exit(NULL);
				}
			}
			printf("Leido bytes %i:\n%s\n",realrequestsize,buffer);
			s = read_method(client_request,buffer,&offset);
			if(s != 0)	{
				//Enviar Error
				http_request_free(client_request);
				free(buffer);
				fclose(client);	
				close(clientfd);
				pthread_exit(NULL);
			}
			printf("Offset: %i\n",offset);
			s = read_headers(client_request,buffer,&offset);
			printf("Leido: %s\n",buffer);
			http_request_free(client_request);
			fclose(client);	
		}
		close(clientfd);
	}	
	pthread_exit(NULL);
}

int read_buffer(int fd,char *buffer,int length)	{
	int offset = 0,readed,toread;
	if(buffer != NULL)	{
		do	{
			toread = (offset+64 < length )  ? 64: length-offset;
			readed = recv(fd,buffer +offset,64,MSG_DONTWAIT);
			if(readed >= 0)
				offset+= readed;
		}while(readed != -1 && offset <length);
	}
	return offset;
}

int read_method(HTTP_REQUEST client_request,char *buffer,int *offset)	{
	printf("read_method\n");
	char *token,*aux = NULL;
	char *http_tokens[3],*http_token,*http_aux = NULL;
	int i = 0;
	int current_offset = (offset != NULL) ? offset[0] : 0;
	token = strtok_r(buffer + current_offset,"\r\n\t",&aux);
	if(token != NULL)	{
		current_offset = strlen(token) + 1;
		http_token = strtok_r(token," ",&http_aux);
		while(i < 3 && http_token  != NULL)	{
			http_tokens[i] = http_token;
			printf("Token encontrado %i: %s\n",i,http_tokens[i]);
			http_token = strtok_r(NULL," ",&http_aux);
			i++;
		}
		if(i != 3)	{
			printf("i != 3\n");
			client_request->error = 1;
			return 1;
		}
		http_request_set_method(client_request,http_tokens[0]);
		http_request_set_uri(client_request,http_tokens[1]);
		http_request_set_version(client_request,http_tokens[2]);
	}
	if(offset != NULL){
		offset[0] = current_offset;
	}
	return 0;
}

int read_headers(HTTP_REQUEST client_request,char *buffer,int *offset)	{
	printf("read_headers\n");
	if(client_request == NULL|| offset == NULL || buffer == NULL)	{
		exit(1);
	}
	char *token,*token1, *aux = NULL,*aux1 =NULL;
	int current_offset = offset[0],length, entrar = 1;
	printf("current_offset: %i\n",current_offset);
	printf("%p : %s\n",buffer,buffer + current_offset);
	client_request->_HEADERS = http_headers_init();
	token = strtok_r(buffer + current_offset,"\n",&aux);
	
	while(entrar && token != NULL)	{
		/*
		length = strlen(token);
		if()
		*/
		aux1 = strchr(token,':');
		if(aux1 == NULL)	{
			
		}
		printf("Header: %s\n",token);
		
		token = strtok_r(NULL,"\r\n",&aux);
	}
	printf("--read_headers\n");
	return 0;
}

