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

#define ARGS_ERROR "The program accepts just one command-line argument\n"
#define ARG_ERROR "The given path %s is not valid: %s\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define LINE_SIZE 1024

// Declaretions
int write_user_input(int fd);
//char * get_line(void);

int main(int argc, char** argv){
	int fd;
	if (argc != 2) {
		printf(ARGS_ERROR);
		return -1;
	}
	struct stat st;
	if (stat(argv[1], &st) == -1){ 
		if (errno == ENOENT) mkfifo(argv[1], 0777); // The file doesn't exist
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
			else mkfifo(argv[1], 0777);
		}
	}
	fd = open(argv[1], O_WRONLY);
	if (fd == -1){
		printf(FUNC_ERROR, "open", strerror(errno));
		return -1;
	}
	if (write_user_input(fd) == -1){
		close(fd);
		return -1;
	}
	else{
		remove(argv[1]);
		close(fd);
	}
	return 0;
}


int write_user_input(int fd){
	char line[LINE_SIZE];
	while (fgets(line, LINE_SIZE, stdin) != NULL){
		if (write(fd, line, strlen(line)) == -1){
			printf(FUNC_ERROR, "write", strerror(errno));
			return -1;
		}
	}
	return 0;
}

//
//int write_user_input(int fd){
//	char * line;
//	while (1){
//
//		line = get_line();
//		if (line == NULL) return -1;
//		if (write(fd, line, strlen(line) + 1) == -1){
//			printf(FUNC_ERROR, "write", strerror(errno));
//			free(line);
//			return -1;
//		}
//		if (line[strlen(line)] == EOF){
//			free(line);
//			return 0;
//		}
//		free(line);
//	}
//}
//
//
//char * get_line(void) { //referance: http://stackoverflow.com/questions/314401/how-to-read-a-line-from-the-console-in-c
//	int c;
//	size_t lenmax = LINE_SIZE, len = LINE_SIZE;
//	char * line = malloc(LINE_SIZE), *curr_p = line;
//	if (line == NULL) return NULL; //the malloc failed
//	while (1){
//		c = fgetc(stdin);
//		if (c == EOF) break;
//		if (--len == 0) { //extand the line
//			len = lenmax;
//			char * new_line = realloc(line, lenmax *= 2);
//			if (new_line == NULL){ //the realloc failed
//				free(line);
//				return NULL;
//			}
//			curr_p = new_line + (curr_p - line);
//			line = new_line;
//		}
//		*curr_p++ = c;
//		if (c == '\n') break; //end of line
//	}
//	*curr_p = '\0';
//	return line;
//}