/*
 * SO2 Lab - Interrupts (#5)
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_DESCRIPTION("SO2 KBD");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL		KERN_ALERT
#define MODULE_NAME		"so2_kbd"

#define SO2_KBD_MAJOR		42
#define SO2_KBD_MINOR		0
#define SO2_KBD_NR_MINORS	1

#define I8042_KBD_IRQ		1
#define I8042_STATUS_REG	0x64
#define I8042_DATA_REG		0x60

#define MY_BASEPORT		0x3F8
#define MY_NR_PORTS		8

#define BUFFER_SIZE		1024
#define SCANCODE_RELEASED_MASK	0x80

#define MAGIC_WORD		"root"
#define MAGIC_WORD_LEN		4

struct so2_device_data {
	struct cdev cdev;
	struct mutex mutex;
	/* TODO 4: Add spinlock. */
	spinlock_t lock;
	char buf[BUFFER_SIZE];
	size_t buf_idx;
	char tmp_buf[BUFFER_SIZE];
	size_t tmp_buf_idx;
	/* the number of characters that were matched so far */
	size_t passcnt;
} devs[1];

/*
 * Checks if scancode corresponds to key press or release.
 */
static int is_key_press(int scancode)
{
	return !(scancode & SCANCODE_RELEASED_MASK);
}

/*
 * Return the character of the given scancode.
 * Only works for alphanumeric/space/enter; returns '?' for other
 * characters.
 */
static char get_ascii(int scancode)
{
	static char *row1 = "1234567890";
	static char *row2 = "qwertyuiop";
	static char *row3 = "asdfghjkl";
	static char *row4 = "zxcvbnm";

	scancode &= ~SCANCODE_RELEASED_MASK;
	if (scancode >= 0x02 && scancode <= 0x0b)
		return *(row1 + scancode - 0x02);
	if (scancode >= 0x10 && scancode <= 0x19)
		return *(row2 + scancode - 0x10);
	if (scancode >= 0x1e && scancode <= 0x26)
		return *(row3 + scancode - 0x1e);
	if (scancode >= 0x2c && scancode <= 0x32)
		return *(row4 + scancode - 0x2c);
	if (scancode == 0x39)
		return ' ';
	if (scancode == 0x1c)
		return '\n';
	return '?';
}

/*
 * Handle IRQ 1 (keyboard IRQ).
 */
static u8 i8042_read_data(void)
{
	u8 val;
	/* TODO 3: read DATA register (8 bits) */
	val = inb(I8042_DATA_REG);
	return val;
}

/* TODO 2: Implement basic interrupt handler. */
/* TODO 3: Implement ASCII key collecting in interrupt handler. */
/* TODO 5: Match password and clean buffer. */
irqreturn_t so2_kbd_interrupt_handle(int irq_no, void *dev_id);
irqreturn_t so2_kbd_interrupt_handle(int irq_no, void *dev_id)
{
	u8 scancode;
	char ch;
	int pressed;
	struct so2_device_data *data = (struct so2_device_data *) dev_id;

	scancode = i8042_read_data();
	ch = get_ascii(scancode);
	pressed = is_key_press(scancode);

	if (pressed && data->buf_idx < BUFFER_SIZE-1)
	{
		data->buf[data->buf_idx] = ch;
		data->buf_idx++;
		if (MAGIC_WORD[data->passcnt] == ch)
			data->passcnt++;
	}


	if (data->passcnt == MAGIC_WORD_LEN)
	{
		spin_lock(&data->lock);
		data->buf_idx = 0;
		memset(data->buf, 0, BUFFER_SIZE);
		spin_unlock(&data->lock);
		data->passcnt = 0;
	}
	//printk(LOG_LEVEL "IRQ %d: scancode=0x%x (%u) pressed=%d ch=%c\n", irq_no, scancode, scancode, pressed, ch);
	
	return IRQ_NONE;
}


static int so2_kbd_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data =
		container_of(inode->i_cdev, struct so2_device_data, cdev);

	file->private_data = data;
	printk(LOG_LEVEL "%s opened\n", MODULE_NAME);
	return 0;
}

static int so2_kbd_release(struct inode *inode, struct file *file)
{
	printk(LOG_LEVEL "%s closed\n", MODULE_NAME);
	return 0;
}

