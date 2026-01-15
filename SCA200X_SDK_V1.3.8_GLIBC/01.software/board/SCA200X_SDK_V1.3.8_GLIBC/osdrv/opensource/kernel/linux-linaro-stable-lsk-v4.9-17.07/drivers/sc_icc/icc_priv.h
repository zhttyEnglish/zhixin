#ifndef __ICC_PRIV_H__
#define __ICC_PRIV_H__

#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include "icc_shm.h"

#define ICC_RECORD_MSG
#define ICC_RECORD_MSG_NUM 16

#define icc_debug(fmt, ...)  \
    do {}while (0);
//  pr_debug("[Icc debug] %s %d, " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define icc_error(fmt, ...)  \
    pr_emerg("[Icc err] %s %d, " fmt, __func__, __LINE__, ##__VA_ARGS__)

enum {
	ICC_CORE_A_0,
	ICC_CORE_A_1,
	ICC_CORE_A_2,
	ICC_CORE_A_3,

	ICC_CORE_M_0,
	ICC_CORE_M_1,
	ICC_CORE_M_2,
	ICC_CORE_M_3,

	ICC_CORE_CEVA_0,
	ICC_CORE_CEVA_1,
	ICC_CORE_CEVA_2,
	ICC_CORE_CEVA_3,

	ICC_CORE_TOTAL,
};

#define ICC_INVALID_MSG_ID       (~0)

#define ICC_CLIENT_MSG_Q_NUM     512 //128

#define ICC_CLIENT_MSG_TRHESHOLD 0x40

#define ICC_REQ_FLAG_UBUF (1 << 0)

#define ICC_REQ_FLAG_TX   (1 << 1)

/* Default timeout 30s */
#define ICC_DEFAULT_TIMEOUT 30000

struct icc_shm {
	struct icc_shm_mgmt *shm_mgmt;
	unsigned char       *shm_buff;
	struct dentry       *dbg_file;
};

struct icc_peer;
struct icc_peer_table;

struct icc_peer_ops {
	/* wakeup core, core_id: src core id */
	int  (*notify)(struct icc_peer *peer);
	void (*wakeup)(struct icc_peer *peer);
};

struct icc_peer {
	struct list_head      list;
	struct mutex          mutex;
	struct icc_core       *icc;
	struct icc_peer_ops   *peer_ops;
	struct icc_peer_table *peer_tbl;
	int                   core_id;
	int                   valid;
	char                  *name;
	void                  *priv;
	struct dentry         *dbg_file;
};

struct icc_core {
	struct list_head   client_list;
	struct mutex       mutex;
	struct icc_shm     shm;
	spinlock_t         lock;
	phys_addr_t        phys_addr;
	void               *virt_addr;
	size_t             mem_size;
	int                core_id;
	struct miscdevice  dev;
	struct task_struct *rcv_tsk;
	struct icc_peer    peer[ICC_CORE_TOTAL];
	struct idr         idr;
	struct dentry      *dbg_dir;
	struct dentry      *clients_dbg_dir;
};

struct icc_msg_queue {
	struct list_head   msg_list;
	unsigned int       msg_id;
	unsigned int       msg_num;
	struct icc_request *rx_request;
};

struct icc_msg {
	struct list_head    list;
	unsigned int        core_id;
	unsigned char       *kbuf;
	unsigned int        size;
};

struct icc_request {
	struct list_head list;
	struct icc_client *client;
	struct task_struct *task;
	/* kernel or user buffer */
	int flag;
	int msg_id;
	int core_id;
	size_t size;
	unsigned char *kbuf;
	unsigned char __user *ubuf;
	int res;
	struct completion done;
};

struct icc_client {
	struct list_head      list;
	struct task_struct    *tsk;
	struct icc_core       *icc;
	struct mutex          mutex;
	unsigned char         *name;
	int                   id;
	int                   timeout;
	struct icc_msg_queue  msg_rx_queue[ICC_CLIENT_MSG_Q_NUM];
	struct dentry         *dbg_file;
#ifdef ICC_RECORD_MSG
	struct {
		int msg_id;
		int core_id;
		size_t size;
		struct task_struct *task;
	} last_tx[ICC_RECORD_MSG_NUM], last_rx[ICC_RECORD_MSG_NUM];
	int record_rx_pos;
	int record_tx_pos;
#endif
};

struct icc_peer_table {
	struct list_head list;
	unsigned char *name;
	int (*create)(struct icc_peer *peer, struct device_node *node);
	int (*destroy)(struct icc_peer *peer);
	int (*get_coreid)(struct device_node *node);
};

struct icc_core *icc_core_create(void);
int icc_core_destroy(struct icc_core *icc);
int icc_vmap_mem(struct icc_core *icc);
int icc_of_mem_init(struct platform_device *pdev);
int icc_of_peer_init(struct platform_device *pdev);
int icc_peer_create(struct icc_core *icc, struct device_node *node);
void icc_register_peer_tbl(struct icc_peer_table *tbl);
int icc_get_core_id(void);
void icc_wakeup_rx_thread(struct icc_peer *peer);
int icc_add_msg_nolock(struct icc_msg_queue *msg_q, unsigned char *buf,
    unsigned int size,
    unsigned int core_id);
int icc_clients_handle_msg(struct icc_core *icc, unsigned char *buf, unsigned int size,
    unsigned int msg_id, unsigned int core_id);

struct icc_client *icc_client_create(struct icc_core *icc, const char *name);
void icc_client_destroy(struct icc_client *client);
int icc_client_check(struct icc_core *icc, struct icc_client *client);
int icc_register_msg(struct icc_client *client, unsigned int msg_id);
int icc_register_msg(struct icc_client *client, unsigned int msg_id);
int icc_receive(struct icc_client *client, struct icc_request *request);
int icc_send(struct icc_client *client, struct icc_request *request);
int icc_set_timeout(struct icc_client *client, unsigned int timeout);
int icc_cancle_msg(struct icc_client *client, unsigned int msg_id);

void wakeup_icc_shm_poll_rx_thread(void);
#endif
