#include <linux/fs.h>

static struct dentry *s2fs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *dir_name)
{
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, dir_name);
	if (! dentry)
		goto out;

	inode = s2fs_make_inode(sb, S_IFDIR|0755);
	if (! inode)
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}
