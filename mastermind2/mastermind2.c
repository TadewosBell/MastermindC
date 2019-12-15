/* Name: Tadewos Bellete
   Email: bell6@umbc.edu
   Description: mastermind game but written with device drivers

   Citations:
   https://www.tutorialspoint.com/c_standard_library/c_function_strncmp.htm
   https://stackoverflow.com/questions/16955936/string-termination-char-c-0-vs-char-c-0
   http://man7.org/linux/man-pages/man3/errno.3.html
   https://www.avrfreaks.net/forum/convert-int-char-0
   https://www.csee.umbc.edu/~jtang/cs421.f19/homework/hw4/hw4_test.c
 */
/*
 * This file uses kernel-doc style comments, which is similar to
 * Javadoc and Doxygen-style comments.  See
 * ~/linux/Documentation/kernel-doc-nano-HOWTO.txt for details.
 */

/*
 * Getting compilation warnings?  The Linux kernel is written against
 * C89, which means:
 *  - No // comments, and
 *  - All variables must be declared at the top of functions.
 * Read ~/linux/Documentation/CodingStyle to ensure your project
 * compiles without warnings.
 */

#define pr_fmt(fmt) "mastermind2: " fmt

#include <linux/capability.h>
#include <linux/cred.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include "nf_cs421net.h"

#define NUM_PEGS 4
#define NUM_COLORS 6

/** true if user is in the middle of a game */
struct mm_game
{
	struct list_head list;

	bool game_active;
	/** tracks number of guesses user has made */
	unsigned num_guesses;
	/** result of most recent user guess */
	char last_result[4];
	/** code that player is trying to guess */
	int target_code[NUM_PEGS];
	/** buffer that records all of user's guesses and their results */
	char *user_view;

	kuid_t id;
};

static LIST_HEAD(global_game);

static int num_games = 0;

static int num_games_started = 0;

size_t scnWrite = 0;

unsigned long flags;

static char *irq_cookie;

static int num_changes = 0;

static int num_invalid_changes = 0;

static int num_colors = 6;

static DEFINE_SPINLOCK(dev_lock);



static struct mm_game *mm_find_game(kuid_t uid){

	struct mm_game *game, *ret;
	
	list_for_each_entry(game, &global_game, list){
		pr_info("in list iterating");
		if(uid_eq(game->id, uid)){
			ret = container_of(game->list,struct mm_game,list);
			return ret;
		}
	}

	game = kmalloc(sizeof(*game), GFP_KERNEL);
	if(!game)
		return -ENOMEM;
	game->id = uid;
	game->user_view = vmalloc(PAGE_SIZE);
	list_add(&game->list, &global_game);
	return 0;

}

static void mm_free_games(void){
	struct mm_game *game, *tmp;
	list_for_each_entry_safe(game,tmp, &global_game, list){
		vfree(game->user_view);
		kfree(game);
	}
}