static ssize_t so2_kbd_read(struct file *file, char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data =
		(struct so2_device_data *) file->private_data;
	size_t to_read;
	unsigned long flags;

	/* Readers are exclusive. */
	mutex_lock(&data->mutex);

	/* Check if range is valid. */
	if (*offset >= data->buf_idx) {
		mutex_unlock(&data->mutex);
		return 0;
	}

	/*
	 * TODO 4: Copy information to temporary buffer. Use spinlock
	 * for exclusive access.
	 */
	spin_lock_irqsave(&data->lock, flags);
	strncpy(data->tmp_buf, data->buf, data->buf_idx);
	data->tmp_buf_idx = data->buf_idx;
	spin_unlock_irqrestore(&data->lock, flags);
	/*
	 * TODO 4: Fill to_read variabble with actual bytes to read. Minimum
	 * of buf_idx and size.
	 */
	if (*offset > data->tmp_buf_idx)
	{
		mutex_unlock(&data->mutex);
		return 0;
	}

	if (data->tmp_buf_idx < *offset + size)
	{
		to_read = data->tmp_buf_idx;
	}
	else
	{
		to_read = size;
	}
	data->tmp_buf[to_read] = '\0';
	/* TODO 4: Copy information in temporary buffer to user space. */
	if (copy_to_user(user_buffer, data->tmp_buf, to_read+1))
		return -EFAULT;
		
	/* Update offset. */
	*offset += to_read;
	mutex_unlock(&data->mutex);

	return to_read;
}

static const struct file_operations so2_kbd_fops = {
	.owner = THIS_MODULE,
	.open = so2_kbd_open,
	.release = so2_kbd_release,
	.read = so2_kbd_read,
};

static int so2_kbd_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR),
			SO2_KBD_NR_MINORS, MODULE_NAME);
	if (err != 0) {
		printk(LOG_LEVEL "ERROR: %s: error %d\n",
				"register_region", err);
		goto out;
	}

	/* TODO 1: Uncomment code and return -EBUSY when the request fails. */
	/* The following piece of code will fail, so leave it commented when you are finished. */
	/*
	if (request_region(I8042_DATA_REG, 1, MODULE_NAME) == NULL) {
		// This fails because the keyboard driver already registered the region.
		err = -EBUSY;
		goto out_unregister;
	}

	if (request_region(I8042_STATUS_REG, 1, MODULE_NAME) == NULL) {
		// Same here
		err = -EBUSY;
		goto out_unregister;
	}
	*/

	/* Initialize mutex for exclusive read operation. */
	mutex_init(&devs[0].mutex);

	/* TODO 4: Initialize spinlock before requesting IRQ. */
	spin_lock_init(&devs[0].lock);
	/* TODO 2: Request IRQ. */
	err = request_irq(I8042_KBD_IRQ, so2_kbd_interrupt_handle, IRQF_SHARED, MODULE_NAME, &devs[0]);
	if (err < 0) {
		goto out_unregister;
	}
	/* Initialize buffers. */
	memset(devs[0].buf, 0, BUFFER_SIZE);
	devs[0].buf_idx = 0;
	memset(devs[0].tmp_buf, 0, BUFFER_SIZE);
	devs[0].tmp_buf_idx = 0;
	devs[0].passcnt = 0;
	/* Initialize character device. */
	cdev_init(&devs[0].cdev, &so2_kbd_fops);
	cdev_add(&devs[0].cdev, MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR), 1);

	printk(LOG_LEVEL "Driver %s loaded\n", MODULE_NAME);
	return 0;

out_unregister:
	unregister_chrdev_region(MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR),
			SO2_KBD_NR_MINORS);
out:
	return err;
}

static void so2_kbd_exit(void)
{
	cdev_del(&devs[0].cdev);

	/* TODO 2: Free IRQ */
	free_irq(I8042_KBD_IRQ, &devs[0]);
	/* We didn't call request_region, so leave this commented out. */
	/*
	release_region(I8042_STATUS_REG, 1);
	release_region(I8042_DATA_REG, 1);
	*/

	unregister_chrdev_region(MKDEV(SO2_KBD_MAJOR, SO2_KBD_MINOR),
			SO2_KBD_NR_MINORS);
	printk(LOG_LEVEL "Driver %s unloaded\n", MODULE_NAME);
}

module_init(so2_kbd_init);
module_exit(so2_kbd_exit);
