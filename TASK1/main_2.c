#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "ioctl_basic.h"


#define DEVICE "/dev/i2c_flash"
#define PAGES 2


void random_string_gen(char* string, int length)
{
  char charset[] = "0123456789"
                   "abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  while (length-- > 0) {
    int index = rand() % 62;
    *string++ = charset[index];
  }
  *string = '\0';
}


int main()
{
	int i, fd;					 // fd is file descripter
	int pages;
	char* recv_data;
	char* sent_data;
	unsigned long position, pos;
	int option;
	int usr_res, ren;
	char* random_string = NULL;

	fd= open(DEVICE,O_RDWR);
	if(fd==-1)
	{
     	printf("file %s either does not exit or is currently used by an another user\n", DEVICE);
     	exit(-1);
	}
	printf ("\nUser has given Open command\n");


/******************************************************************************************************/

	while (1) {
		option = 0;
		printf("Enter option you want to choose:\n Press 1 to set page ( 0 to 511)\n Press 2 to write\n Press 3 to read\n Press 4 to get the current page position\n Press 5 to erase\n Press 6 to get status of EEPROM\n Press 7 to exit\n");
		scanf("%d",&option);
		switch (option)
		{
			case 1:
				printf("Enter the current page position you wish to set(0 to 511)\t");
				scanf("%ld",&position);
				ioctl(fd,IOCTL_FLASHSETP, position);  //ioctl call
			break;
			case 2:
				printf ("\n How many pages you want to write?(1 to 512)\t");
				scanf("%d",&pages);
				sent_data = (char*)malloc(sizeof(char)*64*pages);
				random_string = (char *)malloc(sizeof(char) * pages * 64);
				random_string_gen(random_string, pages * 64);
				strcpy(sent_data, random_string);
				printf("\nData generated in user space is:%s\n",sent_data);				
				write(fd,sent_data, pages);				
				free (sent_data);
		
			break;
			case 3:
				printf ("\n How many pages you want to read? (1 to 512)\t");
				scanf("%d",&pages);
				recv_data = (char*)malloc(sizeof(char)*64*pages);
				read(fd, recv_data, pages);
				recv_data[64*pages] = '\0';
				printf("\nMessage from process is:%s\n", recv_data);
				free (recv_data);
			break;

			case 4:
				
				ioctl(fd,IOCTL_FLASHGETP, &pos);
				printf("Current pointer is at %ld\n", pos);
			break;

			case 5:
	
				ren = ioctl(fd,IOCTL_FLASHERASE, 1);  //ioctl call
				if ( ren != 0)
					printf ("Wait.! EEPROM is Busy\n");
				else
					printf("The EEPROM is erased\n");
			break;
			case 6:
				usr_res = ioctl(fd,IOCTL_FLASHGETS, 1);
				if ( usr_res != 0)
					printf ("Status of EEPROM is - Busy\n");
				else
					printf ("Status of EEPROM is - Available\n");
			break;
			case 7:
				close(fd);
				printf("User has given close command\n");
				return 0;
			default:
				printf("Invalid Option. Try Again!\n");
				return;
				break;
		}
	}
	
}






