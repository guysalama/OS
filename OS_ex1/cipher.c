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

#define OPENDIR_ERROR "Error occurred while opening directory: %s\n"
#define MKDIR_ERROR "Error occurred while creating directory: %s\n"
#define OPEN_ERROR "Error occurred while openning file: %s\n"
#define STAT_ERROR "Error occurred while getting file stat: %s\n"
#define CRYP_ERROR "Error occurred while encrypting or decrypting a file: %s\n"
#define CLOSE_ERROR "Error occurred while closing file: %s\n"
#define CLOSEDIR_ERROR "Error occurred while closing directory: %s\n"
#define READ_ERROR "Error occurred while reading from file: %s\n"
#define WRITE_ERROR "Error occurred while writing to file: %s\n"
#define BUF_SIZE 4000

//Declarations
int cryp_func(int cryp, int key, int res);
int error(char* msg);
int cryp_error(char* msg);

int main(int argc, char** argv){
	int key, cryp, res;
	struct dirent *dp;
	struct stat statbuf;

	assert(argc == 4);
	DIR *cryp_dir = opendir(argv[1]);
	if (cryp_dir == NULL) return error(OPENDIR_ERROR);
	if (mkdir(argv[3], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0){
		if (errno != EEXIST) return error(MKDIR_ERROR);
	}
	DIR *res_dir = opendir(argv[3]);
	if (res_dir == NULL) return error(OPENDIR_ERROR);
	key = open(argv[2], O_RDONLY);
	if (key == -1) return error(OPEN_ERROR);
	char cryp_path[PATH_MAX], res_path[PATH_MAX];
	dp = (struct dirent*) readdir(cryp);
	while (dp != NULL){
		sprintf(cryp_path, "%s/%s", argv[1], dp->d_name); // get full path to the encrypted/decrypted file
		sprintf(res_path, "%s/%s", argv[3], dp->d_name); // get full path to the result file
		if (stat(cryp_path, &statbuf) == -1) return error(STAT_ERROR); // call stat to get file metadata
		if ((statbuf.st_mode & S_IFMT) == S_IFDIR) continue; // skip directories
		cryp = open(cryp_path, O_RDONLY);// open encrypted/decrypted file
		if (cryp == -1){ 
			printf(OPEN_ERROR, strerror(errno));
			return errno;
		}
		res = open(res_path, O_CREAT | O_TRUNC | O_RDWR);
		if (res == -1){
			printf(OPEN_ERROR, strerror(errno));
			return errno;
		}
		if (cryp_func(cryp, res, key) == -1) return error(CRYP_ERROR); // encrypting/decrypting the file
		lseek(key, 0, SEEK_SET); // return to the starting point of the key file
		if (close(cryp) == -1 || close(res) == -1) return error(CLOSE_ERROR);
		dp = (struct dirent*) readdir(cryp);
	}
	if ((int) close(key) == -1) return error(CLOSE_ERROR);	
	if ((int) closedir(cryp) == -1) return error(CLOSEDIR_ERROR);
}

int cryp_func(int cryp, int key, int res){
	int key_cnt, cryp_cnt, idx, i;
	char key_buf[BUF_SIZE + 1], cryp_buf[BUF_SIZE + 1], res_buf[BUF_SIZE + 1]; // read first 4000 characters
	key_buf[BUF_SIZE] = '\0'; // string closer
	cryp_buf[BUF_SIZE] = '\0';
	res_buf[BUF_SIZE] = '\0';
	do{
		key_cnt = read(key, key_buf, 4000);
		if (key_cnt == -1) return cryp_error(READ_ERROR);
		if (key_cnt != BUF_SIZE) key_buf[key_cnt] = '\0';
		idx = strlen(key_buf);
		while (idx < BUF_SIZE){
			lseek(key, 0, SEEK_SET);
			key_cnt = read(key, &key_buf[idx], BUF_SIZE - idx);
			if (key_cnt == -1) return cryp_error(READ_ERROR);
			idx += key_cnt;
			if (idx < BUF_SIZE) key_buf[idx] = '\0';
		}

		cryp_cnt = read(cryp, cryp_buf, BUF_SIZE);
		if (cryp_cnt == -1) return cryp_error(READ_ERROR);
		if (cryp_cnt < BUF_SIZE){
			cryp_buf[cryp_cnt] = EOF;
			res_buf[cryp_cnt] = EOF;
		}

		for (i = 0; i < cryp_cnt; i++) res_buf[i] = key_buf[i] ^ cryp_buf[i];
		if (write(res, res_buf, cryp_cnt) == -1) return cryp_error(WRITE_ERROR);
	} while (cryp_cnt = BUF_SIZE);
	return 0;
}

int error(char* msg){
	printf(msg, strerror(errno));
	return errno;
}

int cryp_error(char* msg){
	printf(msg, strerror(errno));
	return -1;
}