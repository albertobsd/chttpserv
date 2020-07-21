#pragma once

typedef struct STR_HTTP_HEADERS {
	int max_length;
	int length;
	char **keys;
	char **values;
}*HTTP_HEADERS;


HTTP_HEADERS http_headers_init();
void http_headers_add(HTTP_HEADERS hh,char *key,char *value);
char *http_headers_get_value(HTTP_HEADERS hh, char *key);
void http_headers_set_value_index(HTTP_HEADERS hh,int index, char *value);
void http_headers_increment_max_length(HTTP_HEADERS hh);
void http_headers_free(HTTP_HEADERS hh);

HTTP_HEADERS http_headers_init()	{
	HTTP_HEADERS hh = NULL;
	hh = (HTTP_HEADERS) calloc(1,sizeof(struct STR_HTTP_HEADERS));
	if(hh == NULL)	{
		perror("calloc");
		exit(101);
	}
	hh->length = 0;
	hh->max_length = 2;
	hh->keys = (char **) calloc(hh->max_length,sizeof(char**));
	hh->values = (char **) calloc(hh->max_length,sizeof(char**));
	if(hh->keys == NULL || hh->values ==NULL )	{
		perror("calloc");
		exit(102);
	}
	return hh;
}

/*
 *	http_headers_add add add new <key,value> to hh
 *  if key already exist, his current value is going to be overwriting
 *  the current params key,value are copied in internal struct and are no longer need you can FREE them without worries 
 */
void http_headers_add(HTTP_HEADERS hh,char *key,char *value)	{
	int i = 0,entrar  = 1;
	int key_length = 0;
	int value_length = 0;
	if(hh == NULL || key == NULL || value == NULL)	{
		fprintf(stderr,"Some value is NULL\n");
		exit(102);
	}
	if(hh->length == hh->max_length)	{
		http_headers_increment_max_length(hh);
	}
	while(entrar && i < hh->length)	{
		if(strcmp(key,hh->keys[i]) == 0)	{
			entrar = 0;
			i--;
		}
		i++;
	}
	if(entrar)	{
		key_length = strlen(key);
		value_length = strlen(value);
		hh->keys[hh->length] = (char*) malloc(key_length + 1);
		hh->values[hh->length] = (char*) malloc(value_length + 1);
		memcpy(hh->keys[hh->length],key,key_length);
		memcpy(hh->values[hh->length],value,value_length);
		hh->keys[hh->length][key_length] = '\0';
		hh->values[hh->length][value_length] = '\0';
		hh->length++;
	}
	else	{
		http_headers_set_value_index(hh,i,value);
	}
}

char *http_headers_get_value(HTTP_HEADERS hh, char *key)	{
	char *ret = NULL;
	int i = 0,entrar = 1;
	if(hh == NULL || key == NULL)	{
		fprintf(stderr,"Some value is NULL\n");
		exit(102);
	}
	while(entrar && i < hh->length)	{
		if(strcmp(key,hh->keys[i]) == 0)	{
			i--;
			entrar = 0;
		}
		i++;
	}
	if(entrar == 0)	{
		ret = hh->values[i];
	}
	return ret;
}

void http_headers_set_value_index(HTTP_HEADERS hh,int index, char *value)	{
	int value_length = 0;
	if(hh == NULL || value == NULL)	{
		fprintf(stderr,"Some value is NULL\n");
		exit(102);
	}
	free(hh->values[index]);
	value_length = strlen(value);
	hh->values[index] = (char*) malloc(value_length + 1);
	memcpy(hh->values[hh->length],value,value_length);
	hh->values[hh->length][value_length] = '\0';
}

void http_headers_increment_max_length(HTTP_HEADERS hh)	{
	hh->max_length *= 2;
	hh->keys = realloc(hh->keys,hh->max_length * sizeof(char**));
	hh->values = realloc(hh->values,hh->max_length * sizeof(char**));
	if(hh->keys == NULL || hh->values == NULL)	{
		perror("realloc");
		exit(105);
	}
}

void http_headers_free(HTTP_HEADERS hh)	{
	int i = 0;
	if(hh != NULL )	{
		if(hh->keys != NULL && hh->values != NULL){
			while(i< hh->length)	{
				if(hh->keys[i] != NULL)	{
					free(hh->keys[i]);
				}
				if(hh->values[i] != NULL)	{
					free(hh->values[i]);
				}				
				i++;
			}
			free(hh->keys);
			free(hh->values);
		}
		free(hh);
	}
}
