/*
 * CUSE: Character device in Userspace
 *
 * Copyright (C) 2008-2009  SUSE Linux Products GmbH
 * Copyright (C) 2008-2009  Tejun Heo <tj@kernel.org>
 *
 * This file is released under the GPLv2.
 *
 * CUSE enables character devices to be implemented from userland much
 * like FUSE allows filesystems.  On initialization /dev/cuse is
 * created.  By opening the file and replying to the CUSE_INIT request
 * userland CUSE server can create a character device.  After that the
 * operation is very similar to FUSE.
 *
 * A CUSE instance involves the following objects.
 *
 * cuse_conn	: contains fuse_conn and serves as bonding structure
 * channel	: file handle connected to the userland CUSE server
 * cdev		: the implemented character device
 * dev		: generic device for cdev
 *
 * Note that 'channel' is what 'dev' is in FUSE.  As CUSE deals with
 * devices, it's called 'channel' to reduce confusion.
 *
 * channel determines when the character device dies.  When channel is
 * closed, everything begins to destruct.  The cuse_conn is taken off
 * the lookup table preventing further access from cdev, cdev and
 * generic device are removed and the base reference of cuse_conn is
 * put.
 *
 * On each open, the matching cuse_conn is looked up and if found an
 * additional reference is taken which is released when the file is
 * closed.
 */

#include <linux/fuse.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/magic.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/uio.h>

#include "fuse_i.h"

#if defined(RPC_FS_PATCH)
#define CUSE_CONNTBL_LEN	256
#define MAX_RPC_FS_FILE_COUNT (256*8)
#else
#define CUSE_CONNTBL_LEN	64
#endif

struct cuse_conn {
	struct list_head	list;	/* linked on cuse_conntbl */
	struct fuse_conn	fc;	/* fuse connection */
	struct cdev		*cdev;	/* associated character device */
	struct device		*dev;	/* device representing @cdev */

	/* init parameters, set once during initialization */
	bool			unrestricted_ioctl;
#if defined(RPC_FS_PATCH)
	/* for normal used restricted fromat(fuse_ioctl_ext) ioctl case */
	bool            restricted_ioctl;
	/* for restricted case, but not contain a completed compat/un-compat
	   process, such as 64bit .ko + 64bit kernel + 32bit libfuse.so */
	bool            restricted_compat_ioctl;
#endif
};

static DEFINE_MUTEX(cuse_lock);		/* protects registration */
static struct list_head cuse_conntbl[CUSE_CONNTBL_LEN];
static struct class *cuse_class;
#if defined(RPC_FS_PATCH)
static struct file* files[MAX_RPC_FS_FILE_COUNT];
static long fd_bitmap[MAX_RPC_FS_FILE_COUNT/BITS_PER_LONG];
struct cuse_conn *rpc_main_cc = NULL;
#endif

static struct cuse_conn *fc_to_cc(struct fuse_conn *fc)
{
	return container_of(fc, struct cuse_conn, fc);
}

static struct list_head *cuse_conntbl_head(dev_t devt)
{
	return &cuse_conntbl[(MAJOR(devt) + MINOR(devt)) % CUSE_CONNTBL_LEN];
}


/**************************************************************************
 * CUSE frontend operations
 *
 * These are file operations for the character device.
 *
 * On open, CUSE opens a file from the FUSE mnt and stores it to
 * private_data of the open file.  All other ops call FUSE ops on the
 * FUSE file.
 */

static ssize_t cuse_read_iter(struct kiocb *kiocb, struct iov_iter *to)
{
	struct fuse_io_priv io = FUSE_IO_PRIV_SYNC(kiocb->ki_filp);
	loff_t pos = 0;

	return fuse_direct_io(&io, to, &pos, FUSE_DIO_CUSE);
}

