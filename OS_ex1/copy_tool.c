#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>


#define ARGS_ERROR "The program accepts just two command-line arguments\n"
#define ARG_ERROR "The given path %s is not valid: %s\n"
#define FUNC_ERROR "Error occurred while running the function %s: %s\n"
#define BUFF_SIZE 4096


int main(int argc, char ** argv){
	if (argc != 3) {
		printf(ARGS_ERROR);
		return -1;
	}
	struct stat st;
	char *src_path = argv[1], *dst_path = argv[2], *src_addr, *dst_addr;
	int src_fd, dst_fd, src_size, offset = 0, i;
	src_fd = open(src_path, O_RDWR, 0777);
	if (src_fd == -1){
		printf(FUNC_ERROR, "open", strerror(errno));
		return -1;
	}
	dst_fd = open(dst_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
	if (dst_fd == -1){
		printf(FUNC_ERROR, "open", strerror(errno));
		close(dst_fd);
		return -1;
	}
	if (stat(src_path, &st) == -1){
		printf(FUNC_ERROR, "stat", strerror(errno));
		close(src_fd);
		close(dst_fd);
		return -1;
	}
	src_size = st.st_size;
	if (truncate(dst_path, src_size) == -1){
		printf(FUNC_ERROR, "truncate", strerror(errno));
		close(src_fd);
		close(dst_fd);
		return -1;
	}
	
	while (offset < src_size){
		src_addr = mmap(NULL, BUFF_SIZE, PROT_READ, MAP_SHARED, src_fd, offset);
		dst_addr = mmap(NULL, BUFF_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, dst_fd, offset);
		if (src_addr == MAP_FAILED | dst_addr == MAP_FAILED){
			printf(FUNC_ERROR, "mmap", strerror(errno));
			close(src_fd);
			close(dst_fd);
			return -1;
		}
		for (i = 0; i < BUFF_SIZE; i++){
			dst_addr[i] = src_addr[i];
			if (src_addr[i] == EOF) break;
		}
		if (munmap(src_addr, BUFF_SIZE) == -1 || munmap(dst_addr, BUFF_SIZE) == -1){
			printf(FUNC_ERROR, "munmap", strerror(errno));
			close(src_fd);
			close(dst_fd);
			return -1;
		}
		offset += i;
	}
	close(src_fd);
	close(dst_fd);
	return 0;
}