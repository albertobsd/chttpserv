#pragma once
#include "util.h"
#include "conf.h"
#include "http_headers.h"
#include "http_request.h"
#include "debug.h"

typedef struct STR_HTTP_SERVE_CLIENT {
	int client_fd;
  char *buffer;
	HTTP_REQUEST client_request;
  HTTP_HEADERS headers_to_be_send;
  int offset;
  int flag_error;
  int maxrequestsize;
  int realrequestsize;
}*HTTP_SERVE_CLIENT;

HTTP_SERVE_CLIENT http_serve_client_init(int cliend_fd);
int http_serve_client_process(HTTP_SERVE_CLIENT hsc);
void http_serve_client_free(HTTP_SERVE_CLIENT hsc);
void http_serve_client_do(int cliend_fd);
int http_serve_read_buffer(HTTP_SERVE_CLIENT hsc);
int http_serve_read_method(HTTP_SERVE_CLIENT hsc);
int http_serve_send_file(HTTP_SERVE_CLIENT hsc,char *path);
int http_serve_check_and_send(HTTP_SERVE_CLIENT hsc);
int http_serve_read_headers(HTTP_SERVE_CLIENT hsc);
char *http_serve_nextline(HTTP_SERVE_CLIENT hsc);
void http_serve_info(HTTP_SERVE_CLIENT hsc);

void http_serve_client_do(int cliend_fd)  {
  printf("http_serve_client_do\n");
  HTTP_SERVE_CLIENT hsc = http_serve_client_init( cliend_fd);
  http_serve_client_process(hsc) ;
  http_serve_client_free(hsc);
}

HTTP_SERVE_CLIENT http_serve_client_init(int cliend_fd)  {
  printf("http_serve_client_init\n");
  HTTP_SERVE_CLIENT hsc = debug_calloc(1,sizeof(struct STR_HTTP_SERVE_CLIENT));
  if(hsc == NULL)  {
    perror("debug_calloc");
    exit(0);
  }
  hsc->client_fd = cliend_fd;
  hsc->client_request = http_request_init();
  hsc->headers_to_be_send = http_headers_init();
  hsc->maxrequestsize = strtol(_CONFIG[MAXREQUESTSIZE],NULL,10);
  hsc->buffer = (char*) debug_malloc(hsc->maxrequestsize+1);
  if(hsc->buffer == NULL)	{
    perror("debug_malloc");
    exit(-1);
  }
  return hsc;
}

int http_serve_read_buffer(HTTP_SERVE_CLIENT hsc) {
  printf("http_serve_read_buffer\n");
  int offset = 0,readed,toread;
	if(hsc->buffer != NULL)	{
		do	{
			toread = (offset+64 < hsc->maxrequestsize )  ? 64: hsc->maxrequestsize-offset;
			readed = recv(hsc->client_fd,hsc->buffer +offset,64,MSG_DONTWAIT);
			if(readed > 0)
				offset+= readed;
		}while(readed > 0 && offset < hsc->maxrequestsize);
	}
	return offset;
}

/*
 *	http_serve_read_method reads the firts line of the buffer
 *	3 tokens separate by spaces are expected
 */
int http_serve_read_method(HTTP_SERVE_CLIENT hsc) {
  printf("http_serve_read_method\n");
  char *token,*aux = NULL;
	char *http_tokens[3],*http_token,*http_aux = NULL;
	int i = 0;	/*	Token number*/
	int current_offset =  0;
	token = strtok_r(hsc->buffer + current_offset,"\r\n\t",&aux);
	if(token != NULL)	{
		current_offset = strlen(token) + 2;
		http_token = strtok_r(token," ",&http_aux);
		while(i < 3 && http_token  != NULL)	{
			http_tokens[i] = http_token;
			http_token = strtok_r(NULL," ",&http_aux);
			i++;
		}
		if(i != 3)	{
			return 1;
		}
		http_request_set_method(hsc->client_request,http_tokens[0]);
		http_request_set_uri(hsc->client_request,http_tokens[1]);
		http_request_set_version(hsc->client_request,http_tokens[2]);
	}
  hsc->offset = current_offset;
	return 0;
}