static ssize_t cuse_write_iter(struct kiocb *kiocb, struct iov_iter *from)
{
	struct fuse_io_priv io = FUSE_IO_PRIV_SYNC(kiocb->ki_filp);
	loff_t pos = 0;
	/*
	 * No locking or generic_write_checks(), the server is
	 * responsible for locking and sanity checks.
	 */
	return fuse_direct_io(&io, from, &pos,
			      FUSE_DIO_WRITE | FUSE_DIO_CUSE);
}

static int cuse_open(struct inode *inode, struct file *file)
{
	dev_t devt = inode->i_cdev->dev;
	struct cuse_conn *cc = NULL, *pos;
	int rc;

	/* look up and get the connection */
	mutex_lock(&cuse_lock);
	list_for_each_entry(pos, cuse_conntbl_head(devt), list)
		if (pos->dev->devt == devt) {
			fuse_conn_get(&pos->fc);
			cc = pos;
			break;
		}
	mutex_unlock(&cuse_lock);

	/* dead? */
	if (!cc)
		return -ENODEV;

	/*
	 * Generic permission check is already done against the chrdev
	 * file, proceed to open.
	 */
	rc = fuse_do_open(&cc->fc, 0, file, 0);
	if (rc)
		fuse_conn_put(&cc->fc);
	return rc;
}

#if defined(RPC_FS_PATCH)
int cuse_dev_open(char *devname, bool compat)
{
	struct cuse_conn *cc = NULL, *pos;
	struct file *file;
	int rc, i, fd = -1;

	//TODO, find cuse_conntbl by device number
	/* look up and get the connection */
	mutex_lock(&cuse_lock);
	for (i = 0; i < CUSE_CONNTBL_LEN; ++i) {
		list_for_each_entry(pos, &cuse_conntbl[i], list)
			if (!strcmp(dev_name(pos->dev), devname)) {
			    fuse_conn_get(&pos->fc);
			    cc = pos;
			    break;
			}
	}
	mutex_unlock(&cuse_lock);

	/* dead? */
	if (!cc)
		return -ENODEV;

	mutex_lock(&cuse_lock);
	fd = find_next_zero_bit(fd_bitmap, MAX_RPC_FS_FILE_COUNT, 0);
	if (fd >= MAX_RPC_FS_FILE_COUNT) {
		printk("[cuse]failed to allocate fd\n");
	    mutex_unlock(&cuse_lock);
		return -1;
	}

	__set_bit(fd, fd_bitmap);
	file = kzalloc(sizeof(*file), GFP_KERNEL);
	if (!file) {
		printk("[cuse]failed to create file\n");
		__clear_bit(fd, fd_bitmap);
	    mutex_unlock(&cuse_lock);
		return -1;
	}
	mutex_unlock(&cuse_lock);

	file->f_flags = O_CREAT;
	files[fd] = file;

	/*
	 * Generic permission check is already done against the chrdev
	 * file, proceed to open.
	 */
	rc = fuse_do_open(&cc->fc, 0, file, 0);
	if (rc) {
		fuse_conn_put(&cc->fc);
		return -1;
	}

	if (compat)
		cc->restricted_compat_ioctl = true;
	else
		cc->restricted_compat_ioctl = false;

	return fd;
}

EXPORT_SYMBOL_GPL(cuse_dev_open);
#endif

static int cuse_release(struct inode *inode, struct file *file)
{
	struct fuse_file *ff = file->private_data;
#if defined(RPC_FS_PATCH)
	struct fuse_conn *fc = ff->sub_fc;
#else
	struct fuse_conn *fc = ff->fc;
#endif

	fuse_sync_release(ff, file->f_flags);
	fuse_conn_put(fc);

	return 0;
}

#if defined(RPC_FS_PATCH)
int cuse_dev_close(int fd)
{
	struct file *file = files[fd];
	int rc;

	rc = cuse_release(NULL, file);

	mutex_lock(&cuse_lock);
	__clear_bit(fd, fd_bitmap);
	mutex_unlock(&cuse_lock);

	if (file)
		kfree(file);

	return rc;
}
EXPORT_SYMBOL_GPL(cuse_dev_close);
#endif