/*
* charToInt
* buf: characture to convert to int
* out puts correct int for char and -1 when invalid
*/
static int charToInt(char buf)
{
	int i, retVal;
	char stringInt[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

	retVal = -1;
	for (i = 0; i < 10; i++) {
		if (buf == stringInt[i]) {
			retVal = i;
		}
	}
	return retVal;
}

/**
 * mm_num_pegs() - calculate number of black pegs and number of white pegs
 * @target: target code, up to NUM_PEGS elements
 * @guess: user's guess, up to NUM_PEGS elements
 * @num_black: *OUT* parameter, to store calculated number of black pegs
 * @num_white: *OUT* parameter, to store calculated number of white pegs
 *
 * You do not need to modify this function.
 *
 */
static void mm_num_pegs(int target[], int guess[], unsigned *num_black,
			unsigned *num_white)
{
	size_t i;
	size_t j;
	bool peg_black[NUM_PEGS];
	bool peg_used[NUM_PEGS];

	*num_black = 0;
	for (i = 0; i < NUM_PEGS; i++) {
		if (guess[i] == target[i]) {
			(*num_black)++;
			peg_black[i] = true;
			peg_used[i] = true;
		} else {
			peg_black[i] = false;
			peg_used[i] = false;
		}
	}

	*num_white = 0;
	for (i = 0; i < NUM_PEGS; i++) {
		if (peg_black[i])
			continue;
		for (j = 0; j < NUM_PEGS; j++) {
			if (guess[i] == target[j] && !peg_used[j]) {
				peg_used[j] = true;
				(*num_white)++;
				break;
			}
		}
	}
}

/* Copy mm_read(), mm_write(), mm_mmap(), and mm_ctl_write(), along
 * with all of your global variables and helper functions here.
 */
/* Part 1: YOUR CODE HERE */
/**
 * mm_read() - callback invoked when a process reads from
 * /dev/mm
 * @filp: process's file object that is reading from this device (ignored)
 * @ubuf: destination buffer to store output
 * @count: number of bytes in @ubuf
 * @ppos: file offset (in/out parameter)
 *
 * Write to @ubuf the last result of the game, offset by
 * @ppos. Copy the lesser of @count and (string length of @last_result
 * - *@ppos). Then increment the value pointed to by @ppos by the
 * number of bytes copied. If @ppos is greater than or equal to the
 * length of @last_result, then copy nothing.
 *
 * If no game is active, instead copy to @ubuf up to four '?'
 * characters.
 *
 * Return: number of bytes written to @ubuf, or negative on error
 */
static ssize_t mm_read(struct file *filp, char __user * ubuf, size_t count,
		       loff_t * ppos)
{
	/* FIXME */
	char fourQ[] = { '?', '?', '?', '?' };
	kuid_t userID = (kuid_t)current_uid();
	struct mm_game *game = mm_find_game(userID);
	if (game->game_active == false) {
        spin_lock(&dev_lock);
		memcpy(game->last_result, fourQ, sizeof(fourQ));
        spin_unlock(&dev_lock);
		pr_info("game active is false\n");
	}
	if (*ppos >= sizeof(game->last_result)) {
		pr_info("ppos was greater\n");
		return 0;
	}
	if (*ppos + count > sizeof(game->last_result)) {
		pr_info("ppos + count was greater\n");
		count = sizeof(game->last_result) - *ppos;
	}
	if (copy_to_user(ubuf, game->last_result + *ppos, count) != 0) {
		pr_info("ppos + count was greater\n");
		return -EFAULT;
	}
	*ppos += count;

	return count;
}

/**
 * mm_write() - callback invoked when a process writes to /dev/mm
 * @filp: process's file object that is reading from this device (ignored)
 * @ubuf: source buffer from user
 * @count: number of bytes in @ubuf
 * @ppos: file offset (ignored)
 *
 * If the user is not currently playing a game, then return -EINVAL.
 *
 * If @count is less than NUM_PEGS, then return -EINVAL. Otherwise,
 * interpret the first NUM_PEGS characters in @ubuf as the user's
 * guess. Calculate how many are in the correct value and position,
 * and how many are simply the correct value. Then update
 * @num_guesses, @last_result, and @user_view.
 *
 * <em>Caution: @ubuf is NOT a string; it is not necessarily
 * null-terminated.</em> You CANNOT use strcpy() or strlen() on it!
 *
 * Return: @count, or negative on error
 */
static ssize_t mm_write(struct file *filp, const char __user * ubuf,
			size_t count, loff_t * ppos)
{
	/* FIXME */
	unsigned right, rightVal;
	int i;
	char blackPeg, whitePeg;
	char targetBuf[NUM_PEGS];
	int guess[NUM_PEGS];
	kuid_t userID = current_uid();
	struct mm_game *game = mm_find_game(userID);
    
	if (game->game_active == false || count < NUM_PEGS) {
		return -EINVAL;
	}
	//increment the number of guesses made if entry is valid
    spin_lock(&dev_lock);
	game->num_guesses = game->num_guesses + 1;
    spin_unlock(&dev_lock);

	//copy entry to buffer
	memcpy(targetBuf, ubuf, NUM_PEGS);

	//convert guess characters to int and store in an array
	for (i = 0; i < NUM_PEGS; i++) {
		if (charToInt(targetBuf[i]) == -1) {
			return -EINVAL;
		}
		guess[i] = charToInt(targetBuf[i]);
        if(guess[i] > num_colors){
            return -EINVAL;
        }
	}
	for (i = 0; i < NUM_PEGS; i++) {
		pr_info("guess %d: %d\n", i, guess[i]);
	}

	//get number of black and white pegs
	mm_num_pegs(game->target_code, guess, &right, &rightVal);

	//convert b and w values to string then store
	blackPeg = right + '0';
	whitePeg = rightVal + '0';

	pr_info("black Peg: %c\n", blackPeg);
	pr_info("white Peg: %c\n", whitePeg);

    spin_lock(&dev_lock);
	game->last_result[1] = blackPeg;
	game->last_result[3] = whitePeg;
	scnWrite +=
	    scnprintf(game->user_view + scnWrite, PAGE_SIZE - scnWrite,
		      "Guess %d: %c%c%c%c | %c%c%c%c \n", game->num_guesses,
		      targetBuf[0], targetBuf[1], targetBuf[2], targetBuf[3],
		      game->last_result[0], game->last_result[1], game->last_result[2],
		      game->last_result[3]);
    if(charToInt(blackPeg) == 4){
        scnWrite += scnprintf(game->user_view + scnWrite,PAGE_SIZE - scnWrite,"You Won the game! \n");
        game->game_active = false;
    }
    spin_unlock(&dev_lock);

	pr_info("history %s", game->user_view);
	return count;
}

/**
 * mm_mmap() - callback invoked when a process mmap()s to /dev/mm
 * @filp: process's file object that is mapping to this device (ignored)
 * @vma: virtual memory allocation object containing mmap() request
 *
 * Create a read-only mapping from kernel memory (specifically,
 * @user_view) into user space.
 *
 * Code based upon
 * <a href="http://bloggar.combitech.se/ldc/2015/01/21/mmap-memory-between-kernel-and-userspace/">http://bloggar.combitech.se/ldc/2015/01/21/mmap-memory-between-kernel-and-userspace/</a>
 *
 * You do not need to modify this function.
 *
 * Return: 0 on success, negative on error.
 */
static int mm_mmap(struct file *filp, struct vm_area_struct *vma)
{   
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);
	kuid_t userID = current_uid();
	struct mm_game *game = mm_find_game(userID);
	spin_lock(&dev_lock);
	unsigned long page = vmalloc_to_pfn(game->user_view);
	spin_unlock(&dev_lock);
	if (size > PAGE_SIZE){
		return -EIO;
    }
	vma->vm_pgoff = 0;
	vma->vm_page_prot = PAGE_READONLY;
	if (remap_pfn_range(vma, vma->vm_start, page, size, vma->vm_page_prot))
        return -EAGAIN;
	return 0;
}

