/* YOUR FILE-HEADER COMMENT HERE */
/*
help from: 

	Carly Heiner
*/
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>

int main(void) {
	printf("Hello, world!\n");
	int fd,retval;
	int fileDesc;
	void *dest;
	char readBuf[];

	fd = Open("/dev/mm",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)

	dest = mmap(NULL, PAGE_SIZE,PROT_READ,MAP_SHARED,fd,0)

	if(dest == MAP_FAILED){
		printf("failed memory map");
		exit(1);
	}
	munmap(dest, PAGE_SIZE);

	close(fd);


	fileDesc = open("/dev/mm_ctl", O_RDWR);

	if(fileDesc < 0){
		printf("didnt open\n");
		return 0;
	}

	retVal = write(fileDesc, "start\n", 5);

	retVal = read(fileDesc, readBuf, 18);

	printf("read result %s\n", readBuf);

	close(fileDesc);

	return 0;
}
