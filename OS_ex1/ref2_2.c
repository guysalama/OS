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
int		num_raid0_dev;
int		num_raid1_dev;
int		*dev_fd;

void do_raid0_rw(char* operation, int sector, int count)
{
	int i = sector;

	while (i < sector + count)
	{
		// find the relevant device for current sector
		int block_num = i / SECTORS_PER_BLOCK;
		int dev_num = block_num % num_raid0_dev;

		// find offset of sector inside device
		int block_start = i / (num_raid0_dev * SECTORS_PER_BLOCK);
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

		do_raid1_rw(operation, dev_num, offset, size, sector_start, sector_end);

		i += num_sectors;
	}
}

int do_raid1_rw(char* operation, int dev_num, int offset, int size, int sector_start, int sector_end){
	int i = 0;
	while (dev_fd[dev_num + i] < 0 && i < num_raid1_dev) i++;
	if (i == num_raid1_dev){
		printf("Operation on bad device %d\n", dev_num);
		return 0;
	}
	if (!strcmp(operation, "READ")){
		assert(offset == lseek(dev_fd[dev_num + i], offset, SEEK_SET));
		assert(size == read(dev_fd[dev_num + i], buf, size));
		printf("Operation on device %d, sector %d-%d\n",
			dev_num + i, sector_start, sector_end);
	}
	else if (!strcmp(operation, "WRITE")){
		for (i; i < num_raid1_dev; i++){
			if (dev_fd[dev_num + i] > 0){
				assert(offset == lseek(dev_fd[dev_num + i], offset, SEEK_SET));
				assert(size == write(dev_fd[dev_num + i], buf, size));
				printf("Operation on device %d, sector %d-%d\n",
					dev_num + i, sector_start, sector_end);
			}
		}
	}

}

int restore_data(int dev){
	int raid0_dev = dev / num_raid1_dev;
	int i = 0;
	ssize_t read_b, written_b;
	for (i = 0; i < num_raid1_dev; i++){
		if ((raid0_dev + i) != dev && dev_fd[raid0_dev + i] > 0){
			// continue from where we stopped
			lseek(dev_fd[raid0_dev + i], lseek(dev_fd[dev], 0, SEEK_CUR), SEEK_SET);
			// dup content
			read_b = read(dev_fd[raid0_dev + i], buf, BUFFER_SIZE)
				while (read_b > 0){
				if (read_b != write(dev_fd[dev], buf, read_b)){
					// kill new dev and return err
					assert(!close(dev_fd[dev]));
					dev_fd[dev] = -1;
					return 0;
				}
				read_b = read(dev_fd[raid0_dev + i], buf, BUFFER_SIZE)
				}
			if (read_b != 0){
				// error in dev copied from, kill and continue to next
				assert(!close(dev_fd[raid0_dev + i]));
				dev_fd[raid0_dev + i] = -1;
			}
			else return 1; // got to EOF
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	assert(argc > 3);

	int i;
	char line[1024];

	// number of devices == number of arguments (ignore 1st)
	num_raid1_dev = atoi(argv[1]);
	num_dev = argc - 2;
	num_raid0_dev = num_dev / num_raid1_dev;
	int _dev_fd[num_dev];
	dev_fd = _dev_fd;

	// open all devices
	for (i = 0; i < num_dev; ++i) {
		printf("Opening device %d: %s\n", i, argv[i + 2]);
		dev_fd[i] = open(argv[i + 2], O_RDWR);
		assert(dev_fd[i] >= 0);
	}

	// vars for parsing input line
	char operation[20];
	char new_dev[100];
	int sector;
	int count;

	// read input lines to get command of type "OP <SECTOR> <COUNT>"
	while (fgets(line, 1024, stdin) != NULL) {
		assert(sscanf(line, "%s %d %s", operation, &sector, new_dev) == 3);

		// KILL specified device
		if (!strcmp(operation, "KILL")) {
			assert(!close(dev_fd[sector]));
			dev_fd[sector] = -1;
		}
		// REPAIR specified device
		else if (!strcmp(operation, "REPAIR")) {
			if (dev_fd[sector] != -1){
				assert(!close(dev_fd[sector]));
				dev_fd[sector] = -1;
			}

			dev_fd[sector] = open(new_dev, O_RDWR);
			assert(dev_fd[sector] >= 0);
			restore_data(sector);
		}
		// READ / WRITE
		else {
			count = atoi(new_dev);
			do_raid0_rw(operation, sector, count);
		}
	}

	for (i = 0; i < argc - 1; i++) {
		if (dev_fd[i] >= 0)
			assert(!close(dev_fd[i]));
	}
}