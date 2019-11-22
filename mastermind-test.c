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
#include <errno.h>

int main(void) {
	printf("Hello, world!\n");
	int fd, retVal;
	int fileDesc;
	void *dest;
	char readBuf[100];

	fd = open("/dev/mm",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	dest = mmap(NULL, PAGE_SIZE,PROT_READ,MAP_SHARED,fd,0);

	if(dest == MAP_FAILED){
		printf("failed memory map");
		exit(1);
	}

	close(fd);

	//FIRST TEST
	printf("TEST: if new line start causes -EINVAL\n");
	fileDesc = open("/dev/mm_ctl", O_RDWR);
	if(fileDesc < 0){
		printf("/dev/mm_ctl didnt open\n");
		return 0;
	}
	retVal = write(fileDesc, "start\n", 6);
	if(errno == 22){
		printf("Correct value returned, Test passed\n");
	}


	printf("TEST: if incorrect sized guess causes -EINVAL\n");
	fileDesc = open("/dev/mm", O_RDWR);
	if(fileDesc < 0){
		printf("/dev/mm didnt open\n");
	}
	retVal = write(fileDesc, "123456",6);
	if(errno == 22){
		printf("correct value returned, Test passed\n");
	}

	printf("TEST: if guess 1234 return B1W2");
	retVal = write(fileDesc, "1234",4);

	printf("History: %s\n", (char *) dest);


	//retVal = strcmp()



	close(fileDesc);

	munmap(dest, PAGE_SIZE);

	return 0;
}
