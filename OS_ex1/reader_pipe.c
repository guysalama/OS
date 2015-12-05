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

// Messages
#define ARGS_ERROR "The program accepts just one command-line arguments\n"
#define ARG_ERROR "The given path %s is not valid: %s\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define FIFO_ERROR "The given file in path %s isn't a FIFO file\n"
#define LINE_SIZE 1024
//#define BUF_SIZE 4096

int read_from_fifo(int fd);
//int print_chars(int len);

int main(int argc, char** argv){
	int fd, ret_val = 0;
	size_t len = 0, read;
	if (argc != 2) {
		printf(ARGS_ERROR);
		return -1;
	}
	struct stat st;
	while (1){
		if ((stat(argv[1], &st) == -1) && (errno != ENOENT)){
			printf(ARG_ERROR, argv[1], strerror(errno));
			return -1;
		}
		while ((stat(argv[1], &st) == -1) && (errno == ENOENT)) sleep(1);
		if (!S_ISFIFO(st.st_mode)){ // It isn't fifo file
			printf(FIFO_ERROR, argv[1]);
			return -1;
		}
		fd = open(argv[1], O_WRONLY);
		if (fd == -1){
			printf(FUNC_ERROR, "open", strerror(errno));
			return -1;
		}
		if (read_from_fifo(fd) == -1){
			close(fd);
			return -1;
		}
		close(fd);
	}
}

int read_from_fifo(int fd){
	int num;
	char line[LINE_SIZE];
	do{
		num = read(fd, line, LINE_SIZE);
		if (read == -1){
			printf(FUNC_ERROR, "read", strerror(errno));
			return -1;
		}
		line[num] = '\0';
		printf("%s", line);
	} while (num > 0);
	return 0;
}

//
//int read_from_fifo(int fd){
//	size_t len, total_len;
//	read_buf[BUF_SIZE] = '\0';
//	while (0){
//		len = read(fd, read_buf, BUF_SIZE);
//		if (len == -1) return -1;
//		if (len == BUF_SIZE){
//			total_len += len;
//			lseek(fd, total_len, SEEK_SET);
//			print_chars(len);
//			continue;
//		}
//		if (len != BUF_SIZE){
//			read_buf[len] = '\0';
//			print_chars(len);
//			break;
//		}
//	}
//	return 0;
//}
//
//
//int print_chars(int len){
//	int i;
//	for (i = 0; i <= len; i++){
//		printf("%c", read_buf[i]);
//	}
//}
//
//