static long cuse_file_ioctl(struct file *file, unsigned int cmd,
			    unsigned long arg)
{
	struct fuse_file *ff = file->private_data;
#if defined(RPC_FS_PATCH)
	struct cuse_conn *cc = fc_to_cc(ff->sub_fc);
#else
	struct cuse_conn *cc = fc_to_cc(ff->fc);
#endif
	unsigned int flags = 0;

#if defined(RPC_FS_PATCH)
	if (cc->restricted_compat_ioctl)
		flags |= FUSE_IOCTL_RESTRICTED_COMPAT;
	else if (cc->unrestricted_ioctl)
		flags |= FUSE_IOCTL_UNRESTRICTED;
	else if (cc->restricted_ioctl)
		flags |= FUSE_IOCTL_RESTRICTED;
#else
	if (cc->unrestricted_ioctl)
		flags |= FUSE_IOCTL_UNRESTRICTED;
#endif

	return fuse_do_ioctl(file, cmd, arg, flags);
}

static long cuse_file_compat_ioctl(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	struct fuse_file *ff = file->private_data;
	struct cuse_conn *cc = fc_to_cc(ff->fc);
	unsigned int flags = FUSE_IOCTL_COMPAT;

#if defined(RPC_FS_PATCH)
	if (cc->unrestricted_ioctl)
		flags |= FUSE_IOCTL_UNRESTRICTED;
	else if (cc->restricted_ioctl)
		flags |= FUSE_IOCTL_RESTRICTED;
#else
	if (cc->unrestricted_ioctl)
		flags |= FUSE_IOCTL_UNRESTRICTED;
#endif

	return fuse_do_ioctl(file, cmd, arg, flags);
}

#if defined(RPC_FS_PATCH)
int cuse_dev_ioctl(int fd, unsigned int cmd, unsigned long args)
{
	struct file *file = files[fd];

	return cuse_file_ioctl(file, cmd, args);
}
EXPORT_SYMBOL_GPL(cuse_dev_ioctl);
#endif

static const struct file_operations cuse_frontend_fops = {
	.owner			= THIS_MODULE,
	.read_iter		= cuse_read_iter,
	.write_iter		= cuse_write_iter,
	.open			= cuse_open,
	.release		= cuse_release,
	.unlocked_ioctl		= cuse_file_ioctl,
	.compat_ioctl		= cuse_file_compat_ioctl,
	.poll			= fuse_file_poll,
	.llseek		= noop_llseek,
};


/**************************************************************************
 * CUSE channel initialization and destruction
 */

struct cuse_devinfo {
	const char		*name;
};

/**
 * cuse_parse_one - parse one key=value pair
 * @pp: i/o parameter for the current position
 * @end: points to one past the end of the packed string
 * @keyp: out parameter for key
 * @valp: out parameter for value
 *
 * *@pp points to packed strings - "key0=val0\0key1=val1\0" which ends
 * at @end - 1.  This function parses one pair and set *@keyp to the
 * start of the key and *@valp to the start of the value.  Note that
 * the original string is modified such that the key string is
 * terminated with '\0'.  *@pp is updated to point to the next string.
 *
 * RETURNS:
 * 1 on successful parse, 0 on EOF, -errno on failure.
 */
static int cuse_parse_one(char **pp, char *end, char **keyp, char **valp)
{
	char *p = *pp;
	char *key, *val;

	while (p < end && *p == '\0')
		p++;
	if (p == end)
		return 0;

	if (end[-1] != '\0') {
		printk(KERN_ERR "CUSE: info not properly terminated\n");
		return -EINVAL;
	}

	key = val = p;
	p += strlen(p);

	if (valp) {
		strsep(&val, "=");
		if (!val)
			val = key + strlen(key);
		key = strstrip(key);
		val = strstrip(val);
	} else
		key = strstrip(key);

	if (!strlen(key)) {
		printk(KERN_ERR "CUSE: zero length info key specified\n");
		return -EINVAL;
	}

	*pp = p;
	*keyp = key;
	if (valp)
		*valp = val;

	return 1;
}

