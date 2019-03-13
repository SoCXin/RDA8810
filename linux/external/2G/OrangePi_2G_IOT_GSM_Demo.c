/*
 * OrangePi 2G-IOT GSM Demo 
 *  (C) Copyright 2017 OrangePi
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NR_CITY  30
#define MODEM_PATH  "/dev/modem0"
#define VERSION     "0.1.0"

struct Centry_number {
	char *city;
	char *number;
} City_Number[NR_CITY] = {
	{ "ShenZhen",           "13010888500" },
	{ "Beijing",            "13010112500" },
	{ "Shanghai",           "13010314500" },
	{ "Shandong",           "13010171500" },
	{ "Jiangsu" ,           "13010341500" },
	{ "Zhejiang",           "13010360500" },
	{ "Fujian",             "13010380500" },
	{ "Sichuan",            "13010811500" },
	{ "Chongqing",          "13010831500" },
	{ "Hainan" ,            "13010501500" },
	{ "Heilongjiang",       "13010980500" },
	{ "Jilin",              "13010911500" },
	{ "Tianjin",            "13010130500" },
	{ "Hebei",              "13010180500" },
	{ "Inner Mongolia",     "13010950500" },
	{ "Shanxi",             "13010701500" },
	{ "Anhui",              "13010305500" },
	{ "Xinjiang",           "13010969500" },
	{ "Qinghai",            "13010776500" },
	{ "Gansu",              "13010879500" },
	{ "Ningxia",            "13010796500" },
	{ "Guizhou",            "13010788500" },
	{ "Yunnan",             "13010868500" },
	{ "Hunan",              "13010731500" },
	{ "Hubei",              "13010710500" },
	{ "Guangdong",          "13010200500" },
	{ "Guangxi",            "13010591500" },
	{ "Henan",              "13010761500" },
	{ "Jiangxi",            "13010720500" },
	{ "Liaoning",           "13010240500"},
};

/*
 * Initialize serial  
 */
void serial_init(int fd)
{
	struct termios options;

	tcgetattr(fd, &options);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CSIZE;
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CS8;
	options.c_cflag &= ~CSTOPB;
	options.c_iflag |= IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	tcsetattr(fd, TCSANOW, &options);
}

void display_message(int direction, const char *message)
{
	if (direction) {
		printf("Send Message ------> %s\n", MODEM_PATH);
		printf(">> %s\n", message);
	} else {
		printf("Rece Message <------ %s\n", MODEM_PATH);
		printf("<< %s\n", message);
	}
}

void Send_AT(int fd, const char *str1, const char *str2, const char *str3)
{
	char buff[128];
	char answer[128];

	memset(buff, 0, sizeof(buff));
	if (str1 != NULL)
		strcpy(buff, str1);
	if (str2 != NULL)
		strcat(buff, str2);
	if (str3 != NULL)
		strcat(buff, str3);
	write(fd, buff, strlen(buff));
	display_message(1, buff);

	memset(answer, 0, sizeof(answer));
	sleep(1);
	read(fd, answer, sizeof(answer));
	display_message(0, answer);

}

int send(int fd, char *cmgf, char *cmgs, char *csca, char *message)
{
	/* AT Test */
	Send_AT(fd, "AT\r", NULL, NULL);
	/* Set Modem Full Function */
	Send_AT(fd, "AT +CFUN=", "1", "\r");
	/* Set CMGF */
	Send_AT(fd, "AT +CMGF=", cmgf, "\r");
	/* Set Message Centr Number */
	Send_AT(fd, "AT +CSCA=", csca, "\r");
	/* Set Receive Number */
	Send_AT(fd, "AT +CMGS=", cmgs, "\r");
	/* Send Message */
	Send_AT(fd, message, NULL, NULL);
}

int Send_Message(int fd)
{
	char buff[128];
	char num1[64];
	char num2[64];
	int i;
	int choice;

	printf("********* City Select **********\n");
	for (i = 0; i < NR_CITY; i++) 
		printf("[%2d] %s\n", i, City_Number[i].city);
	printf("Please select your City!\n");
	scanf("%d", &choice);
	do {
		memset(num1, 0, sizeof(num1));
		printf("\nPlease Entry Receive phone number:\n");
		scanf("%s", num1);
	} while (strlen(num1) != 11);

	sleep(1);
	memset(buff, 0, sizeof(buff));
	printf("Please input Meesage:\n");
	scanf("%s", buff);

	/* Restruct buff */
	i = strlen(buff);
	buff[i] = 0x1A;
	buff[i+1] = '\r';
	buff[i+2] = '\0';

	memset(num2, 0, sizeof(num2));
	strcpy(num2, "+86");
	strcat(num2, num1);

	memset(num1, 0, sizeof(num1));
	strcpy(num1, "+86");
	strcat(num1, City_Number[choice].number);
	
	send(fd, "1", num2, num1, buff);
}

/*
 * Call Phone.
 */
void Call_Phone(int fd)
{
	char buff[128];
	char number[20];

	do {
		memset(number, 0, sizeof(number));
		printf("\nPlease input phone number:");
		scanf("%s", number);
	} while (strlen(number) != 11);

	memset(buff, 0, sizeof(buff));
	strcpy(buff, "+86");
	strcat(buff, number);
	strcat(buff, ";");

	/* AT Test */
	Send_AT(fd, "AT\r", NULL, NULL);
	/* Call */
	Send_AT(fd, "AT", " DT ", buff);
}

int main(int argc, char *argv[])
{
	int fd;
	char choice;

	fd = open(MODEM_PATH, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		printf("Can't open %s\n", MODEM_PATH);
		return -1;
	}

	/* Initialize /dev/modem0 */
	serial_init(fd);
	
	printf("************************************************\n");
	printf("\tWelcome to OrangePi 2G-IOT\n");
	printf("\tModem version %s\n", VERSION);
	printf("************************************************\n");
	printf("Entry your select:\n");
	printf("1. Send Message\n");
	printf("2. Call Phone\n");
	printf("3. Exit\n");
	choice = getchar();

	switch (choice) {
	case '1': 
			Send_Message(fd);
			break;
	case '2':
			Call_Phone(fd);
			break;
	default:
			break;
	
	}
	close(fd);

	return 0;
}
