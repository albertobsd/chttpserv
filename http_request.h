#pragma once
#include "http_headers.h"
#include "http_serve_conf.h"

#define UNKNOW	0
#define GET		1
#define POST	2
#define HEAD	3
#define METHODS_LENGTH 4

#define HTTP1	1
#define HTTP11	2
#define VERSIONS_LENGTH 3

typedef struct STR_HTTP_REQUEST {
	int method;
	char *uri;
	int version;
	HTTP_HEADERS _HEADERS;
	HTTP_HEADERS _GET;
	HTTP_HEADERS _POST;
}*HTTP_REQUEST;

//	extern HTTP_SERVE_CONF server_config;


HTTP_REQUEST http_request_init();
int http_request_set_method(HTTP_REQUEST hr,char *method);
int http_request_set_version(HTTP_REQUEST hr,char *version);
int http_request_set_uri(HTTP_REQUEST hr,char *uri);
int http_request_set_GET_values(HTTP_REQUEST hr,char *get_params);
void http_request_free(HTTP_REQUEST hr);
void http_request_info(HTTP_REQUEST hr);

char *http_request_supported_method[METHODS_LENGTH] = {"","GET","POST","HEAD"};
char *http_request_supported_version[VERSIONS_LENGTH] = {"","HTTP/1.0","HTTP/1.1"};

int http_request_enable_debug = 0;

HTTP_REQUEST http_request_init()	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_init\n");
	HTTP_REQUEST hr = NULL;
	hr = (HTTP_REQUEST) calloc(1,sizeof(struct STR_HTTP_REQUEST));
	if(hr == NULL)	{
		fprintf(stderr,"calloc\n");
		perror("calloc");
		exit(-1);
	}
	else	{
		hr->_HEADERS = http_headers_init();
		hr->_GET = http_headers_init();
		hr->_POST = http_headers_init();
	}
	return hr;
}

int http_request_set_method(HTTP_REQUEST hr,char *method)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_set_method\n");
	int  i = 1, entrar = 1;
	if(hr == NULL || method == NULL)	{
		fprintf(stderr,"http_request_set_method: some params are NULL\n");
		return 1;
	}
	while(entrar && i < METHODS_LENGTH)	{
		if(strcmp(http_request_supported_method[i],method) == 0)	{
			entrar = 0;
			i--;
		}
		i++;
	}
	if(entrar)	{
		return 2;
	}
	if( http_request_enable_debug == 1 ) fprintf(stderr,"Method: %s\n",http_request_supported_version[i]);
	hr->method = i;
	return 0;
}

int http_request_set_version(HTTP_REQUEST hr,char *version)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_set_version : %s\n",version);
	int  i = 1, entrar = 1,ret = 0;
	if(hr == NULL || version == NULL)	{
		fprintf(stderr,"http_request_set_version: some params are NULL\n");
		ret = -1;
	}
	else	{
		while(entrar && i < VERSIONS_LENGTH)	{
			if(strcmp(http_request_supported_version[i],version) == 0)	{
				entrar = 0;
				i--;
			}
			i++;
		}
		if(entrar)	{
			ret = -1;
		}
		if( http_request_enable_debug == 1 ) fprintf(stderr,"Version: %s\n",http_request_supported_version[i]);
		hr->version = i;
	}
	return 0;
}

int http_request_set_uri(HTTP_REQUEST hr,char *uri)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_set_uri	\n");
	int uri_length,ret = 0;
	int qmark_offset;
	char *temp;
	char *qmark_ptr;
	if(hr == NULL || uri == NULL)	{
		fprintf(stderr,"http_request_set_version: some params are NULL\n");
		return 1;
	}
	uri_length = strlen(uri);
	qmark_ptr = strrchr(uri,'?');

	hr->uri = (char*) malloc(server_config->maxlinerequestsize);
	if(hr->uri == NULL)	{
		fprintf(stderr,"malloc\n");
		perror("malloc");
		ret = -1;
	}
	else	{
		if(qmark_ptr != NULL)	{
			qmark_offset = qmark_ptr - uri;
			memcpy(hr->uri,uri,qmark_offset);
			hr->uri[qmark_offset] = '\0';
			temp = realloc(hr->uri,qmark_offset+1);
			if(temp == NULL)	{
				fprintf(stderr,"realloc\n");
				perror("realloc");
				exit(-1);
			}
			hr->uri = temp;
			ret =  http_request_set_GET_values(hr,uri+qmark_offset+1);
		}
		else	{
			if(strcmp(uri,"/") == 0){
				strcpy(hr->uri,"/index.html");
				hr->uri[11] = '\0';
				temp = realloc(hr->uri,12);
				if(temp == NULL)	{
					fprintf(stderr,"realloc\n");
					perror("realloc");
					exit(-1);
				}
				hr->uri = temp;
			}
			else	{
				memcpy(hr->uri,uri,uri_length);
				hr->uri[uri_length] = '\0';
				temp = realloc(hr->uri,uri_length+1);
				if(temp == NULL)	{
					fprintf(stderr,"realloc\n");
					perror("realloc");
					exit(-1);
				}
				hr->uri = temp;
			}
		}
	}
	if( http_request_enable_debug == 1 ) fprintf(stderr,"URI: %s\n",hr->uri);
	return ret;
}

