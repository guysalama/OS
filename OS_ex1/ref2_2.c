#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

int fifo;
char* f_path;

static void signal_handler(int signum){
	if (signum == SIGINT || signum == SIGTERM){
		if (unlink(f_path) != 0) printf("Error removing FIFO: %s\n", strerror(errno));
		if (close(fifo) != 0) printf("Error closing FIFO: %s\n", strerror(errno));
		exit(0);
	}
	if (signum == SIGPIPE) printf("Error: writing to FIFO without open reader\n");
}

int reg_signals(){
	struct sigaction signal;
	signal.sa_handler = &signal_handler;
	sigemptyset(&signal.sa_mask);
	signal.sa_flags = 0;

	if (sigaction(SIGTERM, &signal, NULL) < 0) {
		printf("Error registering signal handler for SIGTERM: %s\n", strerror(errno));
		return 0;
	}

	if (sigaction(SIGINT, &signal, NULL) < 0) {
		printf("Error registering signal handler for SIGINT: %s\n", strerror(errno));
		return 0;
	}

	if (sigaction(SIGPIPE, &signal, NULL) < 0) {
		printf("Error registering signal handler for SIGPIPE: %s\n", strerror(errno));
		return 0;
	}
	return 1;
}

int open_write_fifo(char* path){
	struct stat statbuf;
	int fd;
	if (stat(path, &statbuf) == 0){
		// file exists
		if (S_ISFIFO(statbuf.st_mode)){
			if ((fd = open(path, O_WRONLY)) == -1){
				printf("Error opening FIFO: %s : %s\n", path, strerror(errno));
				return 0;
			}
			return fd;
		}
		int retval = unlink(path);
		assert(retval == 0);
	}
	else{
		if (errno != ENOENT){ // file exists but got other error from stat
			printf("file exists, got error %s\n", strerror(errno));
			return 0;
		}
	}
	if (mkfifo(path, 0777) == -1){
		printf("Error creating FIFO: %s : %s\n", path, strerror(errno));
		return 0;
	}
	if ((fd = open(path, O_WRONLY)) == -1){
		printf("Error opening FIFO: %s : %s\n", path, strerror(errno));
		return 0;
	}
	return fd;
}


int main(int argc, char** argv){
	assert(argc == 2);
	f_path = argv[1];
	if (!reg_signals()) return 0;

	fifo = open_write_fifo(f_path);
	assert(fifo != 0);

	char line[1024];
	while (fgets(line, 1024, stdin) != NULL) {
		if (write(fifo, line, strlen(line)) == -1){
			printf("Error writing to FIFO: %s\n", strerror(errno));
			return 0;
		}
	}

	signal_handler(SIGTERM);
	return 0;
}