/**
 * mm_ctl_write() - callback invoked when a process writes to
 * /dev/mm_ctl
 * @filp: process's file object that is writing to this device (ignored)
 * @ubuf: source buffer from user
 * @count: number of bytes in @ubuf
 * @ppos: file offset (ignored)
 *
 * Copy the contents of @ubuf, up to the lesser of @count and 8 bytes,
 * to a temporary location. Then parse that character array as
 * following:
 *
 *  start - Start a new game. If a game was already in progress, restart it.
 *  quit  - Quit the current game. If no game was in progress, do nothing.
 *
 * If the input is neither of the above, then return -EINVAL.
 *
 * <em>Caution: @ubuf is NOT a string;</em> it is not necessarily
 * null-terminated, nor does it necessarily have a trailing
 * newline. You CANNOT use strcpy() or strlen() on it!
 *
 * Return: @count, or negative on error
 */
static ssize_t mm_ctl_write(struct file *filp, const char __user * ubuf,
			    size_t count, loff_t * ppos)
{
	char start[5] = { 's', 't', 'a', 'r', 't' };
	char quit[] = { 'q', 'u', 'i', 't' };
    char color[] = { 'c','o','l','o','r','s'};
	char clearRes[] = { 'B', '-', 'W', '-' };
	char targetBuf[8];
    int colorChoice;
	size_t copyLn = 8;
	kuid_t userID = current_uid();
	struct mm_game *game = mm_find_game(userID);
	
	pr_info("mm_ctl_write all variables intialized and started\n");

	if (count < sizeof(targetBuf))
		copyLn = count;

	memcpy(targetBuf, ubuf, copyLn);
	pr_info("copied command %s\n", targetBuf);
	//pr_info("compare value memcmp: %d\n", memcmp(targetBuf, start, sizeof(targetBuf)));
	pr_info("compare value strncmp: %d\n",
		strncmp(targetBuf, start, count));

	if (strncmp(targetBuf, start, count) == 0) {
		//set the target code to 4211
        spin_lock(&dev_lock);
		game->target_code[0] = 4;
		game->target_code[1] = 2;
		game->target_code[2] = 1;
		game->target_code[3] = 1;
		pr_info("start\n");
		scnWrite = 0;
		//set number of guesses to 0
		game->num_guesses = 0;
		//set the user buffer to null
		memset(game->user_view, 0, PAGE_SIZE);
		pr_info("memset done\n");
		game->game_active = true;
        num_games += 1;
		memcpy(game->last_result, clearRes, 4);
		pr_info("copy to user done\n");
        spin_unlock(&dev_lock);
		return count;

	} else if (strncmp(targetBuf, quit, count) == 0) {

		pr_info("in quit if\n");
        spin_lock(&dev_lock);
		game->game_active = false;
        num_games -= 1;
        spin_unlock(&dev_lock);
		return count;
    } else if(strncmp(targetBuf, color, 6) == 0){
        colorChoice = charToInt(targetBuf[7]);
        pr_info("color: %d \n", colorChoice);
        if(capable(CAP_SYS_ADMIN)){
            pr_info("sys administrator permission\n");
        }
        else{
            pr_info("not sys administrator\n");
            return -EACCES;
        }
        if(colorChoice < 2 || colorChoice > 9){
            return -EINVAL;
        }
        spin_lock(&dev_lock);
        num_colors = colorChoice;
        spin_unlock(&dev_lock);
        return count;
    }
	pr_err("Invalid argument\n");
	return -EINVAL;
}

