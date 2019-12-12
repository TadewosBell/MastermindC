#define pr_fmt(fmt) "CS421 Final: " fmt

#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>

static const char *lyrics[] = {
	"Oh, say can you see, by the dawn's early light,",
	"What so proudly we hailed at the twilight's last gleaming?",
	"Whose broad stripes and bright stars, through the perilous fight,",
	"O'er the ramparts we watched, were so gallantly streaming?",
	"And the rockets' red glare, the bombs bursting in air,",
	"Gave proof through the night that our flag was still there.",
	"O say, does that star-spangled banner yet wave",
	"O'er the land of the free and the home of the brave?"
};

struct thread_data {
	int whoami;
	struct completion comp;
};

static struct mutex lock;
static bool running;
static int next;

static int thread_func(void *arg)
{
	struct thread_data *my_data = (struct thread_data *)(arg);

	mutex_lock(&lock);
	while (next != my_data->whoami) {
		mutex_unlock(&lock);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(2 * HZ);
		mutex_lock(&lock);
	}
	pr_info("%s\n", lyrics[my_data->whoami]);
	mutex_unlock(&lock);
	complete(&my_data->comp);
	return 0;
}

static int delayed_work(void *unused)
{
	struct thread_data data[8];
	int i;

	pr_info("Defence of Fort M'Henry\n");
	mutex_lock(&lock);
	next = -1;
	for (i = 0; i < 8; i++) {
		data[i].whoami = i;
		init_completion(&data[i].comp);
		kthread_run(thread_func, &data[i], "final%d", i);
	}
	for (i = 0; i < 8; i++) {
		next = i;
		mutex_unlock(&lock);
		wait_for_completion_interruptible(&data[i].comp);
		mutex_lock(&lock);
	}
	running = false;
	mutex_unlock(&lock);
	return 0;
}

static ssize_t final_write(struct file *filp, const char __user * ubuf,
			   size_t count, loff_t * ppos)
{
	char buf[6];
	int retval;

	if (count != 5)
		return -EINVAL;
	retval = copy_from_user(buf, ubuf, 5);
	if (retval)
		return -EINVAL;
	buf[5] = '\0';
	if (strcmp(buf, "cs421") == 0) {
		mutex_lock(&lock);
		if (running) {
			mutex_unlock(&lock);
			return -EBUSY;
		}
		running = true;
		kthread_run(delayed_work, NULL, "final");
		mutex_unlock(&lock);
		return 5;
	}
	return -EINVAL;
}

static const struct file_operations final_fops = {
	.write = final_write,
};

static struct miscdevice final_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "final",
	.fops = &final_fops,
	.mode = 0666,
};

static int __init final_init(void)
{
	mutex_init(&lock);
	return misc_register(&final_dev);
}

static void __exit final_exit(void)
{
	misc_deregister(&final_dev);
}

module_init(final_init);
module_exit(final_exit);
MODULE_DESCRIPTION("CS421 Final");
MODULE_LICENSE("GPL");