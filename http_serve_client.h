#pragma once
#include <errno.h>
#include <unistd.h>
#include "util.h"
#include "conf.h"
#include "http_headers.h"
#include "http_request.h"
#include "http_serve_conf.h"
#include "debug.h"

typedef struct STR_HTTP_SERVE_CLIENT {
	int client_fd;
	FILE *client;
  char *buffer;
	HTTP_REQUEST client_request;
  HTTP_HEADERS headers_to_be_send;
  char *flag_error;
  int offset;
  int maxrequestsize;
  int realrequestsize;
	int counter;
}*HTTP_SERVE_CLIENT;

HTTP_HEADERS default_errors;
int http_serve_enable_debug = 0;

HTTP_SERVE_CLIENT http_serve_client_init(int client_fd,int counter);
int http_serve_client_process(HTTP_SERVE_CLIENT hsc);
int http_serve_read_method(HTTP_SERVE_CLIENT hsc);
int http_serve_send_file(HTTP_SERVE_CLIENT hsc,char *path);
int http_serve_check_and_send(HTTP_SERVE_CLIENT hsc);
int http_serve_read_headers(HTTP_SERVE_CLIENT hsc);
int http_serve_default_errors();
void http_serve_client_send_error_failback(char *error,int fd);
void http_serve_client_free(HTTP_SERVE_CLIENT hsc);
void http_serve_client_do(int client_fd,int counter);
void http_serve_info(HTTP_SERVE_CLIENT hsc);
void http_serve_client_send_error(HTTP_SERVE_CLIENT hsc);
char *http_serve_nextline(HTTP_SERVE_CLIENT hsc);

#define HTTP_ERROR_LENGTH 40

int http_serve_default_errors()	{
	char *base = "HTTP/1.1 %s\r\nConnection: close\r\n\r\n<html><head><title>%s</title></head><body><h1>%s</h1></body></html>\n";
	char *error_number[HTTP_ERROR_LENGTH] = {"400","401","402","403","404","405","406","407","408","409","410","411","412","413","414","415","416","417","418","421","422","423","424","425","426","428","429","431","451",	"500","501","502","503","504","505","506","507","508","510","511"};
	char *error_strings[HTTP_ERROR_LENGTH] = {
		"400 Bad Request",
		"401 Unauthorized",
		"402 Payment Required",
		"403 Forbidden",
		"404 Not Found",
		"405 Method Not Allowed",
		"406 Not Acceptable",
		"407 Proxy Authentication Required",
		"408 Request Timeout",
		"409 Conflict",
		"410 Gone",
		"411 Length Required",
		"412 Precondition Failed",
		"413 Payload Too Large",
		"414 URI Too Long",
		"415 Unsupported Media Type",
		"416 Range Not Satisfiable",
		"417 Expectation Failed",
		"418 I'm a teapot",
		"421 Misdirected Request",
		"422 Unprocessable Entity",
		"423 Locked",
		"424 Failed Dependency",
		"425 Too Early",
		"426 Upgrade Required",
		"428 Precondition Required",
		"429 Too Many Requests",
		"431 Request Header Fields Too Large",
		"451 Unavailable For Legal Reasons",
		"500 Internal Server Error",
		"501 Not Implemented",
		"502 Bad Gateway",
		"503 Service Unavailable",
		"504 Gateway Timeout",
		"505 HTTP Version Not Supported",
		"506 Variant Also Negotiates",
		"507 Insufficient Storage",
		"508 Loop Detected",
		"510 Not Extended",
		"511 Network Authentication Required"};
	char temp[1024];
	int i = 0;
	default_errors = http_headers_init();
	while(i < HTTP_ERROR_LENGTH)	{
		memset(temp,0,1024);
		snprintf(temp,1024,base,error_strings[i],error_strings[i],error_strings[i]);
		http_headers_add(default_errors,error_number[i],temp);
		i++;
	}
}

void http_serve_client_do(int client_fd,int counter)  {
	if( http_serve_enable_debug == 1 ) printf("%i : http_serve_client_do\n",counter);
  HTTP_SERVE_CLIENT hsc = http_serve_client_init( client_fd,counter);
  http_serve_client_process(hsc) ;
  http_serve_client_free(hsc);
}