struct file_operations mm_fops = {
	.read = mm_read,
	.write = mm_write,
	.mmap = mm_mmap,
};

static struct miscdevice mm_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mm",
	.fops = &mm_fops,
	.mode = 0666,
};

struct file_operations mm_ctl_fops = {
	.write = mm_ctl_write,
};

static struct miscdevice mm_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mm_ctl",
	.fops = &mm_ctl_fops,
	.mode = 0666,
};

/**
 * cs421net_top() - top-half of CS421Net ISR
 * @irq: IRQ that was invoked (ignored)
 * @cookie: Pointer to data that was passed into
 * request_threaded_irq() (ignored)
 *
 * If @irq is CS421NET_IRQ, then wake up the bottom-half. Otherwise,
 * return IRQ_NONE.
 */
static irqreturn_t cs421net_top(int irq, void *cookie)
{
	/* Part 4: YOUR CODE HERE */
	irqreturn_t ret;

	if(irq != 6){
		return IRQ_NONE;
	}

	pr_info("This is top half\n");
	return IRQ_WAKE_THREAD;
}

int validate_data(char *data){
	int i;

	for(i = 0; i < 4; i++){
		pr_info("num first num: %c\n", data[i]);
		if(charToInt(data[i]) > num_colors || charToInt(data[i]) < 2){
			pr_info("numbers too big\n");
			return -1;
		}
	}

	return 1;
}

