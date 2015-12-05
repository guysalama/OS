#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#define ARGS_ERROR "The program accepts just one command-line arguments\n"
#define ARG_ERROR "The given path %s is not valid\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define LINE_SIZE 100

// Declaretions
int write_user_input(int fd);
char * get_line(void);

int main(int argc, char** argv){
	int fd;
	if (argc != 2) {
		printf(ARGS_ERROR);
		return -1;
	}
	struct stat st;
	if (stat(argv[1], &st) == -1){ 
		if (errno == ENOENT) mkfifo(argv[1], S_IWUSR | S_IWGRP | S_IWOTH); // The file doesn't exist
		else{
			printf(ARG_ERROR, argv[1], strerror(errno));
			return -1;
		}
	}
	else{ //the file exist
		if (!S_ISFIFO(st.st_mode)){ // It isn't fifo file
			if (unlink(argv[1]) == -1){
				printf(FUNC_ERROR, "unlink", strerror(errno));
				return -1;
			}
			else mkfifo(argv[1], S_IWUSR | S_IWGRP | S_IWOTH);
		}
	}
	fd = open(argv[1], O_WRONLY);
	if (write_user_input(fd) == -1) return -1;
	else{
		remove(argv[1]);
		close(fd);
	}
	return 0;
}



int write_user_input(int fd){
	char * line;
	while (1){
		line = get_line();
		if (write(fd, line, strlen(line) + 1) == -1){
			printf(FUNC_ERROR, "write", strerror(errno));
			free(line);
			return -1;
		}
		if (line[strlen(line)] == EOF){
			free(line);
			break;
		}
		free(line);
	}
	return 0;
}


char * get_line(void) { //referance: http://stackoverflow.com/questions/314401/how-to-read-a-line-from-the-console-in-c
	char * line = malloc(LINE_SIZE);
	char * linep = line;
	size_t lenmax = LINE_SIZE, len = lenmax;
	int c;
	
	if (line == NULL) return NULL;
	while (1){
		c = fgetc(stdin);
		if (c == EOF) break;
		if (--len == 0) { //extand the line
			len = lenmax;
			char * linen = realloc(linep, lenmax *= 2);
			if (linen == NULL) {
				free(linep);
				return NULL;
			}
			line = linen + (line - linep);
			linep = linen;
		}
		*line++ = c;
		if (c == '\n') break;
	}
	*line = '\0';
	return linep;
}