/**
 * cuse_parse_dev_info - parse device info
 * @p: device info string
 * @len: length of device info string
 * @devinfo: out parameter for parsed device info
 *
 * Parse @p to extract device info and store it into @devinfo.  String
 * pointed to by @p is modified by parsing and @devinfo points into
 * them, so @p shouldn't be freed while @devinfo is in use.
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
static int cuse_parse_devinfo(char *p, size_t len, struct cuse_devinfo *devinfo)
{
	char *end = p + len;
	char *uninitialized_var(key), *uninitialized_var(val);
	int rc;

	while (true) {
		rc = cuse_parse_one(&p, end, &key, &val);
		if (rc < 0)
			return rc;
		if (!rc)
			break;
		if (strcmp(key, "DEVNAME") == 0)
			devinfo->name = val;
		else
			printk(KERN_WARNING "CUSE: unknown device info \"%s\"\n",
			       key);
	}

	if (!devinfo->name || !strlen(devinfo->name)) {
		printk(KERN_ERR "CUSE: DEVNAME unspecified\n");
		return -EINVAL;
	}

	return 0;
}

static void cuse_gendev_release(struct device *dev)
{
	kfree(dev);
}

/**
 * cuse_process_init_reply - finish initializing CUSE channel
 *
 * This function creates the character device and sets up all the
 * required data structures for it.  Please read the comment at the
 * top of this file for high level overview.
 */
static void cuse_process_init_reply(struct fuse_conn *fc, struct fuse_req *req)
{
	struct cuse_conn *cc = fc_to_cc(fc), *pos;
	struct cuse_init_out *arg = req->out.args[0].value;
	struct page *page = req->pages[0];
	struct cuse_devinfo devinfo = { };
	struct device *dev;
	struct cdev *cdev;
	dev_t devt;
	int rc, i;

	if (req->out.h.error ||
	    arg->major != FUSE_KERNEL_VERSION || arg->minor < 11) {
		goto err;
	}

	fc->minor = arg->minor;
	fc->max_read = max_t(unsigned, arg->max_read, 4096);
	fc->max_write = max_t(unsigned, arg->max_write, 4096);

	/* parse init reply */
#if defined(RPC_FS_PATCH)
	if (arg->flags & CUSE_UNRESTRICTED_IOCTL)
		cc->unrestricted_ioctl = true;
	else if (arg->flags & CUSE_RESTRICTED_IOCTL)
		cc->restricted_ioctl = true;
#else
	cc->unrestricted_ioctl = arg->flags & CUSE_UNRESTRICTED_IOCTL;
#endif

	rc = cuse_parse_devinfo(page_address(page), req->out.args[1].size,
				&devinfo);
	if (rc)
		goto err;

	/* determine and reserve devt */
	devt = MKDEV(arg->dev_major, arg->dev_minor);
	if (!MAJOR(devt))
		rc = alloc_chrdev_region(&devt, MINOR(devt), 1, devinfo.name);
	else
		rc = register_chrdev_region(devt, 1, devinfo.name);
	if (rc) {
		printk(KERN_ERR "CUSE: failed to register chrdev region\n");
		goto err;
	}

	/* devt determined, create device */
	rc = -ENOMEM;
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		goto err_region;

	device_initialize(dev);
	dev_set_uevent_suppress(dev, 1);
	dev->class = cuse_class;
	dev->devt = devt;
	dev->release = cuse_gendev_release;
	dev_set_drvdata(dev, cc);
	dev_set_name(dev, "%s", devinfo.name);

	mutex_lock(&cuse_lock);

	/* make sure the device-name is unique */
	for (i = 0; i < CUSE_CONNTBL_LEN; ++i) {
		list_for_each_entry(pos, &cuse_conntbl[i], list)
			if (!strcmp(dev_name(pos->dev), dev_name(dev)))
				goto err_unlock;
	}

	rc = device_add(dev);
	if (rc)
		goto err_unlock;

	/* register cdev */
	rc = -ENOMEM;
	cdev = cdev_alloc();
	if (!cdev)
		goto err_unlock;

	cdev->owner = THIS_MODULE;
	cdev->ops = &cuse_frontend_fops;

	rc = cdev_add(cdev, devt, 1);
	if (rc)
		goto err_cdev;

	cc->dev = dev;
	cc->cdev = cdev;

#if defined(RPC_FS_PATCH)
	if (!strcmp(devinfo.name, "rpc_main")) {
		rpc_main_cc = cc;
	}
	cc->fc.main_fc = &rpc_main_cc->fc;
	strcpy(cc->fc.name, dev_name(cc->dev));
#endif

	/* make the device available */
	list_add(&cc->list, cuse_conntbl_head(devt));
	mutex_unlock(&cuse_lock);

	/* announce device availability */
	dev_set_uevent_suppress(dev, 0);
	kobject_uevent(&dev->kobj, KOBJ_ADD);
out:
	kfree(arg);
	__free_page(page);
	return;

err_cdev:
	cdev_del(cdev);
err_unlock:
	mutex_unlock(&cuse_lock);
	put_device(dev);
err_region:
	unregister_chrdev_region(devt, 1);
err:
	fuse_abort_conn(fc);
	goto out;
}

