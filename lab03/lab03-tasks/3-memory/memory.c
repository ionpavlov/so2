/*
 * SO2 lab3 - task 3
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_ALERT

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;
	ti = kmalloc(sizeof(struct task_info), GFP_KERNEL);	
	/* TODO 2: Allocate and initialize ti. */
	if (!ti)
		return NULL;
	ti->pid = pid;
	ti->timestamp = jiffies;

	return ti;
}

static int memory_init(void)
{
	ti1 = task_info_alloc(current->pid);
	ti2 = task_info_alloc(current->parent->pid);
	ti3 = task_info_alloc(next_task(current)->pid);
	ti4 = task_info_alloc(next_task(next_task(current))->pid);

	return 0;
}

static void memory_exit(void)
{
	/* TODO 3: Print ti* field values. */
	printk(KERN_ALERT "t1.pid = %d t1.timestamp = %lu\n", ti1->pid, ti1->timestamp);
	printk(KERN_ALERT "t2.pid = %d t2.timestamp = %lu\n", ti2->pid, ti2->timestamp);
	printk(KERN_ALERT "t3.pid = %d t3.timestamp = %lu\n", ti3->pid, ti3->timestamp);
	printk(KERN_ALERT "t4.pid = %d t4.timestamp = %lu\n", ti4->pid, ti4->timestamp);
	/* TODO 4: Free ti*. */
	kfree(ti1);
	kfree(ti2);
	kfree(ti3);
	kfree(ti4);
}

module_init(memory_init);
module_exit(memory_exit);
