#define _GNU_SOURCE
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
#include <time.h>


int main(int argc, char** argv){
	assert(argc == 4);
	assert(validate_file(argv[1]));
	int w_size = atoi(argv[3]) * 1000;
	int i, spot, segs = 128000000 / w_size;  // ask liad why not /w_size*1000
	static char buf[1024 * 1024] __attribute__((__aligned__(4096)));
	buf[0] = random();
	for (i = 1; i < 262144; i++) buf[i * 4 - 1] = random();

	struct timeval start, end;
	long mtime, seconds, useconds;

	// start time measurement
	gettimeofday(&start, NULL);

	int fd;
	if ((fd = open(argv[1], O_WRONLY, 0777)) == -1){
		printf("Error opening file %s : %s\n", argv[1], strerror(errno));
		return 0;
	}
	if (atoi(argv[2])) fcntl(fd, F_SETFL, O_WRONLY | O_DIRECT);

	for (i = 0; i < segs; i++){
		spot = (random() % segs) * w_size;
		if (lseek(fd, spot, SEEK_SET) == (off_t)-1){
			printf("Error seeking in file: %s\n", strerror(errno));
			return errno;
		}
		if (write(fd, buf, w_size) == -1){
			printf("Error writing to file: %s\n", strerror(errno));
			return errno;
		}
	}

	if (close(fd) != 0){
		printf("Error closing file: %s\n", strerror(errno));
		return errno;
	}

	gettimeofday(&end, NULL);

	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds)* 1000 + useconds / 1000.0) + 0.5;
	printf("Elapsed time: %ld milliseconds (%ld useconds)\n", mtime, useconds); // delete before submitting
	printf("This run's throughput is %f\n", 128000 / (double)mtime);

}

int validate_file(char* path){
	struct stat statbuf;
	if (stat(path, &statbuf) == 0){
		// file exists
		if (!S_ISBLK(statbuf.st_mode)){
			if (S_ISLNK(statbuf.st_mode)){
				printf("The file is a symbolic link, exiting..");
				return 0;
			}
			if (statbuf.st_nlink > 1){
				printf("The file has too many hard links, exiting..");
				return 0;
			}
			if (statbuf.st_size != 128 * 1000 * 1000){
				int fd;
				if ((fd = open(path, O_TRUNC | O_WRONLY, 0777)) == -1){
					printf("Error opening file: %s : %s\n", path, strerror(errno));
					return 0;
				}
				return fill_rnd(fd);
			}
		}
	}
	else{
		if (errno == ENOENT){
			int fd;
			if ((fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0777)) == -1){
				printf("Error opening file: %s : %s\n", path, strerror(errno));
				return 0;
			}
			return fill_rnd(fd);
		}
		else{
			strerror(errno);
			return 0;
		}
	}
}

int fill_rnd(int fd){
	long buf[1000], i, j;
	for (j = 0; j < 32000; j++){
		for (i = 0; i < 1000; i++) buf[i] = random();
		if (write(fd, buf, 1000) == -1){
			printf("Error writing to file: %s\n", strerror(errno));
			return 0;
		}
	}
	return (close(fd) == 0);
}