void http_serve_client_send_error_failback(char *error,int fd)	{
	if( http_serve_enable_debug == 1 ) printf("%i : http_serve_client_send_error_failback\n");
	char *reply = NULL;
	reply = http_headers_get_value(default_errors,error);
	if(reply != NULL)	{
		dprintf(fd,reply);
	}
	else	{
		dprintf(fd,"HTTP/1.1 500 Internal Server Error\r\n\r\n<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
	}
	debug_status();
	pthread_exit(NULL);
}

void http_serve_client_send_error(HTTP_SERVE_CLIENT hsc)	{
	if( http_serve_enable_debug == 1 ) printf("%i : http_serve_client_send_error\n",hsc->counter);
	char *reply =NULL;
	if(hsc != NULL)	{
		if(http_serve_enable_debug == 1 ) printf("%i : http_serve_client_send_error %s\n",hsc->counter,hsc->flag_error);
		reply = http_headers_get_value(default_errors,hsc->flag_error);
		if(reply != NULL)	{
			dprintf(hsc->client_fd,reply);
		}
		else	{
			dprintf(hsc->client_fd,"HTTP/1.1 500 Internal Server Error\r\n\r\n<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
		}
	}
	http_serve_client_free(hsc);
	debug_status();
	printf("Saliendo 2\n");
	pthread_exit(NULL);
}

HTTP_SERVE_CLIENT http_serve_client_init(int client_fd,int counter)  {
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_client_init\n",counter);
  HTTP_SERVE_CLIENT hsc = debug_calloc(1,sizeof(struct STR_HTTP_SERVE_CLIENT));
  if(hsc == NULL)  {
		perror("debug_calloc");
		http_serve_client_send_error_failback("500",client_fd);
  }
  hsc->client_fd = client_fd;
	hsc->client = fdopen(hsc->client_fd,"rb+");
	hsc->counter = counter;
  hsc->client_request = http_request_init();
	if(hsc->client_request == NULL)	{
		hsc->flag_error = "500";
		http_serve_client_send_error(hsc);
	}
  hsc->headers_to_be_send = http_headers_init();
	if(hsc->headers_to_be_send == NULL)	{
		hsc->flag_error = "500";
		http_serve_client_send_error(hsc);
	}

  hsc->buffer = (char*) debug_malloc(server_config->maxrequestsize);
  if(hsc->buffer == NULL)	{
    perror("debug_malloc");
		hsc->flag_error = "500";
    http_serve_client_send_error(hsc);
  }
  return hsc;
}

/*
 *	http_serve_read_method reads the firts line of the buffer
 *	3 tokens separate by spaces are expected
 */
int http_serve_read_method(HTTP_SERVE_CLIENT hsc) {
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_read_method\n",hsc->counter);
	int ret = 0,s;
  char *token,*aux = NULL,*line;
	char *http_tokens[3],*http_token,*http_aux = NULL;
	int i = 0;	/*	Token number*/
	line = http_serve_nextline(hsc);
	token = line;
	http_token = strtok_r(token," ",&http_aux);
	while(i < 3 && http_token  != NULL)	{
		http_tokens[i] = http_token;
		http_token = strtok_r(NULL," ",&http_aux);
		i++;
	}
	if(i != 3)	{
		hsc->flag_error = "400";
		ret = 1;
	}
	else	{
		s = http_request_set_method(hsc->client_request,http_tokens[0]);
		//printf("hsc->client_request->method: %i\n",hsc->client_request->method);
		if(s != 0	){
			hsc->flag_error = "405";
			ret = 1;
		}
		else{
			s = http_request_set_uri(hsc->client_request,http_tokens[1]);
			if(s != 0	){
				hsc->flag_error = "500";
				ret = 1;
			}
			s = http_request_set_version(hsc->client_request,http_tokens[2]);
			if(s != 0	){
				hsc->flag_error = "405";
				ret = 1;
			}
		}
	}
	debug_free(line);
	return ret;
}

int http_serve_client_process(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_client_process\n",hsc->counter);
  int s ;
  s = http_serve_read_method(hsc);
  if(s != 0 || hsc->client_request->method == 0)	{
    http_serve_client_send_error(hsc);
  }
  s = http_serve_read_headers(hsc);
  if(s != 0)	{
    http_serve_client_send_error(hsc);
  }
	hsc->buffer = debug_realloc(hsc->buffer,hsc->offset +1);
  s = http_serve_check_and_send(hsc);
  if(s != 0)	{
    http_serve_client_send_error(hsc);
  }
}

int http_serve_check_and_send(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_check_and_send\n",hsc->counter);
	int ret = 0;
  char *resolved_path = debug_malloc(PATH_MAX);
	char *full_path = debug_malloc(PATH_MAX);
	int n = 0;
	n = snprintf(full_path,4096,"%s%s",server_config->path,hsc->client_request->uri);
	if(n >= 0)	{
		full_path = debug_realloc(full_path,n +2 );
	}
	realpath(full_path,resolved_path);
	//printf("Archivo solicitado: %s\n",full_path);
	//printf("Real path: %s\n",resolved_path	);
	if(strncmp(resolved_path,server_config->path,server_config->pathlength) == 0)	{
		if(is_regular_file(resolved_path)){
			//printf("OK : is_regular_file\n");
			dprintf(hsc->client_fd,"HTTP/1.1 200 OK\r\n");
			dprintf(hsc->client_fd,"Content-Length: %i\r\n",fsize(resolved_path));
			dprintf(hsc->client_fd,"Connection: close\r\n");
			dprintf(hsc->client_fd,"\r\n");
			http_serve_send_file(hsc,resolved_path);
		}
		else{
			if(is_directory(resolved_path))	{
				hsc->flag_error = "403";
				ret = 1;
			}
			else	{
				hsc->flag_error = "404";
				ret = 1;
			}
		}
	}
	else	{
		printf("resolved_path = [%s] outside of _CONFIG[PATH] = [%s]\n",resolved_path,server_config->path);
		hsc->flag_error = "410";
		ret = 1;
	}
	debug_status();
	debug_free(full_path);
	debug_free(resolved_path);
	return ret;
}

int http_serve_read_headers(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_read_headers\n",hsc->counter);
  int ret = 0;
	char *token,*token1, *aux = NULL,*aux1 =NULL;
	int length, entrar = 1;
	if(hsc->client_request == NULL )	{
		hsc->flag_error = "500";
		ret = 1;
	}
	else	{
		while(ret == 0 && entrar && (token = http_serve_nextline(hsc)) != NULL)	{
			length = strlen(token);
			if(length > 4)	{
				/*
					length > 4 is the minimun line length that match key:<space>value valid with key and value of one byte
					example:
					A: B\0
					C: D\0
				*/
				//printf("Validando header : %s\n",token);
				aux1 = strchr(token,':');
				if(aux1 == NULL)	{
					/*
						Error there is no :
						Example:
						bar: foo 	<- OK
						bar 			<- This case
						bar: foo1 <- OK
					*/
					//printf("No ':' detected\n");
					hsc->flag_error = "400";
	        ret = 1;
				}
				else	{
					//printf("Resta: %i\n",aux1 - token);
					//printf("length: %i\n",length);
					if(aux1 - token == 0)	{
						/*
						There is no Key, key length = 0 example:
						: Value
						*/
						ret = 1;
						hsc->flag_error = "400";
					}
					else	{
						if(aux1 - token + 2 >= length)	{
							/*
									There is nothing after ':'
									Example:
									bar: foo\r\n	<- OK
									bor:\r\n	<-	This case
									bor: \r\n	<-	Also this case
							*/
							//printf("Empty value header\n");
							hsc->flag_error = "400";
			        ret = 1;
						}
						else	{
							/*
								bar: foo\r\n	<- OK
							*/
							//printf("Header correcto\n");
							aux1[0] = '\0';
							http_headers_add(hsc->client_request->_HEADERS,token,aux1+2);
						}
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
	}
	return ret;
}

void http_serve_client_free(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_client_free\n",hsc->counter);
  if(hsc != NULL) {
    if(hsc->buffer != NULL) {
			//printf("Buffer: procesado\n%s\n",hsc->buffer);
      debug_free(hsc->buffer);
    }
    if(hsc->client_request != NULL )  {
      http_request_free(hsc->client_request);
    }
    if(hsc->headers_to_be_send != NULL) {
      http_headers_free(hsc->headers_to_be_send);
    }
		if(hsc->client != NULL)	{
			fclose(hsc->client);
		}
	  shutdown(hsc->client_fd,SHUT_RDWR);
		close(hsc->client_fd);
    debug_free(hsc);
  }
}

int http_serve_send_file(HTTP_SERVE_CLIENT hsc,char *path){
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_send_file\n",hsc->counter);
	FILE *fd = NULL;
	char buffer[128];
	int length = 0,readed = 0, sended = 0;
	fd = fopen(path,"rb");
	if(fd != NULL)	{
		while(sended >= 0  && readed >= 0 && !feof(fd)  )	{
			readed = fread(buffer,1,128,fd);
			sended = send(hsc->client_fd,buffer,readed,0);
		}
		if(sended ==  -1)	{
			perror("send");
		}
		fclose(fd);
	}
	else	{
		perror("fopen()");
	}
	return 0;
}

char *http_serve_nextline(HTTP_SERVE_CLIENT hsc)	{
	if( http_serve_enable_debug == 1 ) printf("%i : http_serve_nextline\n",hsc->counter);
	int line_length;
	char *temp = NULL,*line;
	if(http_serve_enable_debug == 1 ) printf("%i : http_serve_nextline\n",hsc->counter);
	temp = fgets(hsc->buffer + hsc->offset,server_config->maxlinerequestsize,hsc->client);
	if(temp == (hsc->buffer + hsc->offset))	{
		line_length = strlen(temp);
		hsc->offset += line_length;
		line = debug_calloc(1,line_length+1);
		strncpy(line,temp,line_length);
		trim(line,"\r\n");
		temp = line;
	}
	else	{
		hsc->flag_error = "400";
		http_serve_client_send_error(hsc);
	}
	return temp;
}

void http_serve_info(HTTP_SERVE_CLIENT hsc) {
	if( http_serve_enable_debug == 1 ) printf("%i : http_serve_info\n",hsc->counter);
  printf("client_fd %i\n",hsc->client_fd);
  if(hsc->buffer)
    printf("buffer %p : %s\n",hsc->buffer,hsc->buffer);
  http_request_info(hsc->client_request);
  http_headers_info(hsc->headers_to_be_send);
  printf("offset %i\n",hsc->offset);
  printf("flag_error %s\n",hsc->flag_error);
  //printf("maxrequestsize %i\n",hsc->maxrequestsize);
  //printf("realrequestsize %i\n",hsc->realrequestsize);
}
