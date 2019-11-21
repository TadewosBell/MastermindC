/* Name: Tadewos Bellete
   Email: bell6@umbc.edu
   Test 
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

#define pr_fmt(fmt) "MM: " fmt

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#define NUM_PEGS 4
#define NUM_COLORS 6

/** true if user is in the middle of a game */
static bool game_active;


/** code that player is trying to guess */
static int target_code[NUM_PEGS];

/** tracks number of guesses user has made */
static unsigned num_guesses;

/** result of most recent user guess */
static char last_result[4];

/** buffer that records all of user's guesses and their results */
static char *user_view;

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
  char fourQ[] = {'?','?','?','?'};

  if(game_active == false){
    memcpy(last_result,fourQ, sizeof(fourQ));
    pr_info("game active is false\n");
  }
  if(*ppos >= sizeof(last_result)){
    pr_info("ppos was greater\n");
    return 0;
  }
  if(*ppos + count > sizeof(last_result)){
    pr_info("ppos + count was greater\n");
    count = sizeof(last_result) - *ppos;
  }
  if(copy_to_user(ubuf, last_result + *ppos, count) != 0){
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
  int right,rightVal, retVal, i;
  char targetBuf[NUM_PEGS];

  if(game_active == false || count < NUM_PEGS){
    return -EINVAL;
  }

  retVal = memcpy(targetBuf,ubuf, NUM_PEGS);

  for(i = 0; i < NUM_PEGS; i++){
    pr_info("input %d: %s\n", targetBuf[i]);
  }

	return -EPERM;
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
	unsigned long page = vmalloc_to_pfn(user_view);
	if (size > PAGE_SIZE)
		return -EIO;
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
  /* FIXME */
  int retVal;
  int i;
  char start[5] = {'s','t','a','r','t'};
  bool isStart;
  isStart = true;
  char quit[] = {'q','u','i','t'};
  bool isQuit = true;
  char clearRes[] = {'B','-','W','-'};
  char targetBuf[8];
  size_t copyLn = 8;
  pr_info("mm_ctl_write all variables intialized and started\n");
  if(count < sizeof(targetBuf)) copyLn = count;
  
  retVal = memcpy(targetBuf,ubuf, copyLn);
  pr_info("copied command %s\n", targetBuf);
  pr_info("compare value memcmp: %d\n", memcmp(targetBuf, start, sizeof(targetBuf)));
  pr_info("compare value strncmp: %d\n", strncmp(targetBuf, start, count));
  for(i = 0; i < sizeof(ubuf); i++ ){
    if(targetBuf[i] != start[i]){
      //isStart = false;
      pr_info("is not start\n");
    }
    else{
      pr_info("is Start\n");
    }
  }
  if(strncmp(targetBuf, start, count) == 0){
    //set the target code to 4211
    target_code[0] = 4;
    target_code[1] = 2;
    target_code[2] = 1;
    target_code[3] = 1;
    pr_info("it was start\n");
    //set number of guesses to 0
    num_guesses = 0;

    //set the user buffer to null
    memset(user_view, 0, sizeof(user_view));

    pr_info("memset done\n");
    game_active = true;

    retVal = copy_to_user(last_result, clearRes, 4);

    pr_info("copy to user done\n");
    
    return count;
  }
  else if(strncmp(targetBuf, quit, count) == 0){
    pr_info("in quit if\n");
    game_active = false;
    return count;
  }
  
  pr_err("Invalid argument\n");
  return -1;
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
 * mastermind_init() - entry point into the Mastermind kernel module
 * Return: 0 on successful initialization, negative on error
 */
static int __init mastermind_init(void)
{
  int retval;
  pr_info("Initializing the game.\n");
  user_view = vmalloc(PAGE_SIZE);
  if (!user_view) {
    pr_err("Could not allocate memory\n");
    return -ENOMEM;
  }
  
  /* YOUR CODE HERE */
  retval = misc_register(&mm_dev);
  if(retval < 0) goto failedRegister;

  retval = misc_register(&mm_ctl_dev);
  if(retval < 0) goto failedSecondReg;

  pr_info("Successfully registered devices\n");
  return 0;

 failedRegister:
  pr_err("Could not register micellanous device\n");
  vfree(user_view);
  return -1;

 failedSecondReg:   
  pr_err("Could not register ctl device");
  vfree(user_view);
  misc_deregister(&mm_dev);
  return -1;
}

/**
 * mastermind_exit() - called by kernel to clean up resources
 */
static void __exit mastermind_exit(void)
{
	pr_info("Freeing resources.\n");
	vfree(user_view);

	/* YOUR CODE HERE */
	misc_deregister(&mm_dev);
  misc_deregister(&mm_ctl_dev);
}

module_init(mastermind_init);
module_exit(mastermind_exit);

MODULE_DESCRIPTION("CS421 Mastermind Game");
MODULE_LICENSE("GPL");