static int cuse_send_init(struct cuse_conn *cc)
{
	int rc;
	struct fuse_req *req;
	struct page *page;
	struct fuse_conn *fc = &cc->fc;
	struct cuse_init_in *arg;
	void *outarg;

	BUILD_BUG_ON(CUSE_INIT_INFO_MAX > PAGE_SIZE);

	req = fuse_get_req_for_background(fc, 1);
	if (IS_ERR(req)) {
		rc = PTR_ERR(req);
		goto err;
	}

	rc = -ENOMEM;
	page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!page)
		goto err_put_req;

	outarg = kzalloc(sizeof(struct cuse_init_out), GFP_KERNEL);
	if (!outarg)
		goto err_free_page;

	arg = &req->misc.cuse_init_in;
	arg->major = FUSE_KERNEL_VERSION;
	arg->minor = FUSE_KERNEL_MINOR_VERSION;
	arg->flags |= CUSE_UNRESTRICTED_IOCTL;
	req->in.h.opcode = CUSE_INIT;
	req->in.numargs = 1;
	req->in.args[0].size = sizeof(struct cuse_init_in);
	req->in.args[0].value = arg;
	req->out.numargs = 2;
	req->out.args[0].size = sizeof(struct cuse_init_out);
	req->out.args[0].value = outarg;
	req->out.args[1].size = CUSE_INIT_INFO_MAX;
	req->out.argvar = 1;
	req->out.argpages = 1;
	req->pages[0] = page;
	req->page_descs[0].length = req->out.args[1].size;
	req->num_pages = 1;
	req->end = cuse_process_init_reply;
	fuse_request_send_background(fc, req);

	return 0;

err_free_page:
	__free_page(page);
err_put_req:
	fuse_put_request(fc, req);
err:
	return rc;
}

static void cuse_fc_release(struct fuse_conn *fc)
{
	struct cuse_conn *cc = fc_to_cc(fc);
	kfree_rcu(cc, fc.rcu);
}

/**
 * cuse_channel_open - open method for /dev/cuse
 * @inode: inode for /dev/cuse
 * @file: file struct being opened
 *
 * Userland CUSE server can create a CUSE device by opening /dev/cuse
 * and replying to the initialization request kernel sends.  This
 * function is responsible for handling CUSE device initialization.
 * Because the fd opened by this function is used during
 * initialization, this function only creates cuse_conn and sends
 * init.  The rest is delegated to a kthread.
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
static int cuse_channel_open(struct inode *inode, struct file *file)
{
	struct fuse_dev *fud;
	struct cuse_conn *cc;
	int rc;

	/* set up cuse_conn */
	cc = kzalloc(sizeof(*cc), GFP_KERNEL);
	if (!cc)
		return -ENOMEM;

	fuse_conn_init(&cc->fc);

	fud = fuse_dev_alloc(&cc->fc);
	if (!fud) {
		kfree(cc);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&cc->list);
	cc->fc.release = cuse_fc_release;

	cc->fc.initialized = 1;
	rc = cuse_send_init(cc);
	if (rc) {
		fuse_dev_free(fud);
		return rc;
	}
	file->private_data = fud;

	return 0;
}

