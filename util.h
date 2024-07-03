#pragma once

char *ltrim(char *str, const char *seps);
char *rtrim(char *str, const char *seps);
char *trim(char *str, const char *seps);
char *tohex(char *ptr,int length);
char *fdgets(char *buff,int max_length,int fd);
int index_of(char **array, int size, char *lookfor );
int is_regular_file(const char *path);
int is_directory(const char *path);
off_t fsize(const char *filename);
int url_decode(const char *s, char *dec);
int ishex(unsigned char x);
void log_error(const char *file, int line, const char *msg);


char *ltrim(char *str, const char *seps)	{
	size_t totrim;
	if (seps == NULL) {
		seps = "\t\n\v\f\r ";
	}
	totrim = strspn(str, seps);
	if (totrim > 0) {
		size_t len = strlen(str);
		if (totrim == len) {
			str[0] = '\0';
		}
		else {
			memmove(str, str + totrim, len + 1 - totrim);
		}
	}
	return str;
}

char *rtrim(char *str, const char *seps)	{
	int i;
	if (seps == NULL) {
		seps = "\t\n\v\f\r ";
	}
	i = strlen(str) - 1;
	while (i >= 0 && strchr(seps, str[i]) != NULL) {
		str[i] = '\0';
		i--;
	}
	return str;
}

char *trim(char *str, const char *seps)	{
	return ltrim(rtrim(str, seps), seps);
}

int index_of(char **array, int size, char *lookfor )
{
    int i;
    for (i = 0; i < size; i++)
        if (strcmp(lookfor, array[i]) == 0)
            return i;
    return -1;
}

char *tohex(char *ptr,int length){
  char *buffer;
  int offset = 0;
  unsigned char c;
  buffer = (char *) malloc((length * 2)+1);
  for (int i = 0; i <length; i++) {
    c = ptr[i];
		sprintf(buffer + offset,"%.2x",c);
		offset+=2;
  }
	buffer[length*2] = 0;
  return buffer;
}

/*
 *	nextline function reads the content in ptr pointer
 *	it returns a new allocated pointer with the next avaible line
 *  or NULL if the \0 was reached
 *	the function set offset[0] with the current offset
 */
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int is_directory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

off_t fsize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}

char *fdgets(char *buff,int max_length,int fd)	{
	char *temp = NULL;
	int offset,readed;
	if(buff != NULL	&& max_length > 0)	{
		max_length--;
		offset = 0;
		readed = 0;
		temp = buff;
		do{
			readed = recv(fd,temp+offset,1,0);
			putchar('.');
			offset++;
		}while(readed >= 0 && offset < max_length -1 && 	temp[offset-1] != '\n' );
		if(readed == -1)	{
			perror("recv");
		}
		temp[offset] = 0;
	}
	return temp;
}


int ishex(unsigned char x)	{
	return	(x >= '0' && x <= '9')	|| 	(x >= 'a' && x <= 'f')	|| 	(x >= 'A' && x <= 'F');
}

int url_decode(const char *s, char *dec)	{
	char *o;
	const char *end = s + strlen(s);
	int c;
	for (o = dec; s <= end; o++) {
		c = *s++;
		if (c == '+') c = ' ';
		else if (c == '%' && (	!ishex(*s++)	||
					!ishex(*s++)	||
					!sscanf(s - 2, "%2x", &c)))
			return -1;

		if (dec) *o = c;
	}
	return o - dec;
}


void log_error(const char *file, int line, const char *msg) {
    fprintf(stderr, "Error in file %s at line %d: %s\n", file, line, msg);
	fflush(stderr);
    perror("System error");
}