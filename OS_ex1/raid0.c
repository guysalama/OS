// usage example:
// sudo ./raid0 /dev/sdb /dev/sdc

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <assert.h>
#include <errno.h> 
#include <string.h>

#define DEVICE_SIZE (1024*1024*256) // assume all devices identical in size

#define SECTOR_SIZE 512
#define SECTORS_PER_BLOCK 4
#define BLOCK_SIZE (SECTOR_SIZE * SECTORS_PER_BLOCK)

#define BUFFER_SIZE (BLOCK_SIZE * 2)

char	buf[BUFFER_SIZE];
int		num_dev;
int		*dev_fd;

void do_raid0_rw(char* operation, int sector, int count)
{
	int i = sector;

	while (i < sector + count)
	{
		// find the relevant device for current sector
		int block_num = i / SECTORS_PER_BLOCK;
		int dev_num = block_num % num_dev;

		// make sure device didn't fail
		if (dev_fd[dev_num] < 0) {
			printf("Operation on bad device %d\n", dev_num);
			break;
		}

		// find offset of sector inside device
		int block_start = i / (num_dev * SECTORS_PER_BLOCK);
		int block_off = i % SECTORS_PER_BLOCK;
		int sector_start = block_start * SECTORS_PER_BLOCK + block_off;
		int offset = sector_start * SECTOR_SIZE;

		// try to write few sectors at once
		int num_sectors = SECTORS_PER_BLOCK - block_off;
		while (i + num_sectors > sector + count)
			--num_sectors;
		int sector_end = sector_start + num_sectors - 1;
		int size = num_sectors * SECTOR_SIZE;

		// validate calculations
		assert(num_sectors > 0);
		assert(size <= BUFFER_SIZE);
		assert(offset + size <= DEVICE_SIZE);

		// seek in relevant device
		assert(offset == lseek(dev_fd[dev_num], offset, SEEK_SET));

		if (!strcmp(operation, "READ"))
			assert(size == read(dev_fd[dev_num], buf, size));
		else if (!strcmp(operation, "WRITE"))
			assert(size == write(dev_fd[dev_num], buf, size));

		printf("Operation on device %d, sector %d-%d\n",
			dev_num, sector_start, sector_end);

		i += num_sectors;
	}
}

int main(int argc, char** argv)
{
	assert(argc > 2);

	int i;
	char line[1024];

	// number of devices == number of arguments (ignore 1st)
	num_dev = argc - 1;
	int _dev_fd[num_dev];
	dev_fd = _dev_fd;

	// open all devices
	for (i = 0; i < num_dev; ++i) {
		printf("Opening device %d: %s\n", i, argv[i + 1]);
		dev_fd[i] = open(argv[i + 1], O_RDWR);
		assert(dev_fd[i] >= 0);
	}

	// vars for parsing input line
	char operation[20];
	int sector;
	int count;

	// read input lines to get command of type "OP <SECTOR> <COUNT>"
	while (fgets(line, 1024, stdin) != NULL) {
		assert(sscanf(line, "%s %d %d", operation, &sector, &count) == 3);

		// KILL specified device
		if (!strcmp(operation, "KILL")) {
			assert(!close(dev_fd[sector]));
			dev_fd[sector] = -1;
		}
		// READ / WRITE
		else {
			do_raid0_rw(operation, sector, count);
		}
	}

	for (i = 0; i < argc - 1; i++) {
		if (dev_fd[i] >= 0)
			assert(!close(dev_fd[i]));
	}
}