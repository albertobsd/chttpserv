#pragma once	
#include "util.h"
#include <limits.h>
#include <stdlib.h>



#define CONFIG_VAR_LENGTH_CONST 12

#define	IP 0
#define PORT 1
#define PATH 2
#define LOGFILE 3
#define TIMEOUT 4
#define MAXREQUESTSIZE 5
#define CACHEFILES 6
#define DIRLIST 7
#define SSL 8
#define SSLPORT 9
#define CERTFILE 10
#define CERTKEY 11


int config_var_length = CONFIG_VAR_LENGTH_CONST;
char *defaults_config[CONFIG_VAR_LENGTH_CONST] = {"localhost","80","./public","./access.log","30","1048576","","OFF","ON","443","./cert.pem","./key.pem"};
char *variables_config[CONFIG_VAR_LENGTH_CONST] = {"IP","PORT","PATH","LOGFILE","TIMEOUT","MAXREQUESTSIZE","CACHEFILES","DIRLIST","SSL","SSLPORT","CERTFILE","CERTKEY"};
int found_config[CONFIG_VAR_LENGTH_CONST] ={0,0,0,0,0,0,0,0,0,0,0,0};

char *_CONFIG[CONFIG_VAR_LENGTH_CONST] = {NULL};

int read_config()	{
	int  i = 0;
	FILE *config = NULL;
	config = fopen("serv.conf","r");
	if(config == NULL)	{
		while(i < CONFIG_VAR_LENGTH_CONST)	{
			_CONFIG[i] = defaults_config[i];
			i++;
		}
		return 0;
	}
	else	{
		int line_length = 0,key_length = 0, value_length = 0,index_temp = 0;
		char *line = NULL,*aux,*key = NULL,*value = NULL;
		line = (char*) malloc(1024);
		while(fgets(line,1024,config) != NULL && !feof(config))	{
			trim(line,NULL);
			line_length = strlen(line);
			if(line[0] != '#')	{
				aux = strstr(line,"=");
				if(aux != NULL)	{
					key_length = aux -line;
					value_length = line_length-key_length - 1 ;
					/*
					printf("Line with value: %s\n",line);
					printf("%i - %i\n",key_length,value_length);
					*/
					if(key_length > 0 && value_length > 0)	{
						key = (char*) calloc(key_length+1,1);
						value = (char*) calloc(value_length+1,1);
						memcpy(key,line,key_length);
						memcpy(value,aux+1,value_length);
						trim(key,NULL);
						trim(value,NULL);
						/*
						printf("Key : %s\n",key);
						printf("Value : %s\n",value);
						*/
						if((index_temp = index_of(variables_config,CONFIG_VAR_LENGTH_CONST,key)) >= 0){
							//printf("Key found [%s] index %i : %s\n",key,index_temp,variables_config[index_temp]);
							found_config[index_temp] = 1;							
							_CONFIG[index_temp] = value;
						}
						else	{
							free(key);
							free(value);
							//printf("Ignoring line with unexpected key: %s\n",line);	
						}
					}
					else	{
						//printf("Ignoring line without value or key: %s\n",line);	
					}
				}
				else	{
					//printf("Ignored line without equal sign: %s\n",line);
				}
			}
			else	{
				//printf("Ignored comment line: %s\n",line);
			}
		}
		free(line);
		fclose(config);
		while(i < CONFIG_VAR_LENGTH_CONST)	{
			if(found_config[i] == 0)	{
				_CONFIG[i] = defaults_config[i];
			}
			i++;
		}
	}
	_CONFIG[PATH] = realpath(_CONFIG[PATH],NULL);
	return 0;
}