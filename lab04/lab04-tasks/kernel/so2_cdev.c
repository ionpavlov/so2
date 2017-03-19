/*
 * SO2 Lab - Linux device drivers (#4)
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

#define MY_MAJOR		42
#define MY_MINOR		0
#define NUM_MINORS		1
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			"hello\n"
#define IOCTL_MESSAGE		"Hello ioctl"
#define LOCKED			0
#define UNLOCKED		1

#ifndef BUFSIZ
#define BUFSIZ		4096
#endif

static int my_open(struct inode *inode, struct file *file);
static int my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static int my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t * offset);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int my_release(struct inode *inode, struct file *file);

struct my_device_data {
	struct cdev cdev;
	char buffer[BUFSIZ];
	size_t size;
	wait_queue_head_t wq;
	int flag;
};

struct my_device_data devs;

const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read,
	.write = my_write,
	.release = my_release,
	.unlocked_ioctl = my_ioctl
};

static int my_open(struct inode *inode, struct file *file)
{
	struct my_device_data *my_data = container_of(inode->i_cdev, struct my_device_data, cdev);

	/* validate access to device */
	file->private_data = my_data;
	printk(LOG_LEVEL "%s open successful\n", MODULE_NAME);	
	
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(1000);

	return 0;
}

static int my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
	struct my_device_data *my_data = (struct my_device_data *) file->private_data;
	/* read data from device in my_data->buffer */
	size_t bytes_to_copy = 0;

	if (BUFSIZ >= size + *offset)
		bytes_to_copy = size;
	else
		bytes_to_copy = BUFSIZ - *offset;
	
	if (bytes_to_copy == 0)
		return 0;

	if(copy_to_user(user_buffer, my_data->buffer + *offset, bytes_to_copy))
		return -EFAULT;
	*offset += bytes_to_copy;
	
	return bytes_to_copy;
}

static int my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t * offset)
{
	struct my_device_data *my_data = (struct my_device_data *) file->private_data;

	if(copy_from_user(my_data->buffer, user_buffer, size))
		 return -EFAULT;
	
	return size;
}


static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct my_device_data *my_data = (struct my_device_data*)file->private_data;

	switch(cmd) {
		case MY_IOCTL_PRINT:
			printk(LOG_LEVEL IOCTL_MESSAGE);
			break;
		case MY_IOCTL_SET_BUFFER:
			if(copy_from_user(my_data->buffer, (char*)arg, strlen((char*)arg)))
				return -EFAULT;
			break;
		case MY_IOCTL_GET_BUFFER:
			if(copy_to_user((char*)arg, my_data->buffer, BUFSIZ))
				return -EFAULT;
		case MY_IOCTL_DOWN:
			wait_event_interruptible(devs.wq, devs.flag !=0);
			devs.flag = 0;
			break;
		case MY_IOCTL_UP:
			devs.flag = 1;
			wake_up_interruptible(&devs.wq);
			break;
	}
	return 0;	
}

static int my_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "%s release successful\n", MODULE_NAME);
	return 0;
}



static int so2_cdev_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS, MODULE_NAME);
	if(err != 0)
		return err;

	printk(LOG_LEVEL "%s successful registered\n", MODULE_NAME);
	cdev_init(&devs.cdev, &my_fops);
	cdev_add(&devs.cdev, MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
	
	strcpy(devs.buffer, MESSAGE);

	init_waitqueue_head(&devs.wq);
	devs.flag = 0;

	return 0;
}

static void so2_cdev_exit(void)
{
	cdev_del(&devs.cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
	printk(LOG_LEVEL "%s successful unregistered\n", MODULE_NAME);
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
