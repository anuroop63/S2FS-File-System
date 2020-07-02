#include <linux/init.h>
#include <linux/module.h>
#include <asm/current.h>
#include <linux/kernel.h>

#include "inode.h"
#include "file.h"
#include "dir.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anuroop Katiyar");

#define S2FS_MAGIC 0x20160105

struct dentry *subdir;
int val1,val2;
static atomic_t counter;

void getProcInfo(struct task_struct* task,struct super_block *sb)
{
	struct dentry *pdir;
	char cpid[TMPSIZE];
    struct list_head *children_tasks;
    children_tasks = &(task->children);
    list_for_each(children_tasks, &(task->children))
    {
	struct task_struct *child_task;
        child_task = list_entry(children_tasks, struct task_struct, sibling);
        val2=child_task->pid;
	snprintf(cpid,TMPSIZE,"%d",val2);
	atomic_set(&counter, 0);

        if (subdir){
		if (child_task->parent->pid == val1)
            s2fs_create_file(sb, subdir, cpid, &counter);
		else
			pdir=s2fs_create_dir(sb, subdir, cpid);
	}
	getProcInfo(child_task,sb);
    }
}

static atomic_t subcounter;

static void s2fs_create_files (struct super_block *sb, struct dentry *root)
{
	char ppid[TMPSIZE];
	struct task_struct *task=current;

        while (task->pid != 1){
          task=task->parent;
        }
	
	val1=task->pid;
	snprintf(ppid,TMPSIZE,"%d",val1);

        atomic_set(&subcounter, 0);
        subdir = s2fs_create_dir(sb, root, ppid);
        if (subdir)
	{
		s2fs_create_file(sb, subdir, ppid, &subcounter);
		getProcInfo(task,sb);
	}
}


/*
 * Our superblock operations, both of which are generic kernel ops
 * that we don't have to write ourselves.
 */
static struct super_operations s2fs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};


static int s2fs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;

/*
 * Basic parameters.
 */
	sb->s_magic = S2FS_MAGIC;
	sb->s_op = &s2fs_s_ops;
	sb->s_blocksize = VMACACHE_SIZE;
	sb->s_blocksize_bits = VMACACHE_SIZE;
/*
 * We need to conjure up an inode to represent the root directory
 * of this filesystem.  Its operations all come from libfs, so we
 * don't have to mess with actually *doing* things inside this
 * directory.
 */	
	root = s2fs_make_inode (sb, S_IFDIR | 0755);
	inode_init_owner(root, NULL, S_IFDIR|0755);
	
	if (! root)
	{
	   pr_err("inode allocation failed\n");
           return -ENOMEM;
	}
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	set_nlink(root, 2);
	root_dentry = d_make_root(root);

	if (! root_dentry)
	  iput(root);

	s2fs_create_files (sb, root_dentry);
	sb->s_root = root_dentry;	
	return 0;
	
}


/*
 * Stuff to pass in when registering the filesystem.
 */
static struct dentry *s2fs_mount(struct file_system_type *fst,
		int flags, const char *devname, void *data)
{
	struct dentry *const entry = mount_nodev(fst, flags, data, s2fs_fill_super);
      if (IS_ERR(entry))
          pr_err("s2fs mounting failed\n");
      else
          pr_debug("s2fs mounted successfully\n");
      return entry;
}

static struct file_system_type s2fs_type = {
	.owner 		= THIS_MODULE,
	.name		= "s2fs",
	.mount		= s2fs_mount,
	.kill_sb	= kill_litter_super,

};


/*
 * Get things set up.
 */
static int __init s2fs_init(void)
{
	return register_filesystem(&s2fs_type);
}

static void __exit s2fs_exit(void)
{
	unregister_filesystem(&s2fs_type);
}

module_init(s2fs_init);
module_exit(s2fs_exit);
