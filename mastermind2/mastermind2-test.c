#include "cs421net.h"

/* YOUR CODE HERE */
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <errno.h>
#include <string.h>

int main(void) {
	/* Here is an example of sending some data to CS421Net */
	cs421net_init();
	cs421net_send("4442", 4);
    
	/* YOUR CODE HERE */
	printf("Unit tests!\n");
	int fd, retVal;
	int fileDesc, mmDesc;
	void *dest;
	char readBuff[100];

	fd = open("/dev/mm",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	dest = mmap(NULL, PAGE_SIZE,PROT_READ,MAP_SHARED,fd,0);

	if(dest == MAP_FAILED){
		printf("failed memory map");
		return 0;
	}

	close(fd);
	printf("Test 1: Test if game wasnt started ???? is returned\n");
	mmDesc = open("/dev/mm", O_RDONLY);

	
	retVal = read(mmDesc, readBuff, 4);
	readBuff[4] = '\0';

	if(strcmp(readBuff, "????") == 0){
		printf("correct ???? returned, Test 1 passed\n");
	}



	//second TEST
	printf("TEST2: if new line start causes -EINVAL\n");
	fileDesc = open("/dev/mm_ctl", O_RDWR);
	if(fileDesc < 0){
		printf("/dev/mm_ctl didnt open\n");
		return 0;
	}
	retVal = write(fileDesc, "start\n", 6);
	if(errno == 22){
		printf("Correct value returned, Test2 passed\n");
	}
	close(fileDesc);

	printf("TEST3: if incorrect sized guess causes -EINVAL\n");
	fileDesc = open("/dev/mm", O_RDWR);
	if(fileDesc < 0){
		printf("/dev/mm didnt open\n");
	}
	retVal = write(fileDesc, "123456",6);
	if(errno == 22 && retVal){
		printf("correct value returned, Test3 passed\n");
	}

	printf("TEST4: if incorrect guess characters like K causes -EINVAL\n");

	retVal = write(fileDesc, "1k23",4);
	if(errno == 22){
		printf("correct value returned, Test4 passed\n");
	}
	close(fileDesc);

	//retVal = strcmp()
	printf("Test5: Check if guess when game inactive causes -EINVAL\n");
	fileDesc = open("/dev/mm_ctl", O_RDWR);
	mmDesc = open("/dev/mm", O_RDWR);

	retVal = write(fileDesc, "quit", 4);
	retVal = write(mmDesc, "1234",4);
	if(errno == 22){
		printf("-EINVAL returned, Test5 passed\n");
	}

	printf("Test6: check if correct output is saved userview when guess\n");
	retVal = write(fileDesc, "start", 5);
	retVal = write(mmDesc, "1234",4);
	char expString[] = "Guess 1: 1234 | B1W2 \n";
	int strCmpVal = strcmp(expString,(char *)dest);

	if(strCmpVal == 0){
		printf("history and mmap function correctly, Test 6 passed\n");
	}
	close(fileDesc);
	close(mmDesc);


	printf("Test 7: Test if i can read bast last_result\n");
	mmDesc = open("/dev/mm", O_RDONLY);

	
	retVal = read(mmDesc, readBuff, 20);
	readBuff[4] = '\0';

	if(strcmp(readBuff, "B1W2") == 0){
		printf("correct amount of char (4) was read and not more, Test 7 passed\n");
	}

	close(mmDesc);

	munmap(dest, PAGE_SIZE);

	return 0;
}