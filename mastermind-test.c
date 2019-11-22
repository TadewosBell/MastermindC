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
	int fileDesc, mmDesc;
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
	printf("TEST1: if new line start causes -EINVAL\n");
	fileDesc = open("/dev/mm_ctl", O_RDWR);
	if(fileDesc < 0){
		printf("/dev/mm_ctl didnt open\n");
		return 0;
	}
	retVal = write(fileDesc, "start\n", 6);
	if(errno == 22){
		printf("Correct value returned, Test1 passed\n");
	}
	close(fileDesc);

	printf("TEST2: if incorrect sized guess causes -EINVAL\n");
	fileDesc = open("/dev/mm", O_RDWR);
	if(fileDesc < 0){
		printf("/dev/mm didnt open\n");
	}
	retVal = write(fileDesc, "123456",6);
	if(errno == 22){
		printf("correct value returned, Test2 passed\n");
	}

	printf("TEST3: if incorrect guess characters causes like K -EINVAL\n");

	retVal = write(fileDesc, "1k23",4);
	if(errno == 22){
		printf("correct value returned, Test3 passed\n");
	}
	close(fileDesc);

	//retVal = strcmp()
	printf("Test4: Check if guess when game inactive causes -EINVAL\n");
	fileDesc = open("/dev/mm_ctl", O_RDWR);
	mmDesc = open("/dev/mm", O_RDWR);

	retVal = write(fileDesc, "quit", 4);
	retVal = write(mmDesc, "1234",4);
	if(errno == 22){
		printf("-EINVAL resturned, Test4 passed\n");
	}

	printf("Test5: check if correct output is saved userview when guess\n");
	retVal = write(fileDesc, "start", 5);
	retVal = write(mmDesc, "1234",4);
	char expString[] = "Guess 1: 1234 | B1W2";
	int strCmpVal = strcmp(expString,(char *)dest,20);

	printf("String comp %d\n", strCmpVal);
	printf("string history %s\n", (char *)dest)

	close(fileDesc);
	close(mmDesc);

	munmap(dest, PAGE_SIZE);

	return 0;
}
