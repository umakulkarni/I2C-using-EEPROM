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

static int busy_flag;
static int ready;
unsigned char addr[2] = {0x00,0x00};
static int page_pointer;
static dev_t i2c_dev_number;      							// Allotted device number
struct class *i2c_dev_class;         							// Tie with the device model
struct i2c_client *client;

static struct workqueue_struct *my_wq;							// workqueue
typedef struct 
{											// work structre
	struct work_struct my_work;
	struct i2c_client *client;
	char	*buffer;
	int page_count;
}my_work_t;

my_work_t *work_r, *work_w;							// function to calculate address give page number

void add_calc (int page)
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

static void my_read_wq_function(struct work_struct *work)					// workqueue read function
{
	char *tmp;
	int ret;
	my_work_t *my_work = (my_work_t*)work_r;
	int count = my_work->page_count;
	int i =0, j = 0, k = 0;
	tmp = (char*)kmalloc(sizeof (char)*64, GFP_KERNEL);
	busy_flag = 1;										// Read operation. EEPROM is busy
	gpio_set_value_cansleep(26, 1);

	while (k != count)
	{	
		
		add_calc (page_pointer);
		ret = i2c_master_recv(my_work->client, tmp,64);
		msleep(1);
		for ( j =0; j<64; j ++)
		{
			my_work->buffer[i] = tmp[j];
			i++;		
		}
								
		k ++;
		
		if ( page_pointer == 511)
			page_pointer = 0;
		else
			page_pointer ++;
	}

	busy_flag = 0;										// EEPROM is available
	ready = 1;
	gpio_set_value_cansleep(26, 0);
	kfree(tmp);
	
}


static void my_write_wq_function(struct work_struct *work)					// workqueue function for write
{

	int ret = 0;
	int count;
	unsigned char* tmp;
	int i, j;
	int k = 0;
	int page_count;
	int data_byte_count = 66;
	my_work_t *my_work = (my_work_t*)work_w;		
	client->addr= 0x54;
	page_count = 0;
	tmp = (char*)kmalloc(sizeof(char)*data_byte_count, GFP_KERNEL);
	count  = my_work->page_count;
	busy_flag = 1;										// Write operation. EEPROM is busy
	gpio_set_value_cansleep(26, 1);


	while (k != count)
	{	
		
		add_calc (page_pointer);
		tmp[0] = addr[0];
        	tmp[1] = addr[1];
		j = 2;
		for(i = k*64; i < (k*64)+ 64;i++)
		{
			tmp[j] = my_work->buffer[i];
			j++;
		}
		tmp[66] = '\0';
		ret = i2c_master_send(my_work->client, tmp, 66);
		msleep (5);
		k++;				
		if (page_pointer == 511)
			page_pointer = 0;
		else
			page_pointer ++;
		
	}



	busy_flag = 0;									// End of operation. EEPROM is available
	gpio_set_value_cansleep(26, 0);	
	kfree(tmp);	
	kfree(work_w->buffer);
	kfree(work_w);
}


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
			{									//EEPROM is Available
				return 0;
			}
		}
		break;
		case IOCTL_FLASHERASE:
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
						tmp[j] = 0xFF;
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
		case IOCTL_FLASHSETP:							// to set pointer as per user requirement
		{			
			
			struct i2c_client *client = file->private_data;	
			page_pointer = arg;			
			add_calc (page_pointer);
			client->addr= 0x54;		
			ret = i2c_master_send(client, addr, 2);
			msleep (5);	
			
		}
		break;
		case IOCTL_FLASHGETP:							// to get current pointer
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
	int ret = 0;
	struct i2c_client *client = file->private_data;	
		
	if (ready == 1)										// read work is ready
	{	
			
		ret = copy_to_user((void __user *)buf,(void*)work_r->buffer,64*count);
		kfree(work_r->buffer);
		ready = 0;		
		return 0;
	
	}
	else if (ready != 1 && busy_flag == 0)							// work submitted to workqueue
	{
		
		work_r->buffer = (char*)kmalloc(sizeof(char)*64*count,GFP_KERNEL);
		if (work_r->buffer == NULL)
			return -ENOMEM;
		work_r->page_count = count;
		work_r->client = client;
		INIT_WORK( (struct work_struct*)work_r,my_read_wq_function);
		ret = queue_work(my_wq,(struct work_struct*)work_r);
		msleep(1);
		return -EAGAIN;
	}
	else if (ready != 1 && busy_flag == 1)
	{
											//Data is ready to read but EEPROM is busy
		return -EBUSY;
	}	
	else 
		return -EPERM;
	
}



static ssize_t i2cdev_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset)
{
	int ret = 0;
	int res = 0;
	client = file->private_data;
	if (busy_flag == 1)								// to check EEPROM status
		return -EBUSY;

	work_w = (my_work_t *)kmalloc(sizeof(my_work_t), GFP_KERNEL);
	if (work_w == NULL)
			return -ENOMEM;
	work_w->buffer = (char *)kmalloc (sizeof(char)*count*66, GFP_KERNEL);
	if (work_w->buffer == NULL)
			return -ENOMEM;
	res = copy_from_user((void*)work_w->buffer ,(void __user *)buf  ,count*64);
	msleep(5);
	work_w->page_count = count;
	work_w->client = client;
	INIT_WORK( (struct work_struct *)work_w, my_write_wq_function );
	ret = queue_work(my_wq,(struct work_struct*)work_w);				// work is enqueued
	return 0;
}




static int i2cdev_open(struct inode *inode, struct file *file)
{
         struct i2c_adapter *adap;
         struct i2c_dev *i2c_dev;
 
         i2c_dev = container_of(inode->i_cdev, struct i2c_dev, cdev);
         if (!i2c_dev)
                 return -ENODEV;
 
         adap = i2c_get_adapter(0);
         if (!adap)
                 return -ENODEV;
 
         client = kzalloc(sizeof(*client), GFP_KERNEL);					// client is created
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
	
	printk(KERN_ALERT" released\n");
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
	

	adap = kmalloc (sizeof (struct i2c_adapter), GFP_KERNEL);
	if (adap == NULL)
		return -ENOMEM;
	printk(KERN_ALERT"i2c dev entries driver\n");
	
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&i2c_dev_number, 0, 1, DEVICE) < 0) 
	{
			printk(KERN_DEBUG "Can't register device\n"); 
			return -1;
	}
	
	i2c_dev_class = class_create(THIS_MODULE, DEVICE);
	printk(KERN_ALERT"Class created\n");
	
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

	my_wq = create_workqueue("my_queue");						// create work queue
	
	work_r = (my_work_t*)kmalloc(sizeof(my_work_t),GFP_KERNEL);
	if (work_r == NULL)
		return -ENOMEM;

	
	
	gpio_request(26, "led");							// led is connected to IO pin IO8
	gpio_direction_output(26,0);
	gpio_set_value_cansleep(26, 0);
	gpio_request(29, "mux");							//to enable SDA- A4 and SCL- A5
	gpio_direction_output(29,0);
	gpio_set_value_cansleep(29, 0);
	val = gpio_get_value_cansleep(29);	
	
	//printk(KERN_INFO "GPIO 29 value is %d\n", val);
	return 0;

}

static void __exit i2c_dev_exit(void)
{
	device_destroy(i2c_dev_class, i2c_dev_number);
	class_destroy(i2c_dev_class);
	unregister_chrdev(i2c_dev_number, DEVICE);
	printk("driver exiting\n");
	cdev_del(&i2c_dev->cdev);
	i2c_put_adapter(client->adapter);
	kfree(client);

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
