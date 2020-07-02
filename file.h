#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/uaccess.h>
#include <asm/current.h>
#include <linux/list.h>
#include <linux/sched.h>
#define TMPSIZE 20


char* getPIDinfo(struct task_struct* task)
{
    struct list_head *children_tasks;
    char* info;
    children_tasks = &(task->children);
    
    list_for_each(children_tasks, &(task->children))
    {
        struct task_struct *child_task;
        child_task = list_entry(children_tasks, struct task_struct, sibling);
	info = child_task->comm;	
        getPIDinfo(child_task);
    }
	return 0;
}

int get_task_info(int pid, char* data)
{
	char* info2;
        struct task_struct *task=current;
        while(task->pid != 1 ){
        task=task->parent;
        }
        info2 = getPIDinfo(task);
        return 0;
}

/*
 * Open a file.  All we have to do here is to copy over a
 * copy of the counter pointer so it's easier to get at.
 */
static int s2fs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

/*
 * Read a file.  Here we increment and read the counter, then pass it
 * back to the caller.  The increment only happens if the read is done
 * at the beginning of the file (offset = 0); otherwise we end up counting
 * by twos.
 */

static ssize_t s2fs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{	
	atomic_t *counter = (atomic_t *) filp->private_data;
        int v, len;
        char tmp[TMPSIZE];

/*
 * Encode the value, and figure out how much of it we can pass back.
 */
        v = atomic_read(counter);
	get_task_info(v,buf);

        if (*offset > 0)
                v -= 1;  /* the value returned when offset was zero */
        else
                atomic_inc(counter);
        len = snprintf(tmp, TMPSIZE, "No. of bytes: %d\n", v);
        if (*offset > len)
                return 0;
        if (count > len - *offset)
                count = len - *offset;
/*
 * Copy it back, increment the offset, and we're done.
 */
        if (copy_to_user(buf, tmp + *offset, count))
                return -EFAULT;
        *offset += count;
        return count;

}

/*
 * Write a file.
 */
static ssize_t s2fs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	atomic_t *counter = (atomic_t *) filp->private_data;
	char tmp[TMPSIZE];
/*
 * Only write from the beginning.
 */
	if (*offset != 0)
		return -EINVAL;
/*
 * Read the value from the user.
 */
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
/*
 * Store it in the counter and we are done.
 */
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	return count;
}

static struct file_operations s2fs_fops = {
        .open   = s2fs_open,
        .read   = s2fs_read_file,
        .write  = s2fs_write_file,
};

static struct dentry *s2fs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;

/*
 * Now we can create our dentry and the inode to go with it.
 */
	dentry = d_alloc_name(dir, name);
	if (! dentry)
		goto out;
	inode = s2fs_make_inode(sb, S_IFREG|0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &s2fs_fops;
	inode->i_private = counter;
/*
 * Put it all into the dentry cache and we're done.
 */
	d_add(dentry, inode);
	return dentry;
/*
 * Then again, maybe it didn't work.
 */
  out_dput:
	dput(dentry);
  out:
	return 0;
}
