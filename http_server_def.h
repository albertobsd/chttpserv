#pragma once
#include "http_headers.h"

typedef char* (*TWrapper)(HTTP_HEADERS,HTTP_HEADERS,HTTP_HEADERS,HTTP_HEADERS,HTTP_HEADERS);
/*
	User defined functions
*/
char *cfi_data(HTTP_HEADERS _SERVER,HTTP_HEADERS _GET,HTTP_HEADERS _POST,HTTP_HEADERS _HEADERS,HTTP_HEADERS _HEADERS_REPLY);
/*
	Backend map
*/
#define BACKEND_LENGTH 1

char *urls[BACKEND_LENGTH] = {"/data"};
TWrapper functions[BACKEND_LENGTH] = {&cfi_data};

char *cfi_data(HTTP_HEADERS _SERVER,HTTP_HEADERS _GET,HTTP_HEADERS _POST,HTTP_HEADERS _HEADERS,HTTP_HEADERS _HEADERS_REPLY)	{
	char *buffer = NULL;
	printf("Entrando a cfi_data\n");
	if(http_headers_isset(_POST,"param1"))	{	
		printf("Si existe param1: %s\n",http_headers_get_value(_POST,"param1"));
	}
	return buffer;
}
