#pragma once
#include "conf.h"

typedef struct STR_HTTP_SERVE_CONF {
	char *ip;
	char *path;
	char *logfile;
	int port;
	int pathlength;
	int timeout;
	int maxrequestsize;
  int maxlinerequestsize;
	int length_cachefile;
	char **cachefilespath;
}*HTTP_SERVE_CONF;

HTTP_SERVE_CONF http_serve_conf_init();

int http_serve_conf_enable_verbose = 0;
HTTP_SERVE_CONF server_config;

HTTP_SERVE_CONF http_serve_conf_init()	{
	if(http_serve_conf_enable_verbose == 1 ) printf("http_serve_conf_init\n");
  HTTP_SERVE_CONF hsconf = debug_calloc(1,sizeof(struct STR_HTTP_SERVE_CONF));
  if(hsconf == NULL)  {
		perror("debug_calloc");
		exit(0);
  }
	read_config();
	hsconf->maxrequestsize = strtol(_CONFIG[MAXREQUESTSIZE],NULL,10);
	hsconf->maxlinerequestsize = strtol(_CONFIG[MAXLINEREQUESTSIZE],NULL,10);
	hsconf->path = _CONFIG[PATH];
	hsconf->pathlength = strlen(hsconf->path);
  return hsconf;
}
