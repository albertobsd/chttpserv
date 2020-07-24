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


void **debug_ptr = NULL;
int *debug_ptr_sizes = NULL;
int *debug_valores = NULL;

/*
 *	Custom malloc function this function save the ptr in a List to debug your code
 */
void* debug_malloc(size_t size)	{
	void *ptr = NULL;
	ptr =  malloc(size +10);
	if(ptr != NULL)	{
		debug_add(ptr,size);
	}
	else	{
		perror("malloc");
	}
	return ptr;
}

void* debug_calloc(size_t nmemb,size_t size)	{
	int length =nmemb * size;
//wzPUXG2kFPprZwJrsBxVWCUN0IRz7CmoOw5yXeirMQqTjYtv4uHHlnWfGgd8SBXMuZXZlnryzg0xaDhd
	void *ptr = NULL;
	ptr = malloc(length + 10);
	if(ptr != NULL) 	{
		debug_add(ptr,length);
		memset(ptr,0,length);
	}
	else	{
		perror("calloc");
	}
	return ptr;
}

void* debug_realloc(void* ptr,size_t size)	{
	void *ptr_new = NULL;
	ptr_new = realloc(ptr,size + 10);
	if(ptr_new != NULL)	{
			debug_remove(ptr);
			debug_add(ptr_new,size);
	}else	{
		perror("realloc");
	}
	return ptr_new;
}

void debug_free(void *ptr)	{
	if(ptr)	{
		if(debug_remove(ptr) == 0)
			free(ptr);
	}
}

void debug_add(void *ptr,int length)	{
	pthread_mutex_lock(&debug_lock);
	if(debug_valores[DEBUG_INDEX] >= debug_valores[DEBUG_ITEMS_MAX])	{
		debug_valores[DEBUG_ITEMS_MAX]*=2;
		debug_ptr = (void**)realloc((void*)debug_ptr,debug_valores[DEBUG_ITEMS_MAX]*sizeof(char*));
		debug_ptr_sizes = (int*)realloc((void*)debug_ptr_sizes,debug_valores[DEBUG_ITEMS_MAX]*sizeof(int));
	}
	debug_ptr[debug_valores[DEBUG_INDEX]] = ptr;
	debug_ptr_sizes[debug_valores[DEBUG_INDEX]] = length;
	debug_valores[DEBUG_MEMORY]+=length;
	debug_valores[DEBUG_ITEMS]++;
	if(debug_valores[DEBUG_INDEX] % 20 == 0)	{
		debug_optimizar();
	}
	debug_valores[DEBUG_INDEX]++;
	pthread_mutex_unlock(&debug_lock);
}

/*
 *	This function remove ptr internaly in our debug list
 *  return 0 on success
 */
int debug_remove(void *ptr)	{
	int ret = 0;
	int i = 0;
	int encontrado = 0;
	pthread_mutex_lock(&debug_lock);
	while( !encontrado && i < debug_valores[DEBUG_INDEX] )	{
		if(ptr == debug_ptr[i])	{
			encontrado = 1;
			debug_ptr[i] = NULL;
			debug_valores[DEBUG_MEMORY] -= debug_ptr_sizes[i];
			debug_ptr_sizes[i] = 0;
			debug_valores[DEBUG_ITEMS]--;
		}
		i++;
	}
	if(encontrado == 0)	{
		printf("Pointer not found 0x%p \nMay be you missing change some malloc, calloc realloc?\nOr pointer previously release\n",ptr	);
		ret = 1;
	}
	pthread_mutex_unlock(&debug_lock);
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
	/*
	while(i < debug_valores[DEBUG_INDEX])	{
		if(debug_ptr[i] != NULL)	{
			printf("%i:  ptr: %p , size %i , value %s\n",j,debug_ptr[i],debug_ptr_sizes[i],debug_ptr[i]);
			j++;
		}
		i++;
	}
	*/
	pthread_mutex_unlock(&debug_lock);
}

void debug_list()	{
	int i = 0,j = 0;
	pthread_mutex_lock(&debug_lock);
	while(i < debug_valores[DEBUG_INDEX])	{
		if(debug_ptr[i] != NULL)	{
			printf("%i:  ptr: %p , size %i , value %s\n",j,debug_ptr[i],debug_ptr_sizes[i],debug_ptr[i]);
			j++;
		}
		i++;
	}
	pthread_mutex_unlock(&debug_lock);
}