int http_serve_client_process(HTTP_SERVE_CLIENT hsc)  {
  printf("http_serve_client_process\n");
  int s ;
  hsc->realrequestsize = http_serve_read_buffer(hsc);
  if(hsc->realrequestsize < hsc->maxrequestsize)	{
    hsc->buffer = debug_realloc(hsc->buffer,hsc->realrequestsize+2);
    if(hsc->buffer == NULL)	{
      hsc->flag_error = 500;
    }
  }
  s = http_serve_read_method(hsc);
  if(s != 0)	{
    hsc->flag_error = 400;
  }
  s = http_serve_read_headers(hsc);
  if(s != 0)	{
    hsc->flag_error = 500;
  }
  s = http_serve_check_and_send(hsc);
  if(s != 0)	{
    hsc->flag_error = 500;
  }
  printf("END http_serve_client_process\n");
}

int http_serve_check_and_send(HTTP_SERVE_CLIENT hsc)  {
  printf("http_serve_check_and_send\n");
  char *resolved_path = debug_malloc(PATH_MAX);
	char *full_path = debug_malloc(PATH_MAX);
	int n = 0,length_dir_path;
	length_dir_path = strlen(_CONFIG[PATH]);
	n = snprintf(full_path,4096,"%s%s",_CONFIG[PATH],hsc->client_request->uri);
	if(n >= 0)	{
		full_path = debug_realloc(full_path,n +2 );
	}
	realpath(full_path,resolved_path);
	printf("Archivo solicitado: %s\n",full_path);
	printf("Real path: %s\n",resolved_path	);
	if(strncmp(resolved_path,_CONFIG[PATH],length_dir_path) == 0)	{
		if(is_regular_file(resolved_path)){
			printf("OK : is_regular_file\n");
			dprintf(hsc->client_fd,"HTTP/1.1 200 OK\r\n");
			http_serve_send_file(hsc,resolved_path);
		}
		else{
			if(is_directory(resolved_path))	{
				printf("OK : is_directory\n");
			}
			else	{
				printf("NO : is_directory || is_regular_file\n");
			}
		}
	}
	else	{
		printf("resolved_path = [%s] outside of _CONFIG[PATH] = [%s]\n",resolved_path,_CONFIG[PATH]);
	}
	debug_status();
	debug_free(full_path);
	debug_free(resolved_path);
	return 0;
}

int http_serve_read_headers(HTTP_SERVE_CLIENT hsc)  {
  printf("http_serve_read_headers\n");
  //http_serve_info(hsc);
	if(hsc->client_request == NULL|| hsc->buffer == NULL)	{
		return 1;
	}
	char *token,*token1, *aux = NULL,*aux1 =NULL;
	int length, entrar = 1;
  printf(" %i : %p\n",hsc->offset,hsc->buffer);
	while(entrar && (token = http_serve_nextline(hsc)) != NULL)	{
		length = strlen(token);
    printf("length %i\n",length);
		if(length > 0)	{
			aux1 = strchr(token,':');
			if(aux1 == NULL)	{
				/*
					Error there is no :
					Example:
					bar: foo 	<- OK
					bar 			<- This case
					bar: foo1 <- OK
				*/
        return 1;
			}
			else	{
				if(aux1 - token >= length)	{
					/*
							There is nothing after ':'
							Example:
							bar: foo\r\n	<- OK
							bor:\r\n	<-	This case
							bor: \r\n	<-	Also this case
					*/
          return 1;
				}
				else	{
					/*
						bar: foo\r\n	<- OK
					*/
          printf("Header: %s\n",token);
    			printf("Key: %s\n",aux1+2);
					aux1[0] = '\0';
					http_headers_add(hsc->client_request->_HEADERS,token,aux1+2);
				}
			}
			/*
			printf("Header: %s\n",token);
			printf("Key: %s\n",aux1+2);
			*/
		}
		else	{
			/*
			this is  length	== 0
			means and empty line or end of client stream was reached
			*/
			entrar = 0;
		}
		/*
			Always debug_free the token even with length == 0
			because nextline always return a valid pointer
			while there is more data avaible in his ptr param

		*/
		debug_free(token);
	}
	return 0;
}


