/* Name: Tadewos Bellete
   Email: bell6@umbc.edu
   Description: mastermind game but written with device drivers

   Citations:
   https://www.tutorialspoint.com/c_standard_library/c_function_strncmp.htm
   https://stackoverflow.com/questions/16955936/string-termination-char-c-0-vs-char-c-0
   http://man7.org/linux/man-pages/man3/errno.3.html
   https://www.avrfreaks.net/forum/convert-int-char-0
   https://www.csee.umbc.edu/~jtang/cs421.f19/homework/hw4/hw4_test.c
 */
#include "cs421net.h"
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
    
	/* YOUR CODE HERE */
	printf("Unit tests!\n");
	int fd, retVal;
	int fileDesc, mmDesc;
	void *dest;
	char readBuff[PAGE_SIZE];

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


	printf("Test 7: Test if i can read past last_result\n");
	mmDesc = open("/dev/mm", O_RDONLY);

	
	retVal = read(mmDesc, readBuff, 20);
	readBuff[4] = '\0';

	if(strcmp(readBuff, "B1W2") == 0){
		printf("correct amount of char (4) was read and not more, Test 7 passed\n");
	}

	close(mmDesc);
    //sending new color code to increment number of changes
    cs421net_send("4442", 4);

    mmDesc = open("/sys/devices/platform/mastermind/stats", O_RDONLY);

	
	retVal = read(mmDesc, readBuff, 300);
	readBuff[300] = '\0';
    printf("%s \n", readBuff);
    strCmpVal = strcmp(readBuff, "CS421 Mastermind Stats\nNumber of colors: 6\nNumber of Active Games: 1\nNumber of Games: 1\nNumber of times code was changed: 1\nNumber of invalid code change attempts: 0\n");
    printf("string compare %d\n", strCmpVal);

    if(strCmpVal == 0){
        printf("Test 8 passed: color code changed one time.");
    }

    int numColors,activegames,games,codeChanges,attempts;
    uid_t userId, effId;
    userId = getuid();
    effId = geteuid();
    printf("user Id of the calling process %ld\n effective id %ld\n", (long)userId, (long)effId);
    sscanf(readBuff, "CS421 Mastermind Stats\nNumber of colors: %d\nNumber of Active Games: %d\nNumber of Games: %d\nNumber of times code was changed: %d\nNumber of invalid code change attempts: %d\n",&numColors, &activegames, &games, &codeChanges, &attempts);

    if(codeChanges == 1){
        printf("Test 9: passed, you have changed the color code by signaling an interrupt\n");
    }
    close(mmDesc);
    fileDesc = open("/dev/mm_ctl", O_RDWR);
    retVal = write(fileDesc, "colors 1", 8);
    if(errno == 13){
        printf("Test 10 passed: You tried to set color with out the right priviledges\n");
    }else if(errno == 22){
        printf("SUDO Test 11 passed: tried to set invalid number\n");
    }
    close(fileDesc);

    fileDesc = open("/dev/mm_ctl", O_RDWR);
    retVal = write(fileDesc, "colors 8", 8);

    if(errno != 13){
        printf("SUDO Test 12 passed: You set the color with the correct priveledge\n");
    }
    mmDesc = open("/sys/devices/platform/mastermind/stats", O_RDONLY);
    retVal = write(fileDesc, "start", 5);
    retVal = setuid(1000);
    retVal = write(fileDesc, "start",5);
    retVal = read(mmDesc, readBuff, 300);
	readBuff[300] = '\0';
    printf("%s \n", readBuff);
    strCmpVal = strcmp(readBuff, "CS421 Mastermind Stats\nNumber of colors: 8\nNumber of Active Games: 2\nNumber of Games: 2\nNumber of times code was changed: 2\nNumber of invalid code change attempts: 0\n");
    printf("string compare %d\n", strCmpVal);
    if(strCmpVal == 0){
        printf("SUDO Test 13 passed: two games started, with two uids 0 and 1000\n");
    }

    close(fileDesc);
	
    munmap(dest, PAGE_SIZE);
    

	return 0;
}