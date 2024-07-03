#pragma once
#include <errno.h>
#include <unistd.h>
#include "util.h"
#include "conf.h"
#include "http_headers.h"
#include "http_request.h"
#include "http_serve_conf.h"
#include "http_server_def.h"

typedef struct STR_HTTP_SERVE_CLIENT {
	int client_fd;
	char *buffer;
	FILE *client;
	HTTP_REQUEST client_request;
	HTTP_HEADERS headers_to_be_send;
	int *vargp;
	char *flag_error;
	char *full_path;
	char *resolved_path;
	int offset;
	int maxrequestsize;
	int realrequestsize;
	int counter;
}*HTTP_SERVE_CLIENT;

HTTP_HEADERS default_errors;
int http_serve_enable_debug = 1;

HTTP_SERVE_CLIENT http_serve_client_init(int client_fd,int counter);
int http_serve_client_process(HTTP_SERVE_CLIENT hsc);
int http_serve_read_method(HTTP_SERVE_CLIENT hsc);
int http_serve_send_file(HTTP_SERVE_CLIENT hsc,char *path);
int http_serve_check_and_send(HTTP_SERVE_CLIENT hsc);
int http_serve_check_and_send_head(HTTP_SERVE_CLIENT hsc);
int http_serve_read_headers(HTTP_SERVE_CLIENT hsc);
int http_serve_read_post(HTTP_SERVE_CLIENT hsc);
int http_serve_default_errors();
void http_serve_client_send_error_failback(char *error,int fd);
void http_serve_client_free(HTTP_SERVE_CLIENT hsc);
void* http_serve_client_do(void *vargp);
void http_serve_info(HTTP_SERVE_CLIENT hsc);
void http_serve_client_send_error(HTTP_SERVE_CLIENT hsc);
char *http_serve_nextline(HTTP_SERVE_CLIENT hsc);

#define HTTP_ERROR_LENGTH 41
#define HTTP_CONTENT_TYPE_ARRAYLENGTH 2

#define HTTP_CONTENT_TYPE_FORM 0
#define HTTP_CONTENT_TYPE_JSON 1

char *http_serve_content_type_values[] = {"application/x-www-form-urlencoded","application/json"};

