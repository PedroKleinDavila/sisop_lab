#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "xtea_driver"
#define MAX_DATA 256
#define MAX_COMMAND 1024

static char *key0 = "f0e1d2c3";
static char *key1 = "b4a59687";
static char *key2 = "78695a4b";
static char *key3 = "3c2d1e0f";
module_param(key0, charp, 0444);
module_param(key1, charp, 0444);
module_param(key2, charp, 0444);
module_param(key3, charp, 0444);
MODULE_PARM_DESC(key0, "First 32-bit XTEA key word in hexadecimal");
MODULE_PARM_DESC(key1, "Second 32-bit XTEA key word in hexadecimal");
MODULE_PARM_DESC(key2, "Third 32-bit XTEA key word in hexadecimal");
MODULE_PARM_DESC(key3, "Fourth 32-bit XTEA key word in hexadecimal");

static u32 key[4];
static int major_number;
static struct class *xtea_class;
static struct device *xtea_device;

struct xtea_session {
	struct mutex lock;
	char response[MAX_DATA * 2 + 2];
	size_t response_length;
};

static void encipher(u32 v[2])
{
	u32 v0 = v[0], v1 = v[1], sum = 0;
	unsigned int i;
	for (i = 0; i < 32; ++i) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += 0x9e3779b9;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^
			(sum + key[(sum >> 11) & 3]);
	}
	v[0] = v0;
	v[1] = v1;
}

static void decipher(u32 v[2])
{
	u32 v0 = v[0], v1 = v[1], sum = 0x9e3779b9 * 32;
	unsigned int i;
	for (i = 0; i < 32; ++i) {
		v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^
			(sum + key[(sum >> 11) & 3]);
		sum -= 0x9e3779b9;
		v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	v[0] = v0;
	v[1] = v1;
}

static int parse_hex_byte(char high, char low)
{
	int a = hex_to_bin(high);
	int b = hex_to_bin(low);
	if (a < 0 || b < 0)
		return -EINVAL;
	return (a << 4) | b;
}

static int xtea_open(struct inode *inode, struct file *file)
{
	struct xtea_session *session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session)
		return -ENOMEM;
	mutex_init(&session->lock);
	file->private_data = session;
	return 0;
}

static int xtea_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static ssize_t xtea_read(struct file *file, char __user *buffer, size_t len,
			 loff_t *offset)
{
	struct xtea_session *session = file->private_data;
	ssize_t result;
	mutex_lock(&session->lock);
	result = simple_read_from_buffer(buffer, len, offset,
					 session->response, session->response_length);
	mutex_unlock(&session->lock);
	return result;
}

static ssize_t xtea_write(struct file *file, const char __user *buffer,
			  size_t len, loff_t *offset)
{
	struct xtea_session *session = file->private_data;
	char *command;
	char operation[4], hex[MAX_DATA * 2 + 1], trailing;
	unsigned int data_size, i;
	u8 data[MAX_DATA];
	int fields, value;
	static const char digits[] = "0123456789abcdef";

	if (!len || len > MAX_COMMAND)
		return -EINVAL;
	command = kmalloc(len + 1, GFP_KERNEL);
	if (!command)
		return -ENOMEM;
	if (copy_from_user(command, buffer, len)) {
		kfree(command);
		return -EFAULT;
	}
	command[len] = '\0';
	fields = sscanf(command, "%3s %u %512s %c",
			operation, &data_size, hex, &trailing);
	kfree(command);
	if (fields != 3 || (strcmp(operation, "enc") &&
			    strcmp(operation, "dec")) ||
	    data_size == 0 || data_size > MAX_DATA || data_size % 8 ||
	    strlen(hex) != data_size * 2)
		return -EINVAL;
	for (i = 0; i < data_size; ++i) {
		value = parse_hex_byte(hex[i * 2], hex[i * 2 + 1]);
		if (value < 0)
			return value;
		data[i] = value;
	}
	for (i = 0; i < data_size; i += 8) {
		u32 words[2] = {
			((u32)data[i] << 24) | ((u32)data[i + 1] << 16) |
			((u32)data[i + 2] << 8) | data[i + 3],
			((u32)data[i + 4] << 24) | ((u32)data[i + 5] << 16) |
			((u32)data[i + 6] << 8) | data[i + 7],
		};
		if (operation[0] == 'e')
			encipher(words);
		else
			decipher(words);
		data[i] = words[0] >> 24;
		data[i + 1] = words[0] >> 16;
		data[i + 2] = words[0] >> 8;
		data[i + 3] = words[0];
		data[i + 4] = words[1] >> 24;
		data[i + 5] = words[1] >> 16;
		data[i + 6] = words[1] >> 8;
		data[i + 7] = words[1];
	}
	mutex_lock(&session->lock);
	for (i = 0; i < data_size; ++i) {
		session->response[i * 2] = digits[data[i] >> 4];
		session->response[i * 2 + 1] = digits[data[i] & 15];
	}
	session->response[data_size * 2] = '\n';
	session->response_length = data_size * 2 + 1;
	file->f_pos = 0;
	mutex_unlock(&session->lock);
	return len;
}

static const struct file_operations xtea_fops = {
	.owner = THIS_MODULE,
	.open = xtea_open,
	.release = xtea_release,
	.read = xtea_read,
	.write = xtea_write,
};

static int __init xtea_init(void)
{
	char *values[4] = { key0, key1, key2, key3 };
	unsigned int i;
	int result;

	for (i = 0; i < 4; ++i) {
		if (!values[i] || strlen(values[i]) != 8 ||
		    kstrtou32(values[i], 16, &key[i]))
			return -EINVAL;
	}
	major_number = register_chrdev(0, DEVICE_NAME, &xtea_fops);
	if (major_number < 0)
		return major_number;
	xtea_class = class_create(THIS_MODULE, "xtea_class");
	if (IS_ERR(xtea_class)) {
		result = PTR_ERR(xtea_class);
		goto unregister;
	}
	xtea_device = device_create(xtea_class, NULL,
				    MKDEV(major_number, 0), NULL, DEVICE_NAME);
	if (IS_ERR(xtea_device)) {
		result = PTR_ERR(xtea_device);
		goto destroy_class;
	}
	pr_info("xtea_driver loaded\n");
	return 0;
destroy_class:
	class_destroy(xtea_class);
unregister:
	unregister_chrdev(major_number, DEVICE_NAME);
	return result;
}

static void __exit xtea_exit(void)
{
	device_destroy(xtea_class, MKDEV(major_number, 0));
	class_destroy(xtea_class);
	unregister_chrdev(major_number, DEVICE_NAME);
	pr_info("xtea_driver unloaded\n");
}

module_init(xtea_init);
module_exit(xtea_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("XTEA character driver with load-time user key");
