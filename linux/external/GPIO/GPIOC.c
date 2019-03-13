#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define SET_PATH     "/sys/bus/platform/drivers/rda-gpioc/rda-gpioc/gpo_set"
#define CLEAR_PATH   "/sys/bus/platform/drivers/rda-gpioc/rda-gpioc/gpo_clear"
#define PIN          "27"

void GPIO_SET(void)
{
	int fd;

	fd = open(SET_PATH, O_RDWR);
	if (fd < 0) {
		printf("%s can't open\n", SET_PATH);
		return;
	}
	printf("Succeed to open GPIO\n");

	write(fd, PIN, 2);

	close(fd);
}

void GPIO_CLEAR(void)
{
	int fd;

	fd = open(CLEAR_PATH, O_RDWR);
	if (fd < 0) {
		printf("%s can't open\n", CLEAR_PATH);
		return;
	}
	printf("Succeed to open GPIO\n");

	write(fd, PIN, 2);

	close(fd);
}

/*
 * Single write and read
 * Mode1
 */
int main()
{
	for (;;) {
		GPIO_SET();
		sleep(1);
		GPIO_CLEAR();
		sleep(1);
	}

    return 0;    
}
