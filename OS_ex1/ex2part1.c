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

#define ARGS_ERROR "The program accepts just three command-line arguments"
#define ARG_ERROR "The given argument: %s are not valid"
#define LINK_ERROR "The file is symbolic link or has additional hard links"
#define BLOCK_ERROR "The input file %s is a block device"

#define OPEN_ERROR "Error occurred while openning the file %s: %s\n"
#define STAT_ERROR "Error occurred while getting file stat %s: %s\n"
#define CLOSE_ERROR "Error occurred while closing the file %s: %s\n"
#define READ_ERROR "Error occurred while reading from file: %s\n"
#define WRITE_ERROR "Error occurred while writing to the file %s: %s\n"
#define SEEK_ERROR "Error occurred while seeking in the file %s: %s\n"
#define THROUGHPUT "The program wrote %f MB/sec on average"

#define MB 1024*1024
#define KB 1024

int main(int argc, char** argv){
	int i, j, ws, fd, o_direct;
	if (argc != 4){
		printf(ARGS_ERROR);
		return -1;
	}
	if (typeCheck(argv[1]) == -1){
		return -1;
	}
	if (strcmp(argv[2], "1") != 0 || strcmp(argv[2], "0") != 0){
		printf(ARG_ERROR, argv[2]);
		return -1;
	}
	ws = atoi(argv[3]);
	if (!ws){ // ws == 0
		printf(ARG_ERROR, argv[3]);
		return -1;
	}
	static char buf[MB] __attribute__((__aligned__(4096)));
	for (j = 0; j < MB; j++) buf[i] = 'a' + (random() % 26);


	struct timeval t1, t2; //referance: http://stackoverflow.com/questions/2150291/how-do-i-measure-a-time-interval-in-c, first answer
	double elapsedTime;
	gettimeofday(&t1, NULL); // start timer

	o_direct = atoi(argv[2]);
	if (o_direct) fd = open(argv[1], O_WRONLY | O_DIRECT, S_IRWXU | S_IRWXG | S_IRWXO);
	else  fd = open(argv[1], O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd == -1){
		printf(OPEN_ERROR, argv[2], strerror(errno));
		return -1;
	}
	int repeats = (128 * MB) / (ws * KB);
	for (i = 0; i < repeats; i++){
		int offset = (random() % repeats) * ws;
		if (lseek(fd, offset, SEEK_SET) == (off_t)-1){
			printf(SEEK_ERROR, argv[1], strerror(errno));
			return -1;
		}
		if (write(fd, buf, ws) == -1){
			printf(WRITE_ERROR, argv[1], strerror(errno));
			return -1;
		}
	}
	if (close(fd) != 0){
		printf(CLOSE_ERROR, argv[1], strerror(errno));
		return -1;
	}

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	printf(THROUGHPUT, elapsedTime);
	return 0;
}


int typeCheck(char* path){
	struct stat st;  //referance: http://stackoverflow.com/questions/24290273/check-if-input-file-is-a-valid-file-in-c
	if (stat(path, &st) == -1){
		if ((errno == ENOENT) && (strcmp(path, "") != 0)){
			int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
			if (fd == -1){
				printf(OPEN_ERROR, path, strerror(errno));
				return -1;
			}
			return writeRandom(fd, path);
		}
		else{
			printf(STAT_ERROR, path, strerror(errno));
			return -1;
		}
	}
	else{
		if (!S_ISBLK(st.st_mode)){
			if (S_ISLNK(st.st_mode) || (st.st_nlink > 1)){
				printf(LINK_ERROR);
				return -1;
			}
			if (st.st_size != (128 * MB)){
				int fd = open(path, O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
				return writeRandom(fd, path);
			}
		}
		else{
			printf(BLOCK_ERROR, path);
			return -1;
		}
	}
}


int writeRandom(int fd, char* path){
	int i, j;
	char buf[KB];
	for (i = 0; i < KB; i++){
		for (j = 0; j < KB; j++) buf[i] = 'a' + (random() % 26);
		if (write(fd, buf, KB) == -1){
			printf(WRITE_ERROR, path, strerror(errno));
			return -1;
		}
	}
	if (close(fd) == -1){
		printf(CLOSE_ERROR, path, strerror(errno));
		return -1;
	}
	return 1;
}

