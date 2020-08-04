#pragma once
#include<pthread.h>
#include<string.h>
#include<stdlib.h>

#define DEBUG_ITEMS_MAX 0
#define DEBUG_ITEMS 1
#define DEBUG_INDEX 2
#define DEBUG_MEMORY 3

pthread_mutex_t debug_lock;

void* debug_malloc(size_t size);
void* debug_calloc(size_t nmemb,size_t size);
void debug_free(void *ptr);

void debug_optimizar();
void debug_add(void *ptr,int length);
int debug_remove(void *ptr);
void debug_status();
void debug_list();
char *debug_tohex(char *ptr,int length);

int debug_enable_verbose = 0;

void **debug_ptr = NULL;
int *debug_ptr_sizes = NULL;
int *debug_valores = NULL;

/*
 *	Custom malloc function this function save the ptr in a List to debug your code
 */
void* debug_malloc(size_t size)	{
	void *ptr = NULL;
	pthread_mutex_lock(&debug_lock);
	//printf("pthread_mutex_lock\n");
	ptr =  malloc(size+5);
	if(ptr != NULL)	{
		debug_add(ptr,size);
	}
	else	{
		perror("malloc");
	}
		//printf("pthread_mutex_unlock\n");
	pthread_mutex_unlock(&debug_lock);
	return ptr;
}

void* debug_calloc(size_t nmemb,size_t size	)	{
	pthread_mutex_lock(&debug_lock);
	//printf("pthread_mutex_lock\n");
	void *ptr = NULL;
	ptr = calloc(nmemb,size);
	if(ptr != NULL) 	{
		debug_add(ptr,nmemb*size);
	}
	else	{
		perror("calloc");
	}
	//printf("pthread_mutex_unlock\n");
	pthread_mutex_unlock(&debug_lock);
	return ptr;
}

void* debug_realloc(void* ptr,size_t size)	{
	void *ptr_new = NULL;
	pthread_mutex_lock(&debug_lock);
	//printf("pthread_mutex_lock\n");
	ptr_new = realloc(ptr,size + 10);
	if(ptr_new != NULL)	{
			debug_remove(ptr);
			debug_add(ptr_new,size);
	}
	else	{
		perror("realloc");
	}
	//printf("pthread_mutex_unlock\n");
	pthread_mutex_unlock(&debug_lock);
	return ptr_new;
}

void debug_free(void *ptr)	{
	pthread_mutex_lock(&debug_lock);
	if(ptr != NULL)	{
		if(debug_remove(ptr) == 0)	{
			free(ptr);
		}
	}
	else	{
		fprintf(stderr,"debug_free: NULL Pointer");
	}
	pthread_mutex_unlock(&debug_lock);
}

void debug_add(void *ptr,int length)	{
	if(debug_valores[DEBUG_INDEX] >= debug_valores[DEBUG_ITEMS_MAX])	{
		debug_valores[DEBUG_ITEMS_MAX]*=2;
		debug_ptr = (void**)realloc((void*)debug_ptr,debug_valores[DEBUG_ITEMS_MAX]*sizeof(char*));
		debug_ptr_sizes = (int*)realloc((void*)debug_ptr_sizes,debug_valores[DEBUG_ITEMS_MAX]*sizeof(int));
	}
	debug_ptr[debug_valores[DEBUG_INDEX]] = ptr;
	debug_ptr_sizes[debug_valores[DEBUG_INDEX]] = length;
	debug_valores[DEBUG_MEMORY]+=length;
	debug_valores[DEBUG_ITEMS]++;
	if(debug_valores[DEBUG_INDEX] % 128 == 0)	{
		debug_optimizar();
	}
	debug_valores[DEBUG_INDEX]++;
	if(debug_enable_verbose == 1 ) printf("debug_add: %p : %i\n",ptr,length);
}

/*
 *	This function remove ptr internaly in our debug list
 *  return 0 on success
 */
int debug_remove(void *ptr)	{
	int ret = 0;
	int i = 0;
	int encontrado = 0;
	char *hex;
	if(debug_enable_verbose == 1 ) printf("debug_remove: %p\n",ptr);
	while( !encontrado && i < debug_valores[DEBUG_INDEX] )	{
		if(ptr == debug_ptr[i])	{
			encontrado = 1;
			debug_valores[DEBUG_MEMORY] -= debug_ptr_sizes[i];
			debug_valores[DEBUG_ITEMS]--;
			debug_ptr[i] = NULL;
			debug_ptr_sizes[i] = 0;
		}
		i++;
	}
	if(encontrado == 0)	{
		printf("Pointer not found %p \nMay be you missing change some malloc, calloc realloc?\nOr pointer previously release\n",ptr	);
		debug_list();
		ret = 1;
	}
	return ret;
}

void debug_optimizar()	{
	int i = 0,aux,items;
	while(i < debug_valores[DEBUG_INDEX] ){
		if(debug_ptr[i] == NULL)	{
			items = debug_valores[DEBUG_INDEX] - i;
			memcpy(&debug_ptr[i],&debug_ptr[i+1],items * sizeof(char*));
			memcpy(&debug_ptr_sizes[i],&debug_ptr_sizes[i+1],items * sizeof(int));
			debug_ptr[debug_valores[DEBUG_INDEX]] = NULL;
			debug_ptr_sizes[debug_valores[DEBUG_INDEX]] = 0;
			i--;
			debug_valores[DEBUG_INDEX]--;
		}
		i++;
	}
}

void debug_init()	{
	if(debug_ptr == NULL)	{
		debug_valores = (int*) calloc(4,sizeof(int));
		debug_valores[DEBUG_ITEMS_MAX] = 2;
		debug_ptr = (void**)calloc(debug_valores[DEBUG_ITEMS_MAX],sizeof(char*));
		debug_ptr_sizes = (int*)calloc(debug_valores[DEBUG_ITEMS_MAX],sizeof(char*));
	}
	if(pthread_mutex_init(&debug_lock, NULL)  != 0)	{
		perror("pthread_mutex_init");
		exit(1);
	}
}

void debug_status()	{
	//int i = 0,j = 0;
	pthread_mutex_lock(&debug_lock);
	printf("Current memory Allocated: %i bytes\n",debug_valores[DEBUG_MEMORY]);
	printf("Current total chunks used : %i\n",debug_valores[DEBUG_ITEMS]);
	pthread_mutex_unlock(&debug_lock);
}

void debug_list()	{
	pthread_mutex_lock(&debug_lock);
	int i = 0,j = 0;
	while(i < debug_valores[DEBUG_INDEX])	{
		if(debug_ptr[i] != NULL && j++ && j > 98)	{
			printf("%i:  ptr: %p , size %i , value %s\n",j,debug_ptr[i],debug_ptr_sizes[i],debug_ptr[i]);
		}
		i++;
	}
	pthread_mutex_unlock(&debug_lock);
}

char *debug_tohex(char *ptr,int length)	{
	char *buffer;
  int offset = 0;
  unsigned char c;
  buffer = (char *) malloc((length * 2)+1);
	if(buffer !=NULL)	{
	  for (int i = 0; i <length; i++) {
	    c = ptr[i];
			sprintf(buffer + offset,"%.2x",c);
			offset+=2;
	  }
		buffer[length*2] = '\0';
	}
	else{
		perror("debug_tohex:malloc");
	}
  return buffer;
}
