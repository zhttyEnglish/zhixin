#ifndef __ICC_H__
#define __ICC_H__

#include <linux/ioctl.h>
#include <linux/types.h>

struct icc_ioctl_io_request {
	int flag;
	int msg_id;
	int target_core;
	int size;
	unsigned long buf;
};

struct icc_ioctl_io_request_32 {
	int flag;
	int msg_id;
	int target_core;
	int size;
	unsigned int buf;
};

struct icc_ioctl_reg_msg {
	int msg_id;
};

struct icc_ioctl_timeout {
	int timeout;
};

#define ICC_IOC_MAGIC         'I'
#define ICC_IOC_SEND          _IOWR(ICC_IOC_MAGIC, 0, struct icc_ioctl_io_request)
#define ICC_IOC_RECEIVE       _IOWR(ICC_IOC_MAGIC, 1, struct icc_ioctl_io_request)
#define ICC_IOC_REGISTER_MSG  _IOWR(ICC_IOC_MAGIC, 2, struct icc_ioctl_reg_msg)
#define ICC_IOC_CANCLE_MSG    _IOWR(ICC_IOC_MAGIC, 3, struct icc_ioctl_reg_msg)
#define ICC_IOC_SET_TIMEOUT   _IOWR(ICC_IOC_MAGIC, 4, struct icc_ioctl_timeout)

#define ICC_IOC_SEND_32          _IOWR(ICC_IOC_MAGIC, 0, struct icc_ioctl_io_request_32)
#define ICC_IOC_RECEIVE_32       _IOWR(ICC_IOC_MAGIC, 1, struct icc_ioctl_io_request_32)
#define ICC_IOC_REGISTER_MSG_32  _IOWR(ICC_IOC_MAGIC, 2, struct icc_ioctl_reg_msg)
#define ICC_IOC_CANCLE_MSG_32    _IOWR(ICC_IOC_MAGIC, 3, struct icc_ioctl_reg_msg)
#define ICC_IOC_SET_TIMEOUT_32   _IOWR(ICC_IOC_MAGIC, 4, struct icc_ioctl_timeout)

#endif