int http_request_set_GET_values(HTTP_REQUEST hr,char *get_params)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_set_GET_values\n");
	/*
	int key_length = 0;
	int value_length = 0;
	*/
	char *aux = NULL,*pivote = NULL, *pivote1 = NULL;
	if(hr == NULL || get_params == NULL)	{
		fprintf(stderr,"http_request_set_version: some params are NULL\n");
		return 1;
	}
	pivote = (char *) strtok_r(get_params,"&",&aux);
	while(pivote != NULL)	{	//while there are no more '&'
		pivote1 = strchr(pivote,'=');
		if(pivote1 != NULL){
			if(pivote1 != pivote)	{
				pivote1[0] = '\0';
				http_headers_add(hr->_GET,pivote,pivote1+1);
			}
			else	{
				/*
				There is no Key so we cant add anything to the POST map
				*/
			}
		}
		else	{	//There is no  '=' sign , add the variable with empty value ""
			http_headers_add(hr->_GET,pivote,"");
		}
		pivote = (char *)strtok_r(NULL,"& ",&aux);
	}
	return 0;
}

int http_request_set_POST_values(HTTP_REQUEST hr,char *post_params)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_set_POST_values\n");
	int key_length = 0;
	int value_length = 0;
	int param_length = 0;
	char *aux = NULL,*pivote = NULL, *pivote1 = NULL,*value_urldecode = NULL;
	if(hr == NULL || post_params == NULL)	{
		fprintf(stderr,"http_request_set_version: some params are NULL\n");
		return 1;
	}
	pivote = (char *) strtok_r(post_params,"&",&aux);
	while(pivote != NULL)	{	//while there are no more '&'
		param_length = strlen(pivote);
		pivote1 = strchr(pivote,'=');
		if(pivote1 != NULL){
			key_length = pivote1 - pivote;
			if(pivote1 != pivote){
				pivote1[0] = '\0';
				value_length = strlen(pivote1+1);
				value_urldecode = malloc(value_length*3 + 1);
				if( value_urldecode  != NULL && url_decode(pivote1+1,value_urldecode) > 0 ){
					printf("Adding: %s =  %s\n",pivote,value_urldecode);
					http_headers_add(hr->_POST,pivote,value_urldecode);
					free(value_urldecode);
				}
			}
			else	{
				/*
				There is no Key so we cant add anything to the POST map
				*/
			}
		}
		else	{	//There is no  '=' sign , add the variable with empty value ""
		printf("Adding: %s =  \"\"\n",pivote);
			http_headers_add(hr->_POST,pivote,"");
		}
		pivote = (char *)strtok_r(NULL,"& ",&aux);
	}
	return 0;
}


void http_request_free(HTTP_REQUEST hr)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_free\n");
	if(hr != NULL)	{
		if(hr->uri != NULL)	{
			free(hr->uri);
		}
		if(hr->_GET != NULL)	{
			http_headers_free(hr->_GET);
		}
		if(hr->_HEADERS != NULL)	{
			http_headers_free(hr->_HEADERS);
		}
		if(hr->_POST != NULL)	{
			http_headers_free(hr->_POST);
		}
		free(hr);
	}
}

void http_request_info(HTTP_REQUEST hr)	{
	if( http_request_enable_debug == 1 ) fprintf(stderr,"http_request_info\n");
	if(hr != NULL)	{
		printf("method %i\n",hr->method);
		printf("uri %s\n",hr->uri);
		printf("version %i\n",hr->version);
		http_headers_info(hr->_HEADERS);
		http_headers_info(hr->_GET);
		http_headers_info(hr->_POST);
	}
}
