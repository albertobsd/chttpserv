#pragma once
//#include "debug_bt.h"
#include "debug.h"

typedef struct STR_HTTP_HEADERS {
	int max_length;
	int length;
	char **keys;
	char **values;
}*HTTP_HEADERS;

HTTP_HEADERS http_headers_init();
int http_headers_add(HTTP_HEADERS hh,char *key,char *value);
char *http_headers_get_value(HTTP_HEADERS hh, char *key);
void http_headers_set_value_index(HTTP_HEADERS hh,int index, char *value);
void http_headers_increment_max_length(HTTP_HEADERS hh);
void http_headers_free(HTTP_HEADERS hh);
void http_headers_info(HTTP_HEADERS hh);
int http_headers_isset(HTTP_HEADERS hh,char *key);

int http_headers_enable_debug = 0;

HTTP_HEADERS http_headers_init()	{
	if( http_headers_enable_debug == 1 ) fprintf(stderr,"http_headers_init\n");
	int max_length = 2;
	HTTP_HEADERS hh = NULL;
	char **keys,**values;
	hh = (HTTP_HEADERS) debug_calloc(1,sizeof(struct STR_HTTP_HEADERS));
	keys = (char **) debug_calloc(max_length+1,sizeof(char**));
	values = (char **) debug_calloc(max_length+1,sizeof(char**));
	if(hh == NULL || keys == NULL || values == NULL)	{
		fprintf(stderr,"debug_malloc\n");
		perror("debug_calloc");
		exit(-1);
	}
	else	{
		hh->length = 0;
		hh->max_length = max_length;
		hh->keys = keys;
		hh->values = values;
	}
	return hh;
}

/*
 *	http_headers_add add add new <key,value> to hh
 *  if key already exist, his current value is going to be overwriting
 *  the passed params key,value are copied in internal struct and are no longer need you can FREE them without worries
 */
int http_headers_add(HTTP_HEADERS hh,char *key,char *value)	{
	if( http_headers_enable_debug == 1 ) fprintf(stderr,"http_headers_add\n");
	int ret = 0;
	int i = 0,entrar  = 1;
	int key_length = 0;
	int value_length = 0;
	if(hh == NULL || hh->keys == NULL || hh->values == NULL || key == NULL || value == NULL)	{
		fprintf(stderr,"Some value is NULL\n");
		ret = -1;
	}
	else	{
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
			if( http_headers_enable_debug == 1 ) fprintf(stderr,"K_l:%i , V_l:%i\n",key_length,value_length);
			hh->keys[hh->length] = (char*) debug_malloc(key_length+1);
			hh->values[hh->length] = (char*) debug_malloc(value_length+1);
			if(hh->keys[hh->length] == NULL && hh->values[hh->length] == NULL)	{
				fprintf(stderr,"debug_malloc\n");
				perror("debug_malloc");
				exit(-1);
			}
			else	{
				if( http_headers_enable_debug == 1 ) fprintf(stderr,"k_ptr :  %p, v_ptr : %p\n",hh->keys[hh->length],hh->values[hh->length]);
				memcpy(hh->keys[hh->length],key,key_length);
				memcpy(hh->values[hh->length],value,value_length);
				hh->keys[hh->length][key_length] = '\0';
				hh->values[hh->length][value_length] = '\0';
				hh->length++;
			}
		}
		else	{
			/*
				key already exist, remplace value
			*/
			http_headers_set_value_index(hh,i,value);
		}
	}
	return ret;
}

char *http_headers_get_value(HTTP_HEADERS hh, char *key)	{
	if( http_headers_enable_debug == 1 ) fprintf(stderr,"http_headers_get_value\n");
	char *ret = NULL;
	int i = 0,entrar = 1;
	if(hh == NULL || key == NULL)	{
		fprintf(stderr,"http_headers_get_value(): Some value is NULL\n");
	}
	else	{
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
	}
	return ret;
}

void http_headers_set_value_index(HTTP_HEADERS hh,int index, char *value)	{
	if( http_headers_enable_debug == 1 ) fprintf(stderr,"http_headers_set_value_index\n");
	int value_length = 0;
	if(hh == NULL || value == NULL || index < 0)	{
		fprintf(stderr,"Some value is NULL\n");
		exit(-1);
	}
	else	{
		debug_free(hh->values[index]);
		value_length = strlen(value);
		hh->values[index] = (char*) debug_malloc(value_length +1);
		if(hh->values[index] == NULL)	{
			fprintf(stderr,"debug_malloc\n");
			perror("debug_malloc");
			exit(-1);
		}
		else	{
			memcpy(hh->values[index],value,value_length);
			hh->values[index][value_length] = '\0';
		}
	}
}

void http_headers_increment_max_length(HTTP_HEADERS hh)	{
	hh->max_length *= 2;
	hh->keys =  (char**) debug_realloc( (void*) hh->keys,(hh->max_length +2)* sizeof(char**));
	hh->values = (char**) debug_realloc( (void*) hh->values,(hh->max_length +2) * sizeof(char**));
	if(hh->keys == NULL || hh->values == NULL)	{
		perror("debug_realloc");
		exit(-1);
	}
}

void http_headers_free(HTTP_HEADERS hh)	{
	if( http_headers_enable_debug == 1 ) {
		printf("http_headers_free\n");
		http_headers_info(hh);
	}
	int i = 0;
	if(hh != NULL )	{
		if(hh->keys != NULL && hh->values != NULL){
			while( i < hh->length)	{
				if(hh->keys[i] != NULL)	{
					debug_free(hh->keys[i]);
				}
				if(hh->values[i] != NULL)	{
					debug_free(hh->values[i]);
				}
				i++;
			}
			debug_free(hh->keys);
			debug_free(hh->values);
		}
		debug_free(hh);
	}
}

void http_headers_info(HTTP_HEADERS hh)	{
	int i = 0;
	if(http_headers_enable_debug == 1) fprintf(stderr,"http_headers_info\n");
	if(hh != NULL)	{
		printf("max_length %i\n",hh->max_length);
		printf("length %i\n",hh->length);
		while(i < hh->length)	{
			printf("%i : %s = %s\n",i,hh->keys[i],hh->values[i]);
			i++;
		}
	}
	else	{
		fprintf(stderr,"hh is null\n");
	}
}

int http_headers_isset(HTTP_HEADERS hh,char *key)	{
	int entrar = 1, i = 0;
	while(entrar && i < hh->length)	{
		if(strcmp(hh->keys[i],key) == 0)	{
			entrar = 0;
		}
		i++;
	}
	return !entrar;
}
