#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE3(listProcessInfo, long, pid, char __user *, out, int, size)
{
	struct task_struct *task;
	char info[256];
	int length;

	if (pid <= 0 || !out || size <= 0)
		return -EINVAL;
	rcu_read_lock();
	for_each_process(task) {
		if (task_pid_nr(task) != pid)
			continue;
		length = scnprintf(info, sizeof(info),
			"Process: %s\nPID_Number: %ld\nProcess State: %ld\n"
			"Priority: %d\nRT_Priority: %u\n",
			task->comm, pid, task->state, task->prio,
			task->rt_priority);
		rcu_read_unlock();
		if (length + 1 > size)
			return -ENOSPC;
		if (copy_to_user(out, info, length + 1))
			return -EFAULT;
		return length;
	}
	rcu_read_unlock();
	return -ESRCH;
}
