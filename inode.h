#include <linux/fs.h>

static struct inode *s2fs_make_inode(struct super_block *sb, int mode)
{
	struct inode* inode;            
        inode = new_inode(sb);
        
	if (inode) {
        	inode->i_mode = mode;
        	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        	inode->i_blocks = 0;
        	inode->i_ino = get_next_ino();
	}
	return inode;

}
