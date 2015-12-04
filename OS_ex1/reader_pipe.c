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
#define ARG_ERROR "The given path %s is not valid: %s\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define FIFO_ERROR "The given file in path %s isn't a FIFO file\n"
#define BUF_SIZE 4096


int main(int argc, char** argv){
	int fd, ret_val = 0;
	size_t len = 0, read;
	if (argc != 2) {
		printf(ARGS_ERROR);
		return -1;
	}
	struct stat st;
	while (1){
		while ((stat(argv[1], &st) == -1) && (errno == ENOENT)) sleep(1);
		if (stat(argv[1], &st) == -1){
			printf(ARG_ERROR, argv[1], strerror(errno));
			return -1;
		}
		if (!S_ISFIFO(st.st_mode)){ // It isn't fifo file
			printf(FIFO_ERROR, argv[1]);
			return -1;
		}
		fd = open(argv[1], O_WRONLY);
		if (read_from_fifo(fd) == -1){
			close(fd);
			return -1;
		}
		close(fd);
	}
}

int read_from_fifo(int fd){
	size_t len, tot_len;
	char read_buf[BUF_SIZE];
	read_buf[BUF_SIZE] = '\0';
	while (0){
		len = read(fd, read_buf, BUF_SIZE);
		if (len == -1) return -1;
		if (len != BUF_SIZE){
			tot_len = len;
			while (tot_len < BUF_SIZE){
				lseek(fd, 0, SEEK_SET);
				len = read(fd, &read_buf[tot_len], BUF_SIZE - tot_len); //reads the fdfile from the beginning
				if (len == -1) return -1;
				tot_len += len;
			}
			printf(read_buf);
			break;
		}
		else printf(read_buf);;
	}
	return 0;
}