void http_serve_client_free(HTTP_SERVE_CLIENT hsc)  {
  printf("http_serve_client_free\n");
  if(hsc != NULL) {
    if(hsc->buffer != NULL) {
      debug_free(hsc->buffer);
    }
    if(hsc->client_request != NULL )  {
      http_request_free(hsc->client_request);
    }
    if(hsc->headers_to_be_send != NULL) {
      http_headers_free(hsc->headers_to_be_send);
    }
    debug_free(hsc);
  }
  shutdown(hsc->client_fd,SHUT_RDWR);
  close(hsc->client_fd);
  printf("END http_serve_client_free\n");
}



int http_serve_send_file(HTTP_SERVE_CLIENT hsc,char *path){
  printf("http_serve_send_file\n");
	FILE *fd = NULL;
	char *buffer = NULL;
	int length = 0,readed = 0;
	buffer =  (char*) debug_malloc(64+1);
	if(buffer != NULL)	{
		fd = fopen(path,"rb");
		if(fd != NULL)	{
			while(!feof(fd))	{
				readed = fread(buffer,1,64,fd);
				send(hsc->client_fd,buffer,readed,0);
			}
			fclose(fd);
		}
		else	{
			perror("fopen()");
		}
		debug_free(buffer);
	}
	else	{
		perror("debug_malloc()");
	}
	return 0;
}

char *http_serve_nextline(HTTP_SERVE_CLIENT hsc)	{
  printf("http_serve_nextline\n");
	char *temp = NULL;
	int aux = hsc->offset;
	int i = hsc->offset;
	int length = 0;
	printf("%i : ptr: %s\n",hsc->offset,hsc->buffer + hsc->offset);
	while(hsc->buffer[i] != '\r' && hsc->buffer[i] != '\n' && hsc->buffer[i] != '\0')	{
		i++;
	}
	length = i - aux;
	printf("%i = %i - %i\n",length,i,aux);
	temp = debug_malloc(length + 2);
	switch(hsc->buffer[i])	{
		case '\0':
			if(length == 0)	{
				debug_free(temp);
				temp = NULL;
			}
			else	{
				memcpy(temp,hsc->buffer+hsc->offset,length);
				temp[length] = '\0';
				hsc->offset = i;
			}
		break;
		case 0xD://R
			memcpy(temp,hsc->buffer+hsc->offset,length);
			temp[length] = '\0';
			temp[i+1] = '\0';
			if(hsc->buffer[i+1] == 0xA)
				hsc->offset = i+2;
			else
			  hsc->offset = i+1;

			//printf("Linea: %s\n",temp);
		break;
		case 0xA://N
			memcpy(temp,hsc->buffer+hsc->offset,length);
			temp[length] = '\0';
			hsc->offset = i+1;
		break;
	}
	return temp;
}

void http_serve_info(HTTP_SERVE_CLIENT hsc) {
  printf("client_fd %i\n",hsc->client_fd);
  if(hsc->buffer)
    printf("buffer %p : %s\n",hsc->buffer,hsc->buffer);
  http_request_info(hsc->client_request);
  http_headers_info(hsc->headers_to_be_send);
  printf("offset %i\n",hsc->offset);
  printf("flag_error %i\n",hsc->flag_error);
  printf("maxrequestsize %i\n",hsc->maxrequestsize);
  printf("realrequestsize %i\n",hsc->realrequestsize);
}
