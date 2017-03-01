/*
 * SO lab-02 - task 6 - cmd_mod.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>

MODULE_DESCRIPTION("Command-line args module");
MODULE_AUTHOR("So2rul Esforever");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_WARNING

static int __init cmd_init(void)
{
	printk(LOG_LEVEL "executable=%s\n, PID=%d\n", current->comm, current->pid);
	return 0;
}

static void __exit cmd_exit(void)
{
	printk(LOG_LEVEL "executable=%s\n, PID=%d\n", current->comm, current->pid);
}

module_init(cmd_init);
module_exit(cmd_exit);
