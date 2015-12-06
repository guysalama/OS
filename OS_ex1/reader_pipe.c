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

// Messages
#define ARGS_ERROR "The program accepts just one command-line arguments\n"
#define ARG_ERROR "The given path %s is not valid: %s\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define FIFO_ERROR "The given file in path %s isn't a FIFO file\n"
#define SIG_ERROR "Error occurred while setting %s handler: %s\n"
#define LINE_SIZE 1024


int read_from_fifo(int fd);


int main(int argc, char** argv){
	int fd, ret_val = 0;
	size_t len = 0, read;
	if (argc != 2) {
		printf(ARGS_ERROR);
		return -1;
	}
	struct sigaction act, old_act;
	act = create_ign_act();
	if (act == NULL) return -1;
	struct stat st;
	while (1){
		if ((stat(argv[1], &st) == -1) && (errno == ENOENT)) sleep(1);
		if (stat(argv[1], &st) == -1){ // path exist, other error occurred 
			printf(ARG_ERROR, argv[1], strerror(errno));
			return -1;
		}
		if (!S_ISFIFO(st.st_mode)){ // It isn't fifo file
			printf(FIFO_ERROR, argv[1]);
			return -1;
		}
		fd = open(argv[1], O_RDONLY);
		if (fd == -1){
			printf(FUNC_ERROR, "open", strerror(errno));
			return -1;
		}
		if (switch_acts(&act, &old_act) == -1){
			close(fd);
			return -1;
		}
		if (read_from_fifo(fd) == -1){
			close(fd);
			return -1;
		}
		close(fd);
		if (switch_acts(&old_act, &act) == -1) return -1;
	}
}

int read_from_fifo(int fd){
	int num;
	char line[LINE_SIZE];
	do{
		num = read(fd, line, LINE_SIZE);
		if (num == -1){
			printf(FUNC_ERROR, "read", strerror(errno));
			return -1;
		}
		line[num] = '\0';
		printf("%s", line);
	} while (num > 0);
	return 0;
}

struct sigaction create_ign_act(void){
	struct sigaction new_act;
	new_act.sa_handler = SIG_IGN; //ignore the signal
	new_act.sa_flags = 0;
	if (sigemptyset(&act.sa_mask) == -1){
		printf(SIG_ERROR, "SIG*", strerror(errno));
		return NULL;
	}
}

int switch_acts(struct sigaction *act, struct sigaction *old_act){
	if (sigaction(SIGINT, new_signal, old_signal) == -1){
		printf(SIG_ERROR, "SIGINT", strerror(errno));
		return -1;
	}
	if (sigaction(SIGTERM, new_signal, old_signal) == -1){
		printf(SIG_ERROR, "SIGTERM", strerror(errno));
		return -1;
	}
}
