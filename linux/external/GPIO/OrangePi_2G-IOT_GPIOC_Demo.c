#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SET_PATH     "/sys/bus/platform/drivers/rda-gpioc/rda-gpioc/gpo_set"
#define CLEAR_PATH   "/sys/bus/platform/drivers/rda-gpioc/rda-gpioc/gpo_clear"

void GPIO_SET(void)
{
	FILE *fp = NULL;
	char *data = "27";

	fp = fopen(SET_PATH, "r+");
	if (fp == NULL) {
		printf("%s can't open\n", SET_PATH);
		return;
	}
	printf("Succeed to open GPIO\n");

	fwrite(&data, strlen(data), 1, fp);

	fclose(fp);
	fp = NULL;
}

void GPIO_CLEAR(void)
{
	FILE *fp = NULL;
	char *data = "27";

	fp = fopen(SET_PATH, "r+");
	if (fp == NULL) {
		printf("%s can't open\n", CLEAR_PATH);
		return;
	}
	printf("Succeed to open GPIO\n");

	fwrite(&data, strlen(data), 1, fp);

	fclose(fp);
	fp = NULL;
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
