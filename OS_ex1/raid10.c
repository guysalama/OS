#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <assert.h>
#include <errno.h> 
#include <string.h>
#include <linux/limits.h>

#define OPEN_ERROR "Error occurred while openning the device %d: %s\n"
#define SEEK_ERROR "Error occurred while seeking in the device %d: %s\n"
#define CLOSE_ERROR "Error occurred while closing the device %d: %s\n"
#define READ_ERROR "Error occurred while reading from device %d: %s\n"
#define WRITE_ERROR "Error occurred while writing to the device %d: %s\n"
#define SCANF_ERROR "Error occured while reading input command: %s\n"
#define BAD_DEVICES_ERROR "Operation on bad devices, data can’t be restored at all\n"
#define RAID_ERROR "Error occurred while operate on device: %s\n"
#define ARGS_ERROR "The program accepted %d arguments\n"
#define UNVALID_CALC "Unvalidate calculations"
#define DISK_ACCESSED "Operation on device %d, sector %d-%d\n"
#define OPEN_DEVICE "Opening device %d: %s\n"

//Constants
#define DEVICE_SIZE (1024*1024*256) // assume all devices identical in size
#define SECTOR_SIZE 512
#define SECTORS_PER_BLOCK 4
#define BLOCK_SIZE (SECTOR_SIZE * SECTORS_PER_BLOCK)
#define BUFFER_SIZE (BLOCK_SIZE * 2)

//Declarations
int do_raid10_rw(char* operation, int sector, int count);
int find_next_available_device(int raid0_idx, int prev_idx);
void close_diveces();
int error(char* msg, int details);
int raid10_rw_error(char* msg, int details);

//Globals
char	buf[BUFFER_SIZE];
int		num_dev, num_raid1_dev, num_raid0_dev;
int		*dev_fd;


int main(int argc, char** argv){
	if (argc < 4){
		printf(ARGS_ERROR, argc - 1);
		return -1;
	}
	int i;
	char line[1024];

	num_dev = argc - 2; // number of devices == number of arguments (ignore 1st) - 1
	num_raid1_dev = atoi(argv[1]);
	num_raid0_dev = num_dev / num_raid1_dev //assuming that the total num of devices is a multiple of the num of raid1 devices
	int _dev_fd[num_dev];
	dev_fd = _dev_fd;

	// open all devices
	for (i = 0; i < num_dev; ++i) {
		printf(OPEN_DEVICE, i, argv[i + 2]);
		dev_fd[i] = open(argv[i + 2], O_RDWR);
		if (dev_fd[i] == -1) return error(OPEN_ERROR, i);
	}

	// vars for parsing input line
	char operation[20];
	int sector;
	char countOrDev[PATH_MAX];

	// read input lines to get command of type "OP <SECTOR> <COUNT>"
	while (fgets(line, 1024, stdin) != NULL) {
		if (sscanf(line, "%s %d %d", operation, &sector, countOrDev) != 3) return error(SCANF_ERROR, -1);

		// KILL specified device
		else if (!strcmp(operation, "KILL")) {
			close(dev_fd[sector]); //assuming close always work
			dev_fd[sector] = -1;
		}
		// REPAIR
		else if (!strcmp(operation, "REPAIR")) {
			//close the old device
			if (dev_fd[sector] != -1){
				close(dev_fd[sector]); //assuming close always work
				dev_fd[sector] = -1;
			}
			//open a new one 
			dev_fd[sector] = open(countOrDev, O_RDWR);
			if (dev_fd[sector] == -1) return error(OPEN_ERROR, sector);
			//restore the data
			if (restore(sector) == -1) return error(WRITE_ERROR, sector);
		}
		// READ / WRITE
		else{
			if (do_raid10_rw(operation, sector, atoi(countOrDev)) == -1){
				close_diveces();
				return -1;
			}
		}
	}

	// close all the open devices
	close_diveces();
	return 0;
}

