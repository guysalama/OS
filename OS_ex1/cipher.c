#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <linux/limits.h>

#define OPENDIR_ERROR "Error occurred while opening a directory: %s\n"
#define MKDIR_ERROR "Error occurred while creating a directory: %s\n"
#define OPEN_ERROR "Error occurred while openning a file: %s\n"
#define STAT_ERROR "Error occurred while getting file stat: %s\n"
#define CRYP_ERROR "Error occurred while encrypting or decrypting a file: %s\n"
#define CLOSE_ERROR "Error occurred while closing a file: %s\n"
#define CLOSEDIR_ERROR "Error occurred while closing a directory: %s\n"

int main(int argc, char** argv){
	int key, cryp, res;
	struct dirent *dp;
	struct stat statbuf;

	assert(argc == 4);
	DIR *cryp_dir = opendir(argv[1]);
	if (cryp_dir == NULL) return error(OPENDIR_ERROR);
	if (mkdir(argv[3]) != 0){
		if (errno != EEXIST) return error(MKDIR_ERROR);
	}
	DIR *res_dir = opendir(argv[3]);
	if (res_dir == NULL) return error(OPENDIR_ERROR);
	key = open(argv[2], O_RDONLY);
	if (key == -1) return error(OPEN_ERROR);
	char cryp_path[PATH_MAX], res_path[PATH_MAX];
	dp = readdir(cryp);
	while (dp != NULL){
		sprintf(cryp_path, "%s/%s", argv[1], dp->d_name); // get full path to the encrypted/decrypted file
		sprintf(res_path, "%s/%s", argv[3], dp->d_name); // get full path to the result file
		if (stat(cryp_path, &statbuf) == -1) return error(STAT_ERROR); // call stat to get file metadata
		if ((statbuf.st_mode & S_IFMT) == S_IFDIR) continue; // skip directories
		cryp = open(cryp_path, O_RDONLY);// open encrypted/decrypted file
		if (cryp == -1){ 
			printf(OPEN_ERROR, cryp_path, strerror(errno));
			return errno;
		}
		res = open(res_path, O_CREAT | O_TRUNC | O_RDWR);
		if (res == -1){
			printf(OPEN_ERROR, res_path, strerror(errno));
			return errno;
		}
		if (cryp_func(cryp, res, key) == -1) return error(CRYP_ERROR); // encrypting/decrypting the file
		lseek(key, 0, SEEK_SET); // return to the starting point of the key file
		if (close(cryp) == -1 || close(res) == -1) return error(CLOSE_ERROR);
		dp = readdir(cryp);
	}
	if (close(key) == -1) return error(CLOSE_ERROR);	
	if (closedir(cryp) == -1) return error(CLOSEDIR_ERROR);
}

int error(char* msg){
	printf(msg, strerror(errno));
	return errno;
}

int enc_dec(int src, int key, int res){
	int key_byte, file_byte = 4000, i, tmp;
	char key_buf[4001], file_buf[4001], res_buf[4001];
	key_buf[4000] = '\0';
	file_buf[4000] = '\0';
	res_buf[4000] = '\0';
	while (file_byte == 4000){
		if ((key_byte = read(key, key_buf, 4000)) == -1){
			printf("Error reading from key file: %s\n", strerror(errno));
			return 0;
		}
		if (key_byte != 4000) key_buf[key_byte] = '\0';
		tmp = strlen(key_buf);
		while (tmp < 4000){
			lseek(key, 0, SEEK_SET);
			if ((key_byte = read(key, &key_buf[tmp], 4000 - tmp)) == -1){
				printf("Error reading from key file: %s\n", strerror(errno));
				return 0;
			}
			tmp += key_byte;
			if (tmp != 4000) key_buf[tmp] = '\0';
		}

		if ((file_byte = read(src, file_buf, 4000)) == -1){
			printf("Error reading from file: %s\n", strerror(errno));
			return 0;
		}
		if (file_byte != 4000){
			file_buf[file_byte] = EOF;
			res_buf[file_byte] = EOF;
		}

		for (i = 0; i < file_byte; i++)
			res_buf[i] = key_buf[i] ^ file_buf[i];

		if (write(res, res_buf, file_byte) == -1){
			printf("Error writing to file: %s\n", strerror(errno));
			return 0;
		}
	}
	return 1;
}