/**
 * cuse_channel_release - release method for /dev/cuse
 * @inode: inode for /dev/cuse
 * @file: file struct being closed
 *
 * Disconnect the channel, deregister CUSE device and initiate
 * destruction by putting the default reference.
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
static int cuse_channel_release(struct inode *inode, struct file *file)
{
	struct fuse_dev *fud = file->private_data;
	struct cuse_conn *cc = fc_to_cc(fud->fc);
	int rc;

	/* remove from the conntbl, no more access from this point on */
	mutex_lock(&cuse_lock);
	list_del_init(&cc->list);
	mutex_unlock(&cuse_lock);

	/* remove device */
	if (cc->dev)
		device_unregister(cc->dev);
	if (cc->cdev) {
		unregister_chrdev_region(cc->cdev->dev, 1);
		cdev_del(cc->cdev);
	}
	/* Base reference is now owned by "fud" */
	fuse_conn_put(&cc->fc);

	rc = fuse_dev_release(inode, file);	/* puts the base reference */

	return rc;
}

static struct file_operations cuse_channel_fops; /* initialized during init */


/**************************************************************************
 * Misc stuff and module initializatiion
 *
 * CUSE exports the same set of attributes to sysfs as fusectl.
 */

static ssize_t cuse_class_waiting_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct cuse_conn *cc = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", atomic_read(&cc->fc.num_waiting));
}
static DEVICE_ATTR(waiting, 0400, cuse_class_waiting_show, NULL);

static ssize_t cuse_class_abort_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct cuse_conn *cc = dev_get_drvdata(dev);

	fuse_abort_conn(&cc->fc);
	return count;
}
static DEVICE_ATTR(abort, 0200, NULL, cuse_class_abort_store);

static struct attribute *cuse_class_dev_attrs[] = {
	&dev_attr_waiting.attr,
	&dev_attr_abort.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cuse_class_dev);

static struct miscdevice cuse_miscdev = {
	.minor		= CUSE_MINOR,
	.name		= "cuse",
	.fops		= &cuse_channel_fops,
};

MODULE_ALIAS_MISCDEV(CUSE_MINOR);
MODULE_ALIAS("devname:cuse");

static int __init cuse_init(void)
{
	int i, rc;

	/* init conntbl */
	for (i = 0; i < CUSE_CONNTBL_LEN; i++)
		INIT_LIST_HEAD(&cuse_conntbl[i]);

	/* inherit and extend fuse_dev_operations */
	cuse_channel_fops		= fuse_dev_operations;
	cuse_channel_fops.owner		= THIS_MODULE;
	cuse_channel_fops.open		= cuse_channel_open;
	cuse_channel_fops.release	= cuse_channel_release;

	cuse_class = class_create(THIS_MODULE, "cuse");
	if (IS_ERR(cuse_class))
		return PTR_ERR(cuse_class);

	cuse_class->dev_groups = cuse_class_dev_groups;

	rc = misc_register(&cuse_miscdev);
	if (rc) {
		class_destroy(cuse_class);
		return rc;
	}

	return 0;
}

static void __exit cuse_exit(void)
{
	misc_deregister(&cuse_miscdev);
	class_destroy(cuse_class);
}

module_init(cuse_init);
module_exit(cuse_exit);

MODULE_AUTHOR("Tejun Heo <tj@kernel.org>");
MODULE_DESCRIPTION("Character device in Userspace");
MODULE_LICENSE("GPL");
