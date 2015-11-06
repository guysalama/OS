//comm?
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>


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

int main(int argc, char** argv){
	setvbuf(stdout, NULL, _IONBF, 0);

	assert(argc == 4);
	DIR *src_dir = opendir(argv[1]);
	DIR *dst_dir = opendir(argv[3]);
	if (src_dir == NULL){
		printf("Error opening folder: %s\n", strerror(errno));
		return errno;
	}
	if (dst_dir == NULL){
		if (mkdir(argv[3], 0777) != 0){ //ask??
			printf("Error making folder: %s\n", strerror(errno));
			return errno;
		}
	}
	else if (closedir(dst_dir) != 0){ //ask liad??
		printf("Error closing directory: %s\n", strerror(errno));
		return errno;
	}

	struct dirent *entry;
	struct stat statbuf;
	int key_file, src_file, dst_file;
	char src_path[1000], dst_path[1000];
	if ((key_file = open(argv[2], O_RDONLY)) == -1){
		printf("Error opening key file: %s\n", strerror(errno));
		return errno;
	}
	while ((entry = readdir(src_dir)) != NULL){
		// get full path to src/dst files
		sprintf(src_path, "%s/%s", argv[1], entry->d_name);
		sprintf(dst_path, "%s/%s", argv[3], entry->d_name);
		// call stat to get file metadata
		if (stat(src_path, &statbuf) == -1){
			printf("Error getting file stat: %s\n", strerror(errno));
			return errno;
		}
		// skip directories
		if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		{
			continue;
		}

		// open both files, enc/dec and close files
		if ((src_file = open(src_path, O_RDONLY)) == -1 || (dst_file = open(dst_path, O_CREAT | O_TRUNC | O_RDWR, 0777)) == -1){
			printf("Error opening file: %s : %s\n", dst_path, strerror(errno));
			return errno;
		}

		if (enc_dec(src_file, key_file, dst_file) != 1){
			printf("Error in encryption/decryption: %s\n", strerror(errno));
			return errno;
		}
		// return to the begining of the key for the next file
		lseek(key_file, 0, SEEK_SET);

		if (close(src_file) != 0 || close(dst_file) != 0){
			printf("Error closing file: %s\n", strerror(errno));
			return errno;
		}
	}
	if (close(key_file) != 0 || closedir(src_dir) != 0){
		printf("Error closing file or directory: %s\n", strerror(errno));
		return errno;
	}
}