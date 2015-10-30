#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "ioctl_basic.h" 
	
struct i2c_dev {
	struct cdev cdev;               						// The cdev structure
	struct i2c_adapter *adap;
	struct device *dev;
}*i2c_dev;

#define DEVICE "i2c_flash"
unsigned char addr[2] = {0x00,0x00};
static int page_pointer;
int busy_flag;

void add_calc (int page)							// function to calculate address given page number
{
	int cnt = 0;
	int tmp_addr;
	int tmp1;
	addr[0] = 0x00;
	addr[1] = 0x00;
	while (cnt != page)
	{
		tmp1 = addr[1];
		tmp_addr = addr[0];
		tmp_addr = (tmp_addr << 8) | ( tmp1);
		tmp_addr = tmp_addr + 64;
		addr[1] = (tmp_addr & 0xFF);
		addr[0] = (tmp_addr & 0xFF00) >> 8;
		cnt = cnt + 1;

	}
}
static dev_t i2c_dev_number;     							// Allotted device number
struct class *i2c_dev_class;         							// Tie with the device model


long ioctl_funcs(struct file *file,unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	switch(cmd) 
	{
		case IOCTL_FLASHGETS:
		{
			
			if ( busy_flag == 1)
			{
											//EEPROM is Busy
				return -EBUSY;
			}	
			else	
			{								//EEPROM is Available	
				return 0;
			}
		}
		break;
		case IOCTL_FLASHERASE:							// to erase EEPROM.
		{
			int retr = 0;
			int data_byte_count = 66;
			unsigned char* tmp;
			int j, i = 0;
			int page_count;
			int tmp_addr;
			int tmp1;
			unsigned char addr[2] = {0x00,0x00};
			struct i2c_client *client = file->private_data;
			client->addr= 0x54;
			tmp = (char*)kmalloc(sizeof(char)*data_byte_count, GFP_KERNEL);
			if (tmp == NULL)
				return -ENOMEM;
			page_count = 0;
			if (busy_flag != 1)
			{ 
				gpio_set_value_cansleep(26, 1);
					while (page_count != 512)
				{	
					tmp[0] = addr[0];
	        			tmp[1] = addr[1];
	    		
					j = 2;
					for(i = page_count*64; i < (page_count*64)+ 64;i++)
					{
						tmp[j] = 0xFF;							// all pages
						j++;
					}
					tmp [66] = '\0';
					
    				   		// writing to the device
    			    		retr = i2c_master_send(client, tmp, data_byte_count);
					msleep (5);
					page_count = page_count +1;
			
					tmp1 = addr[1];
					tmp_addr = addr[0];
					tmp_addr = (tmp_addr << 8) | ( tmp1);
					tmp_addr = tmp_addr + 64;
					addr[1] = (tmp_addr & 0xFF);
					addr[0] = (tmp_addr & 0xFF00) >> 8;		
			
				}				
				gpio_set_value_cansleep(26, 0);
			}
			else 
				return -EBUSY;
			kfree(tmp);
			
		}	
		break;


		case IOCTL_FLASHSETP:						// to set pointer as per user requirement
		{			
			
			struct i2c_client *client = file->private_data;	
			page_pointer = arg;			
			add_calc (page_pointer);
			client->addr= 0x54;		
			ret = i2c_master_send(client, addr, 2);
			msleep (5);	
			
		}
		break;
		case IOCTL_FLASHGETP:						// to get current pointer position
		{
			ret = copy_to_user((int *)arg, &page_pointer, sizeof(int));
			
		}
		break;
 	}
	
return ret;

}



