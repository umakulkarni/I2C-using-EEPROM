	i2c-flash.c , main_2.c
============================================

This directory contians 2 files i2c-flash.c and main_2.c.

i2c-flash.c 	---	This the device  driver for our EEPROM which works on pages.
			It implements open,read,write,ioctl,close function for the device file "/dev/i2c_flash".
			read and write operations are blocking operations in TASK1 and non-blocking in TASK2.
			In TASK2 it creates a workqueue and submits the work to the workqueue and returns immediatly.

main_2.c	---	This the user space application which test our driver by sending a number of pages data to the device and the 				reading same from the device. User can modify page pointer and number of pages he wants to write or read.
			This application contineously polls the read function untill it returns 0;

Note: STRICTLY USE TAB WIDTH OF 4 TO VIEW BOTH ' .C' FILES

	Compilation
================================

To compile the driver and user-space application use:

Change your linux terminal's present directory path to where tasks are saved.

Give the source of kernel  “source ~/SDK/environment-setup-i586-poky-linux”.
Compile the driver module using “make” command.
Make the executable of user program main_2 by using command “$CC -o main_2 main_2.c ”
Tranfer the module and user program to board kernel using command “scp -r <file_name> root@<inet_address>:/home/root”

Insert the module into kernel using “insmod i2c_flash.ko” command on PUTTY terminal.

Run the executable by using command “ ./main_2” on PUTTY terminal .
The program will run showing a choice menu of different operations to be performed on EEPROM.

Enter the choice no. accordingly and follow the instructions given by the program at run time.
Once done with using the module, remove it from  kernel using “rmmod i2c_flash.ko” command on PUTTY terminal.


To delete these files  use:

bash:~ make clean


	Usage
=================================

To insert the module use:
-----------------------------

if you are root user:
bash:~ insmod i2c_flash.ko

if you are not root user:
bash:~ sudo insmod i2c_flash.ko
[ It will prompt you to enter your password.]



To remove the module use:
------------------------------

if you are root user:
bash:~ rmmod i2c_flash.ko

if you are not root user:
bash:~ sudo rmmod i2c_flash.ko
[ It will prompt you to enter your password.]


To test your driver use:
---------------------------------

if you are root user:
bash:~ ./main_2

if you are not root user:
bash:~ sudo ./main_2
[ It will prompt you to enter your password.]



