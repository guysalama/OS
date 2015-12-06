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
#include <sys/mman.h>


void clean_exit(char *src, char *dst, int fd_src, int fd_dst, int buf_size){
	if (buf_size != 0) printf("Got error: %s, closing files.\n", strerror(errno));
	if (src != NULL) munmap(src, buf_size);
	if (dst != NULL) munmap(dst, buf_size);
	close(fd_src);
	close(fd_dst);
	exit(0);
}

int main(int argc, char** argv){
	assert(argc == 3);
	char *src_path = argv[1];
	char *dst_path = argv[2];
	int fd_src, fd_dst;
	int file_size, buf_size = sysconf(_SC_PAGE_SIZE);
	if (2048 % buf_size == 0) buf_size = 2048;
	int i, err, offset = 0;
	char *src, *dst;

	if ((fd_src = open(src_path, O_RDWR, 0777)) == -1){
		printf("Error opening file: %s : %s\n", src_path, strerror(errno));
		return 0;
	}
	if ((fd_dst = open(dst_path, O_RDWR | O_CREAT | O_TRUNC, 0777)) == -1){
		printf("Error opening file: %s : %s\n", dst_path, strerror(errno));
		close(fd_src);
		return 0;
	}

	if ((file_size = lseek(fd_src, 0, SEEK_END)) == -1) clean_exit(NULL, NULL, fd_src, fd_dst, buf_size);
	if (ftruncate(fd_dst, file_size) == -1){
		printf("Error truncating file: %s : %s\n", dst_path, strerror(errno));
		clean_exit(NULL, NULL, fd_src, fd_dst, buf_size);
		return 0;
	}
	if (lseek(fd_src, 0, SEEK_SET) == -1) clean_exit(NULL, NULL, fd_src, fd_dst, buf_size);

	while (offset < file_size){

		src = mmap(NULL, buf_size, PROT_READ, MAP_SHARED, fd_src, offset);
		dst = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dst, offset);
		if ((int)src == -1 || (int)dst == -1) clean_exit(src, dst, fd_src, fd_dst, buf_size);

		for (i = 0; i < buf_size; i++){
			if (src[i] == EOF){
				dst[i] = src[i];
				break;
			}
			else dst[i] = src[i];
		}

		offset += buf_size;

		if (munmap(src, buf_size) == -1 || munmap(dst, buf_size)) clean_exit(src, dst, fd_src, fd_dst, buf_size);
	}

	clean_exit(NULL, NULL, fd_src, fd_dst, 0);


	return 0;
}