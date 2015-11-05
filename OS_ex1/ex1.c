//5555
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h> 
#include <string.h>

// print first 10 characters of text file
// or list contents of directory
int main(int argc, char** argv)
{
	// assert first argument directory/filename
	assert(argc == 2);
#if 1
	int fd = open(argv[1], O_RDWR);

	if (fd < 0){
		printf("Error opening file: %s\n", strerror(errno));
		return errno;
	}
	// read first 10 characters
	char buf[11];
	buf[10] = '\0'; // string closer
	assert(read(fd, buf, 10) == 10);
	// we assume this is indeed a text file with readable characters...
	printf("First 10 characters are '%s'\n", buf);
	assert(!close(fd)); // close file


#else
	struct timeval start, end;
	long mtime, seconds, useconds;

	struct dirent *dp;
	DIR *dfd;

	char *dir;
	dir = argv[1];

	// open directory stream
	assert((dfd = opendir(dir)) != NULL);
	char filename_qfd[100];
	// read stream entries
	printf("list directory contents\n");
	// start time measurement
	gettimeofday(&start, NULL);
	while ((dp = readdir(dfd)) != NULL)
	{
		// full path to file
		sprintf(filename_qfd, "%s/%s", dir, dp->d_name);

		// call stat to get file metadata
		struct stat statbuf;
		assert(stat(filename_qfd, &statbuf) != -1);

		// skip directories
		if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		{
			continue;
		}

		printf("%s\n", filename_qfd);
	}

	closedir(dfd);

	// end time measurement and print result
	gettimeofday(&end, NULL);
	seconds = end.tv_sec - start.tv_sec;
	useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds)* 1000 + useconds / 1000.0) + 0.5;
	printf("Elapsed time: %ld milliseconds (%ld useconds)\n", mtime, useconds);
#endif
}