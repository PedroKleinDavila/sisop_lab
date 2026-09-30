#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#define MAX_LOG_MESSAGE 255

SYSCALL_DEFINE1(logUserMessage, const char __user *, message)
{
	char buffer[MAX_LOG_MESSAGE + 1];
	long length;

	if (!message)
		return -EFAULT;
	length = strncpy_from_user(buffer, message, sizeof(buffer));
	if (length < 0)
		return length;
	if (length == 0)
		return -EINVAL;
	if (length >= sizeof(buffer))
		return -E2BIG;
	printk(KERN_INFO "SISOP user message: %s\n", buffer);
	return length;
}
