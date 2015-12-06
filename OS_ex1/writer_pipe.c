#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
//#include <sys/time.h>

#define ARGS_ERROR "The program accepts just one command-line argument\n"
#define ARG_ERROR "The given path %s is not valid: %s\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define SIG_ERROR "Error occurred while setting %s handler: %s\n"
#define CLOSED_PIPE "The writer holds an open FIFO file with no reader\n"
#define LINE_SIZE 1024

// Declaretions
int write_user_input(int fd);
int set_signals(void);
void signals_handler(int signal);

// Globals
int fd;
char * path;

int main(int argc, char** argv){
	if (argc != 2) {
		printf(ARGS_ERROR);
		return -1;
	}
	path = argv[1];
	if (set_signals() == -1) return -1;
	struct stat st;
	if (stat(path, &st) == -1){ 
		if (errno == ENOENT) mkfifo(path, 0777); // The file doesn't exist
		else{
			printf(ARG_ERROR, path, strerror(errno));
			return -1;
		}
	}
	else{ //the file exist
		if (!S_ISFIFO(st.st_mode)){ // It isn't fifo file
			if (unlink(path) == -1){
				printf(FUNC_ERROR, "unlink", strerror(errno));
				return -1;
			}
			else mkfifo(path, 0777);
		}
	}
	fd = open(path, O_WRONLY);
	if (fd == -1){
		printf(FUNC_ERROR, "open", strerror(errno));
		return -1;
	}
	if (write_user_input(fd) == -1){
		close(fd);
		return -1;
	}
	else signals_handler(SIGTERM);
	//return 0;
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

int set_signals(void){
	struct sigaction act;
	act.sa_handler = &signals_handler;
	act.sa_flags = 0;
	if (sigemptyset(&act.sa_mask) == -1){
		printf(SIG_ERROR, "SIG*", strerror(errno));
		return -1;
	}
	if (sigaction(SIGINT, &act, NULL) == -1){
		printf(SIG_ERROR, "SIGINT", strerror(errno));
		return -1;
	}
	if (sigaction(SIGTERM, &act, NULL) == -1){
		printf(SIG_ERROR, "SIGTERM", strerror(errno));
		return -1;
	}
	if (sigaction(SIGPIPE, &act, NULL) == -1){
		printf(SIG_ERROR, "SIGPIPE", strerror(errno));
		return -1;
	}
	return 0;
}


void signals_handler(int signal){
	switch (signal){
	case SIGINT: case SIGTERM:
		if (unlink(path) == -1) printf(FUNC_ERROR, "unlink", strerror(errno));
		close(fd);
		exit(0);
	case SIGPIPE:
		printf(CLOSED_PIPE);
		break;
	default:
		return;
	}
}