int http_serve_default_errors()	{
	char *base = "HTTP/1.1 %s\r\nServer: C HTTP Server\r\nConnection: close\r\n\r\n<html><head><title>%s</title></head><body><h1>%s</h1></body></html>\n";
	char *error_number[HTTP_ERROR_LENGTH] = {"200","400","401","402","403","404","405","406","407","408","409","410","411","412","413","414","415","416","417","418","421","422","423","424","425","426","428","429","431","451",	"500","501","502","503","504","505","506","507","508","510","511"};
	char *error_strings[HTTP_ERROR_LENGTH] = {
		"200 OK",
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
	char temp[2048];
	int i = 0;
	default_errors = http_headers_init();
	while(i < HTTP_ERROR_LENGTH)	{
		memset(temp,0,2048);
		snprintf(temp,2048,base,error_strings[i],error_strings[i],error_strings[i]);
		http_headers_add(default_errors,error_number[i],temp);
		i++;
	}
	return 0;
}

void *http_serve_client_do(void *vargp)  {
	HTTP_SERVE_CLIENT hsc;
	if(vargp != NULL)	{
		hsc = ( HTTP_SERVE_CLIENT ) vargp;
		if( http_serve_enable_debug == 1 ) { fprintf(stderr,"%i : http_serve_client_do\n",hsc->	counter);  fflush(stderr); }

	  http_serve_client_process(hsc) ;
	  http_serve_client_free(hsc);
	}
	pthread_exit(NULL);
}

void http_serve_client_send_error_failback(char *error,int fd)	{
	if( http_serve_enable_debug == 1 ) { fprintf(stderr,"http_serve_client_send_error_failback\n"); fflush(stderr); }
	char *reply = NULL;
	reply = http_headers_get_value(default_errors,error);
	if(reply != NULL)	{
		dprintf(fd,reply);
	}
	else	{
		dprintf(fd,"HTTP/1.1 500 Internal Server Error\r\n\r\n<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
	}
	pthread_exit(NULL);
}

void http_serve_client_send_error(HTTP_SERVE_CLIENT hsc)	{
	//printf("Saliendo %i : offset %i\n",hsc->counter,hsc->offset);
	char *reply =NULL;
	if(hsc != NULL)	{
		if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_client_send_error %s\n",hsc->counter,hsc->flag_error);
		reply = http_headers_get_value(default_errors,hsc->flag_error);
		if(reply != NULL)	{
			dprintf(hsc->client_fd,reply);
		}
		else	{
			dprintf(hsc->client_fd,"HTTP/1.1 500 Internal Server Error\r\n\r\n<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>");
		}
	}
	http_serve_client_free(hsc);
	//status();
	pthread_exit(NULL);
}

HTTP_SERVE_CLIENT http_serve_client_init(int client_fd,int counter)  {
	if(http_serve_enable_debug == 1 ) { fprintf(stderr,"%i : http_serve_client_init\n",counter); fflush(stderr); }
	HTTP_SERVE_CLIENT hsc = (HTTP_SERVE_CLIENT) calloc(1,sizeof(struct STR_HTTP_SERVE_CLIENT));
	if(hsc == NULL)  {
		perror("calloc");
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
	hsc->full_path = (char*) malloc(PATH_MAX);
	hsc->resolved_path = (char*) malloc(PATH_MAX);
	hsc->buffer = (char*) malloc(server_config->maxrequestsize);
	if(hsc->buffer == NULL || hsc->full_path == NULL || hsc->resolved_path == NULL)	{
		perror("malloc");
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
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_read_method\n",hsc->counter);
	int ret = 0,s;
	char *token,*aux = NULL,*line;
	char *http_tokens[3],*http_token,*http_aux = NULL;
	int i = 0;	/*	Token number*/
	line = http_serve_nextline(hsc);
	if(line != NULL)	{
		token = line;
		http_token = strtok_r(token," ",&http_aux);
		while(i < 3 && http_token  != NULL)	{
			http_tokens[i] = http_token;
			http_token = strtok_r(NULL," ",&http_aux);
			i++;
		}
		if(i != 3)	{
			hsc->flag_error = "400";
			ret = -1;
		}
		else	{
			s = http_request_set_method(hsc->client_request,http_tokens[0]);
			if(s != 0	){
				hsc->flag_error = "405";
				ret = -1;
			}
			else{
				s = http_request_set_uri(hsc->client_request,http_tokens[1]);
				if(s != 0	){
					hsc->flag_error = "500";
					ret = -1;
				}
				//fprintf(stderr,"thread %i path: %s\n",hsc->counter,http_tokens[1]);
				s = http_request_set_version(hsc->client_request,http_tokens[2]);
				if(s != 0	){
					hsc->flag_error = "405";
					ret = -1;
				}
			}
		}
		free(line);
	}
	else	{
			hsc->flag_error = "400";
			ret = -1;
	}
	return ret;
}

int http_serve_client_process(HTTP_SERVE_CLIENT hsc)  {
	char *temp;
	int index;
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_client_process\n",hsc->counter);
  int s  = 0;
  s = http_serve_read_method(hsc);
  if(s != 0 || hsc->client_request->method == 0)	{
		fprintf(stderr,"error http_serve_read_method\n");
    http_serve_client_send_error(hsc);
  }
  s = http_serve_read_headers(hsc);
  if(s != 0)	{
    http_serve_client_send_error(hsc);
  }
	fprintf(stderr,"thread %i path: %s\n",hsc->counter,hsc->client_request->uri);
	switch(hsc->client_request->method)	{
		case HEAD:
			s = http_serve_check_and_send_head(hsc);
		  if(s != 0)	{
		    http_serve_client_send_error(hsc);
		  }
		break;
		case GET:
			s = http_serve_check_and_send(hsc);
		  if(s != 0)	{
		    http_serve_client_send_error(hsc);
		  }
		break;
		case POST:
			index = index_of(urls,BACKEND_LENGTH,hsc->client_request->uri);
			if(index != -1)	{ 
				s = http_serve_read_post(hsc);
				if(s != 0)	{
					http_serve_client_send_error(hsc);
				}
				temp = functions[index](NULL,hsc->client_request->_GET,hsc->client_request->_POST,hsc->client_request->_HEADERS,NULL);
				hsc->flag_error = "200";
				http_serve_client_send_error(hsc);
			}
			else	{
				hsc->flag_error = "404";
				http_serve_client_send_error(hsc);
			}
		break;
		default:
			hsc->flag_error = "405";
			http_serve_client_send_error(hsc);
		break;
	}
	return s;
}


int http_serve_check_and_send_head(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_check_and_send\n",hsc->counter);
	int ret = 0;
	int n = 0;
	n = snprintf(hsc->full_path,4096,"%s%s",server_config->path,hsc->client_request->uri);
	realpath(hsc->full_path,hsc->resolved_path);
	if(strncmp(hsc->resolved_path,server_config->path,server_config->pathlength) == 0)	{
		/*
		resolved_path is in side of server_config->path
		*/
		if(is_regular_file(hsc->resolved_path)){
			dprintf(hsc->client_fd,"HTTP/1.1 200 OK\r\n");
			dprintf(hsc->client_fd,"Server: C HTTP Server\r\n");
			dprintf(hsc->client_fd,"Content-Length: %li\r\n",fsize(hsc->resolved_path));
			dprintf(hsc->client_fd,"Connection: close\r\n");
			dprintf(hsc->client_fd,"\r\n");
		}
		else{
			if(is_directory(hsc->resolved_path))	{
				hsc->flag_error = "403";
				ret = -1;
			}
			else	{
				hsc->flag_error = "404";
				ret = -1;
			}
		}
	}
	else	{
		/*
		resolved_path is out side of server_config->path maybe ../.,/.. directory transversal
		*/
		hsc->flag_error = "410";
		ret = -1;
	}
	//status();
	return ret;
}


int http_serve_check_and_send(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_check_and_send\n",hsc->counter);
	int ret = 0;
	int n = 0;
	n = snprintf(hsc->full_path,4096,"%s%s",server_config->path,hsc->client_request->uri);
	realpath(hsc->full_path,hsc->resolved_path);
	if(strncmp(hsc->resolved_path,server_config->path,server_config->pathlength) == 0)	{
		/*
		resolved_path is in side of server_config->path
		*/
		if(is_regular_file(hsc->resolved_path)){
			dprintf(hsc->client_fd,"HTTP/1.1 200 OK\r\n");
			dprintf(hsc->client_fd,"Server: C HTTP Server\r\n");
			dprintf(hsc->client_fd,"Content-Length: %li\r\n",fsize(hsc->resolved_path));
			dprintf(hsc->client_fd,"Connection: close\r\n");
			dprintf(hsc->client_fd,"\r\n");
			http_serve_send_file(hsc,hsc->resolved_path);
		}
		else{
			if(is_directory(hsc->resolved_path))	{

				hsc->flag_error = "403";
				ret = -1;
			}
			else	{

				hsc->flag_error = "404";
				ret = -1;
			}
		}
	}
	else	{
		/*
		resolved_path is out side of server_config->path maybe ../.,/.. directory transversal
		*/
		hsc->flag_error = "410";
		ret = -1;
	}
	//status();
	return ret;
}

int http_serve_read_headers(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_read_headers\n",hsc->counter);
  int ret = 0;
	char *token,*token1, *aux = NULL,*aux1 =NULL;
	int length, entrar = 1;
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
        ret = -1;
			}
			else	{
				//printf("Resta: %i\n",aux1 - token);
				//printf("length: %i\n",length);
				if(aux1 - token == 0)	{
					/*
					There is no Key, key length = 0 example:
					: Value
					*/
					ret = -1;
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
		        ret = -1;
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
		}
		else	{
			/*
			this is  length	== 0
			means and empty line or end of client stream was reached
			*/
			entrar = 0;
		}
		/*
			Always free the token even with length == 0
			because nextline always return a valid pointer
			while there is more data avaible in his ptr param
		*/
		free(token);
	}
	return ret;
}

void http_serve_client_free(HTTP_SERVE_CLIENT hsc)  {
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_client_free\n",hsc->counter);
  if(hsc != NULL) {
    if(hsc->buffer != NULL) {
      free(hsc->buffer);
    }
		if(hsc->full_path != NULL) {
      free(hsc->full_path);
    }
		if(hsc->resolved_path != NULL) {
			free(hsc->resolved_path);
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
		hsc->vargp[3] = 0;
    free(hsc);
  }
}

int http_serve_send_file(HTTP_SERVE_CLIENT hsc,char *path){
	if(http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_send_file\n",hsc->counter);
	FILE *fd = NULL;
	char buffer[128];
	int length = 0,readed = 0, sended = 0;
	fd = fopen(path,"rb");
	if(fd != NULL)	{
		while(sended >= 0  && readed >= 0 && !feof(fd)  )	{
			hsc->vargp[2] = server_config->timeout;
			readed = fread(buffer,1,128,fd);
			if(readed > 0)
				sended = send(hsc->client_fd,buffer,readed,0);
		}
		if(sended ==  -1)	{
			perror("send");
		}
		if(readed == -1)	{
			perror("fread");
		}
		fclose(fd);
	}
	else	{
		fprintf(stderr,"fopen");
		perror("fopen()");
	}
	return 0;
}

char *http_serve_nextbytes(HTTP_SERVE_CLIENT hsc,int bytes)	{
	if( http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_nextline\n",hsc->counter);
	int readed;
	char *temp = NULL;
	readed = fread(hsc->buffer + hsc->offset,1,bytes,hsc->client);
	if(readed > 0)	{
		temp = malloc(readed+1);
		memcpy(temp,hsc->buffer + hsc->offset,readed);
		temp[readed] = '\0';
		hsc->offset += readed;
	}
	printf("Readed: %i\n",readed);
	return temp;
}

char *http_serve_nextline(HTTP_SERVE_CLIENT hsc)	{
	if(hsc == NULL)	{
		return NULL;
	}
	if( http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_nextline\n",hsc->counter);
	int line_length;
	char *temp = NULL,*line;
	int toread = server_config->maxlinerequestsize - hsc->offset;
	if(toread > 0)	{
		fprintf(stderr,"Reading information from\nbuffer %p:%s\noffset %i\nbytes to read : %i\n",hsc->buffer,hsc->buffer,hsc->offset,toread);
		fflush(stderr);
		temp = fgets(hsc->buffer + hsc->offset, toread , hsc->client);
		if( temp == (hsc->buffer + hsc->offset) )	{
			line_length = strlen(temp);
			hsc->offset += line_length;
			line = (char*) calloc(1,line_length+1);
			strncpy(line,temp,line_length);
			trim(line,"\r\n");
			temp = line;
		}
	}
	return temp;
}

void http_serve_info(HTTP_SERVE_CLIENT hsc) {
	if( http_serve_enable_debug == 1 ) fprintf(stderr,"%i : http_serve_info\n",hsc->counter);
  printf("client_fd %i\n",hsc->client_fd);
  if(hsc->buffer)
    printf("buffer %p : %s\n",hsc->buffer,hsc->buffer);
  http_request_info(hsc->client_request);
  http_headers_info(hsc->headers_to_be_send);
  printf("offset %i\n",hsc->offset);
  printf("flag_error %s\n",hsc->flag_error);
}

int http_serve_read_post(HTTP_SERVE_CLIENT hsc)	{
	char *aux,*post;
	int content_length,i,encontrado;
	aux = http_headers_get_value(hsc->client_request->_HEADERS,"Content-Length");
	if( aux == NULL )	{
		hsc->flag_error = "411";
		return -1;
	}
	content_length = strtol(aux,NULL,10);
	printf("Content-Length: %i\n",content_length);
	aux = http_headers_get_value(hsc->client_request->_HEADERS,"Content-Type");
	if( aux == NULL )	{
		hsc->flag_error = "400";
		return -1;
	}
	i = 0;
	encontrado = 0;
	while(!encontrado && i < HTTP_CONTENT_TYPE_ARRAYLENGTH)	{
		if(strcmp(aux,http_serve_content_type_values[i]) == 0)	{
			encontrado = 1;
			i--;
		}
		i++;
	}
	if(encontrado != 1)	{
		hsc->flag_error = "400";
		return -1;
	}
	switch(i)	{
		case HTTP_CONTENT_TYPE_FORM:
			post = http_serve_nextbytes(hsc,content_length);
			if(post != NULL)	{
				http_request_set_POST_values(hsc->client_request,post);
				free(post);
			}
			else	{
				hsc->flag_error = "400";
				return -1;
			}
		break;
		case HTTP_CONTENT_TYPE_JSON:
			hsc->flag_error = "400";
			return -1;
		break;
		default:
			hsc->flag_error = "400";
			return -1;
		break;
	}
	return 0;
}