int set_code(char *data){
	int i;
	kuid_t userID = current_uid();
	struct mm_game *game = mm_find_game(userID);
	for(i = 0; i < 4; i++){
		game->target_code[i] = charToInt(data[i]);
	}
	return 1;
}


/**
 * cs421net_bottom() - bottom-half to CS421Net ISR
 * @irq: IRQ that was invoked (ignore)
 * @cookie: Pointer that was passed into request_threaded_irq()
 * (ignored)
 *
 * Fetch the incoming packet, via cs421net_get_data(). If:
 *   1. The packet length is exactly equal to four bytes, and
 *   2. If all characters in the packet are valid ASCII representation
 *      of valid digits in the code, then
 * Set the target code to the new code, and increment the number of
 * tymes the code was changed remotely. Otherwise, ignore the packet
 * and increment the number of invalid change attempts.
 *
 * Because the payload is dynamically allocated, free it after parsing
 * it.
 *
 * During Part 5, update this function to change all codes for all
 * active games.
 *
 * Remember to add appropriate spin lock calls in this function.
 *
 * <em>Caution: The incoming payload is NOT a string; it is not
 * necessarily null-terminated.</em> You CANNOT use strcpy() or
 * strlen() on it!
 *
 * Return: always IRQ_HANDLED
 */
static irqreturn_t cs421net_bottom(int irq, void *cookie)
{
	/* Part 4: YOUR CODE HERE */

	pr_info("This is bottom half\n");
	int ret;
	long unsigned int data_size = 4;
	long unsigned int * const dataSize = &data_size;
	char *data = cs421net_get_data(dataSize);

	ret = validate_data(data);
	if(ret == -1){
		spin_lock(&dev_lock);
		num_invalid_changes += 1;
		spin_unlock(&dev_lock);
		kfree(data);
		pr_info("incremented invalid changes\n");
		return IRQ_HANDLED;
	}
	pr_info("cooke first num: %c\n", *data);

	spin_lock(&dev_lock);
	num_changes += 1;
	set_code(data);
	pr_info("incremented valid changes\n");
	spin_unlock(&dev_lock);
	kfree(data);

	return IRQ_HANDLED;
}

size_t stat_write = 0;

/**
 * mm_stats_show() - callback invoked when a process reads from
 * /sys/devices/platform/mastermind/stats
 *
 * @dev: device driver data for sysfs entry (ignored)
 * @attr: sysfs entry context (ignored)
 * @buf: destination to store game statistics
 *
 * Write to @buf, up to PAGE_SIZE characters, a human-readable message
 * containing these game statistics:
 *   - Number of colors (range of digits in target code)
 *   - Number of started games
 *   - Number of active games
 *   - Number of valid network messages (see Part 4)
 *   - Number of invalid network messages (see Part 4)
 * Note that @buf is a normal character buffer, not a __user
 * buffer. Use scnprintf() in this function.
 *
 * @return Number of bytes written to @buf, or negative on error.
 */
static ssize_t mm_stats_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
    spin_lock(&dev_lock);
    stat_write += scnprintf(buf + stat_write,PAGE_SIZE - stat_write,"CS421 Mastermind Stats\nNumber of colors: %d\nNumber of Active Games: %d\nNumber of Active Games: %d\nNumber of times code was changed: %d\nNumber of invalid code change attempts: %d\n", num_colors, num_games,num_games_started,num_changes,num_invalid_changes);
	spin_unlock(&dev_lock);
    return stat_write;
}