static ssize_t i2cdev_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset)
{
	char *tmp;
	char *data;
	int ret = 0;
	int ret1 = 0;
	int byte_count = count*64;
	int i =0, j = 0, k = 0;
	struct i2c_client *client = file->private_data;	
	data = (char*)kmalloc(sizeof (char)*byte_count, GFP_KERNEL);
	tmp = (char*)kmalloc(sizeof (char)*64, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	busy_flag = 1;									// Reading from EEPROM hence busy
	gpio_set_value_cansleep(26, 1);
	
	// to read from EEPROM pagewise	
	
	while (k != count)
	{	
		
		add_calc (page_pointer);
		ret = i2c_master_recv(client, tmp,64);					// data received from i2c
		msleep(5);
		for ( j =0; j<64; j ++)
		{
			data[i] = tmp[j];
			i++;		
		}
						
		k ++;
		
		if ( page_pointer == 511)
			page_pointer = 0;
		else
			page_pointer ++;

	}
	ret1 = copy_to_user(buf, data, 64*count);
	busy_flag = 0;									// work done. EEPROM available
	gpio_set_value_cansleep(26, 0);
	kfree(tmp);
	return ret;
}

static ssize_t i2cdev_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset)
{
	int ret = 0;
	unsigned char* tmp;
	char* data;
	int i, j;
	int k =0;
	struct i2c_client *client = file->private_data;
	client->addr= 0x54;
	tmp = (char*)kmalloc(sizeof(char)*66, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	data = (char*)kmalloc(sizeof(char)*64*count, GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;
	ret = copy_from_user((void*)data ,(void __user *)buf  ,count*64);		//data received from user
	if( ret > 0)
		return -1;
	data[64*count] = '\0';	
	busy_flag = 1;									// Writing on EEPROM hence busy
	gpio_set_value_cansleep(26, 1);							//to glow led when EEPROM is busy

	// to write on EEPROM pagewise

	while (k != count)			
	{	
		add_calc (page_pointer);
		tmp[0] = addr[0];
        	tmp[1] = addr[1];
		j = 2;
		for(i = k*64; i < (k*64)+ 64;i++)
		{
			tmp[j] = data[i];
			j++;
		}
		tmp[66] = '\0';
		ret = i2c_master_send(client, tmp, 66);
		msleep (5);
		k++;				
		if (page_pointer == 511)
			page_pointer = 0;
		else
			page_pointer ++;
		
	}
	busy_flag = 0;									// work done. EEPROM available
	gpio_set_value_cansleep(26, 0);
	kfree(tmp);
	kfree(data);
	return ret;

}


static int i2cdev_open(struct inode *inode, struct file *file)
{
         struct i2c_client *client;
         struct i2c_adapter *adap;
         struct i2c_dev *i2c_dev;
 
         i2c_dev = container_of(inode->i_cdev, struct i2c_dev, cdev);
         if (!i2c_dev)
                 return -ENODEV;
 
         adap = i2c_get_adapter(0);						// adapter is attached
         if (!adap)
                 return -ENODEV;
 
         client = kzalloc(sizeof(*client), GFP_KERNEL);				//creates i2c client
         if (!client) 
	{
                 i2c_put_adapter(adap);
                 return -ENOMEM;
         }
         snprintf(client->name, I2C_NAME_SIZE, "i2c-dev %d", 0);
         client->adapter = adap;
         file->private_data = client;
 	
         return 0;
}


static int i2cdev_release(struct inode *inode, struct file *file)
{
	struct i2c_client *client = file->private_data; 
	i2c_put_adapter(client->adapter);
	printk(KERN_ALERT" released\n");
        kfree(client);
	file->private_data = NULL; 
    	return 0;
}


static const struct file_operations fops = {
	.owner		= THIS_MODULE,
	.read		= i2cdev_read,
	.write		= i2cdev_write,
	 .open 		= i2cdev_open,
	.release	 = i2cdev_release,
	.unlocked_ioctl	 = ioctl_funcs,
};

/* ------------------------------------------------------------------------- */



static int __init i2c_dev_init(void)
{
	int ret, val;
	struct i2c_adapter *adap;
	
//	struct i2c_dev *i2c_dev;
	adap = kmalloc (sizeof (struct i2c_adapter), GFP_KERNEL);
	if (adap == NULL)
		return -ENOMEM;
	printk(KERN_ALERT"i2c dev entries driver\n");
	
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&i2c_dev_number, 0, 1, DEVICE) < 0) 
	{
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}
	
	i2c_dev_class = class_create(THIS_MODULE, DEVICE);
	printk(KERN_ALERT"Class is created\n");
	
	/* Bind to already existing adapters right away */
		
	adap = to_i2c_adapter(0);

	i2c_dev = kzalloc (sizeof (*i2c_dev), GFP_KERNEL);

	// register this i2c device with the driver core 
	i2c_dev->adap = adap;

	device_create(i2c_dev_class, NULL ,i2c_dev_number, NULL, DEVICE);
	printk(KERN_ALERT"attached adapter\n");
		


	/* Connect the file operations with the cdev */
	cdev_init(&i2c_dev->cdev, &fops);
	//cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&i2c_dev->cdev, (i2c_dev_number), 1);

	if (ret) 
	{
		printk("Bad cdev\n");
		return ret;
	}
	gpio_request(26, "led");			// led is connected to IO pin IO8
	gpio_direction_output(26,0);
	gpio_request(29, "mux");			//to enable SDA and SCL/ A4, A5
	gpio_direction_output(29,0);
	gpio_set_value_cansleep(29, 0);
	val = gpio_get_value_cansleep(29);	
	
	//printk(KERN_INFO "GPIO 29 value is %d\n", val);
	gpio_set_value_cansleep(26, 0);

	return 0;

}

static void __exit i2c_dev_exit(void)
{
	device_destroy(i2c_dev_class, i2c_dev_number);
	class_destroy(i2c_dev_class);
	unregister_chrdev(i2c_dev_number, DEVICE);
	printk("driver is exiting\n");
	cdev_del(&i2c_dev->cdev);
	gpio_free(26);
	gpio_free(29);

}

MODULE_AUTHOR("Uma Kulkarni");
MODULE_DESCRIPTION("Accessing EEPROM via I2c bus");
MODULE_LICENSE("GPL");

module_init(i2c_dev_init);
module_exit(i2c_dev_exit);

// references: http://lxr.free-electrons.com/source/drivers/i2c/i2c-dev.c
//Copyright (C) 1995-97 Simon G. Vogl
//Copyright (C) 1998-99 Frodo Looijaard <frodol@dds.nl>
//Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>
//The I2C_RDWR ioctl code is written by Kolja Waschk <waschk@telos.de>
