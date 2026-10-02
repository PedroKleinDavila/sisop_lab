#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#define MAX_SLEEPING_PIDS 4096

SYSCALL_DEFINE2(listSleepingProcesses, pid_t __user *, out, size_t, capacity)
{
	struct task_struct *task;
	pid_t *snapshot;
	size_t count = 0;
	long result;

	if (!out || !capacity || capacity > MAX_SLEEPING_PIDS)
		return -EINVAL;
	snapshot = kmalloc_array(capacity, sizeof(*snapshot), GFP_KERNEL);
	if (!snapshot)
		return -ENOMEM;

	rcu_read_lock();
	for_each_process(task) {
		long state = READ_ONCE(task->state);

		if (!(state & (TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)))
			continue;
		if (count == capacity) {
			result = -ENOSPC;
			goto unlock;
		}
		snapshot[count++] = task_pid_nr(task);
	}
	result = count;
unlock:
	rcu_read_unlock();
	if (result >= 0 && copy_to_user(out, snapshot, count * sizeof(*snapshot)))
		result = -EFAULT;
	kfree(snapshot);
	return result;
}