int restore(int dev_idx){
	ssize_t read_size, write_size;
	int raid0_idx = dev_idx / num_raid1_dev;
	int raid1_idx = find_next_available_device(raid0_idx, 0); //find available device to copy from
	while (raid1_idx != -1){
		off_t offset = lseek(dev_fd[dev_idx], 0, SEEK_CUR);
		lseek(dev_fd[raid0_idx + raid1_idx], offset, SEEK_SET);
		read_size = read(dev_fd[raid0_idx + raid1_idx], buf, BUFFER_SIZE);
		while (read_size > 0){
			write_size = write(dev_fd[dev_idx], buf, read_size);
			if (read_size != write_size){
				close(dev_fd[dev_idx]); //assuming close always work
				dev_fd[dev_idx] = -1;
				return -1;
			}
			read_size = read(dev_fd[raid0_idx + raid1_idx], buf, BUFFER_SIZE);
		}
		if (read_size == -1){
			close(dev_fd[dev_idx]; //assuming close always work
			dev_fd[raid0_idx + raid1_idx] = -1;
			raid1_idx = find_next_available_device(raid0_idx, 0);
		}
		else break; //  read_size == 0, EOF
	}
	/*if (raid1_idx == -1){
		printf(BAD_DEVICES_ERROR);
		return -1;
	}*/
	return 0;
}

int do_raid10_rw(char* operation, int sector, int count){
	int i = sector, idx = 0, raid1_idx, dev_idx;
	while (i < sector + count)
	{
		// find the relevant device for current sector
		int block_num = i / SECTORS_PER_BLOCK;
		int raid0_idx = block_num % num_raid0_dev;

		// find offset of sector inside device
		int block_start = i / (num_raid0_dev * SECTORS_PER_BLOCK);
		int block_off = i % SECTORS_PER_BLOCK;
		int sector_start = block_start * SECTORS_PER_BLOCK + block_off;
		int offset = sector_start * SECTOR_SIZE;

		// try to write few sectors at once
		int num_sectors = SECTORS_PER_BLOCK - block_off;
		while (i + num_sectors > sector + count) --num_sectors;
		int sector_end = sector_start + num_sectors - 1;
		int size = num_sectors * SECTOR_SIZE;

		// validate calculations
		if (num_sectors <= 0 || size > BUFFER_SIZE || offset + size > DEVICE_SIZE){
			printf(UNVALID_CALC);
			return -1;
		}

		// find available device
		raid1_idx = find_next_available_device(raid0_idx, 0); 
		if (raid1_idx == -1){ // data can’t be restored at all
			printf(BAD_DEVICES_ERROR);
			return -1;
		}
		
		// read from the device with the lowest index that is not fault
		if (!strcmp(operation, "READ")){ 
			dev_idx = raid0_idx + raid1_idx;
			if (lseek(dev_fd[dev_idx], offset, SEEK_SET) != offset) return raid10_rw_error(SEEK_ERROR, dev_idx);
			if (read(dev_fd[dev_idx], buf, size) != size) return raid10_rw_error(READ_ERROR, dev_idx); 
			printf(DISK_ACCESSED, dev_idx, sector_start, sector_end);
		}

		// write the data to all the available raid1 devices at the same raid0 device
		else if (!strcmp(operation, "WRITE")){
			while (raid1_idx > 0){  
				dev_idx = raid0_idx + raid1_idx;
				if (dev_fd[dev_idx] > 0){
					if (lseek(dev_fd[dev_idx], offset, SEEK_SET) != offset) return raid10_rw_error(SEEK_ERROR, dev_idx);
					if (write(dev_fd[dev_idx], buf, size) != size) return raid10_rw_error(WRITE_ERROR, dev_idx);
					printf(DISK_ACCESSED, dev_idx, sector_start, sector_end);
				}
				raid1_idx = find_next_available_device(raid0_idx, raid1_idx);
			}
		}
		i += num_sectors;
	}
	return 0;
}

int find_next_available_device(int raid0_idx, int prev_idx){
	int raid1_idx = prev_idx;
	if (prev_idx == num_raid1_dev) return -1;
	for (raid1_idx; raid1_idx <= num_raid1_dev; raid1_idx++){
		if (dev_fd[raid0_idx + raid1_idx] >= 0) break;
		if (j = num_raid1_dev) return -1;
	}
	return raid1_idx;
}


void close_diveces(){
	int i;
	for (i = 0; i < num_dev; i++) {
		if (dev_fd[i] >= 0) close(dev_fd[i]); //assuming close always work
	}
}

int error(char* msg, int details){
	if (details == -1) printf(msg, strerror(errno));
	else printf(msg, details, strerror(errno));
	close_diveces();
	return errno;
}

int raid10_rw_error(char* msg, int details){
	printf(msg, details, strerror(errno));
	return -1;
}