static DEVICE_ATTR(stats, S_IRUGO, mm_stats_show, NULL);

/**
 * mastermind_probe() - callback invoked when this driver is probed
 * @pdev platform device driver data
 *
 * Return: 0 on successful probing, negative on error
 */
static int mastermind_probe(struct platform_device *pdev)
{
	/* Merge the contents of your original mastermind_init() here. */
	/* Part 1: YOUR CODE HERE */
    int retval;
	char cookie[4];
	pr_info("Initializing the game.\n");
	kuid_t userID = current_uid();
	struct mm_game *game;
	game = mm_find_game(userID);
	// if (!global_game->user_view) {
	// 	pr_err("Could not allocate memory\n");
	// 	return -ENOMEM;
	// }

	/* YOUR CODE HERE */
	retval = misc_register(&mm_dev);
	if (retval < 0)
		goto failedRegister;

	retval = misc_register(&mm_ctl_dev);
	if (retval < 0)
		goto failedSecondReg;

	pr_info("Successfully registered devices\n");
	/*
	 * You will need to integrate the following resource allocator
	 * into your code. That also means properly releasing the
	 * resource if the function fails.
	 */
	retval = device_create_file(&pdev->dev, &dev_attr_stats);
	if (retval < 0) {
		pr_err("Could not create sysfs entry\n");
        goto failedDeviceCreate;
	}

	irq_cookie = kmalloc(sizeof(cookie), GFP_KERNEL);
	cs421net_enable();
    retval = request_threaded_irq(CS421NET_IRQ,cs421net_top,cs421net_bottom,IRQF_SHARED,"mstr",irq_cookie);
    if (retval < 0) {
		pr_err("Could not register Interrupt handler\n");
        goto failedToRegisterHandler;
	}
	return 0;

failedToRegisterHandler:
    pr_err("Could not register Interrupt handler\n"); 
    device_remove_file(&pdev->dev, &dev_attr_stats);
failedDeviceCreate:
    pr_err("Could not create file");
    misc_deregister(&mm_ctl_dev);
failedSecondReg:
	pr_err("Could not register ctl device");
	misc_deregister(&mm_dev);
failedRegister:
	pr_err("Could not register micellanous device\n");
	mm_free_games();
	return -1;
}

/**
 * mastermind_remove() - callback when this driver is removed
 * @pdev platform device driver data
 *
 * Return: Always 0
 */
static int mastermind_remove(struct platform_device *pdev)
{
	/* Merge the contents of your original mastermind_exit() here. */
	/* Part 1: YOUR CODE HERE */
    pr_info("Freeing resources.\n");
	mm_free_games();
    free_irq(CS421NET_IRQ, irq_cookie);
	/* YOUR CODE HERE */
	misc_deregister(&mm_dev);
	misc_deregister(&mm_ctl_dev);
	device_remove_file(&pdev->dev, &dev_attr_stats);
	cs421net_disable();
	return 0;
}

static struct platform_driver cs421_driver = {
	.driver = {
		   .name = "mastermind",
		   },
	.probe = mastermind_probe,
	.remove = mastermind_remove,
};

static struct platform_device *pdev;

/**
 * cs421_init() -  create the platform driver
 * This is needed so that the device gains a sysfs group.
 *
 * <strong>You do not need to modify this function.</strong>
 */
static int __init cs421_init(void)
{
	pdev = platform_device_register_simple("mastermind", -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);
	return platform_driver_register(&cs421_driver);
}

/**
 * cs421_exit() - remove the platform driver
 * Unregister the driver from the platform bus.
 *
 * <strong>You do not need to modify this function.</strong>
 */
static void __exit cs421_exit(void)
{
	platform_driver_unregister(&cs421_driver);
	platform_device_unregister(pdev);
}

module_init(cs421_init);
module_exit(cs421_exit);

MODULE_DESCRIPTION("CS421 Mastermind Game++");
MODULE_LICENSE("GPL");