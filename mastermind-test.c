/* YOUR FILE-HEADER COMMENT HERE */

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

	int fileDesc;

	fileDesc = open('/dev/mm_ctl', O_RDWR);
	if(fileDesc < 0){
		return 0;
	}

	write(fileDesc, "start", 5);

	close(fileDesc);

	return 0;
}
