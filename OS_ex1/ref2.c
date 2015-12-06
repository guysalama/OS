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


int open_read_fifo(char* path){
	struct stat statbuf;
	int fd;
	while (stat(path, &statbuf) != 0){
		sleep(1);
	}
	if (S_ISFIFO(statbuf.st_mode)){
		if ((fd = open(path, O_RDONLY)) == -1){
			printf("Error opening FIFO: %s : %s\n", path, strerror(errno));
			return 0;
		}
		return fd;
	}
	else{
		printf("%s is not a valid FIFO: %s\n", path, strerror(errno));
		exit(0);
	}
}

int signal_switch(struct sigaction *new_signal, struct sigaction *old_signal){
	if (sigaction(SIGTERM, new_signal, old_signal) < 0) {
		printf("Error registering signal handler for SIGTERM: %s\n", strerror(errno));
		return 0;
	}

	if (sigaction(SIGINT, new_signal, old_signal) < 0) {
		printf("Error registering signal handler for SIGINT: %s\n", strerror(errno));
		return 0;
	}
}

int main(int argc, char** argv){
	assert(argc == 2);

	struct sigaction new_signal, old_signal;
	new_signal.sa_handler = SIG_IGN; 
	sigemptyset(&new_signal.sa_mask);
	new_signal.sa_flags = 0;

	char line[1024];
	int num;
	while (1){
		int fifo = open_read_fifo(argv[1]);
		assert(fifo != 0);

		signal_switch(&new_signal, &old_signal);

		while ((num = read(fifo, line, 1024)) > 0) {
			line[num] = '\0';
			printf("%s", line);
		}

		if (close(fifo) != 0) printf("Error closing FIFO: %s\n", strerror(errno));

		signal_switch(&old_signal, NULL);
	}
}