/*
 * SmartChip icc driver, share memory operation code
 */

#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>

#include "icc_priv.h"

static struct icc_core *shm_icc;

static struct semaphore rx_sem = __SEMAPHORE_INITIALIZER(rx_sem, 0); //rx thread semphore

static void icc_shm_cache_flush(off_t offs, size_t size)
{
#if 0
	mutex_lock(&shm_icc->mutex);
	dma_sync_single_range_for_device(shm_icc->dev.this_device,
	    shm_icc->phys_addr,
	    offs,
	    size,
	    DMA_TO_DEVICE);
	mutex_unlock(&shm_icc->mutex);
#endif
}

static void icc_shm_cache_invalid(off_t offs, size_t size)
{
#if 0
	mutex_lock(&shm_icc->mutex);
	dma_sync_single_range_for_cpu(shm_icc->dev.this_device,
	    shm_icc->phys_addr,
	    offs,
	    size,
	    DMA_FROM_DEVICE);
	mutex_unlock(&shm_icc->mutex);
#endif
}

#if 0
static void icc_shm_dump_mgmt(struct icc_shm_mgmt *mgmt)
{
	struct icc_shm_desc_core *desc_core;
	struct icc_shm_desc_blks *desc_blks = &mgmt->blk_desc;
	int i, j;

	icc_debug("init field=%x", mgmt->init_field);
	icc_debug("init lock =%x", mgmt->init_lock);

	for(i = 0; i < ICC_SHM_NR_NODES; i++) {
		desc_core = &mgmt->core_desc[i];
		icc_debug("core desc %d", i);
		icc_debug("buffer cnt=%d, tx=%d, rx=%d\n",
		    desc_core->count, desc_core->tx_idx, desc_core->rx_idx);

		for(j = 0; j < ICC_SHM_BUFF_DESC_Q_SIZE; j++) {
			icc_debug("ringq %02d, buff idx=%04d, msg id=%02d, payload=%08x, src core=%d",
			    j,
			    desc_core->pkt_desc_q[j].buf_idx,
			    desc_core->pkt_desc_q[j].msg_id,
			    desc_core->pkt_desc_q[j].payload,
			    desc_core->pkt_desc_q[j].src_id);
		}
	}

	icc_debug("block size 0x%x", desc_blks->blk_sz);
	icc_debug("block count %d", desc_blks->count);
	icc_debug("block used %d", desc_blks->used);
}
#endif

static unsigned char *icc_idx_to_buf(struct icc_shm_mgmt *mgmt, uint32_t idx)
{
	unsigned char *buf;

	if(idx > mgmt->blk_desc.count)
		return NULL;

	buf = (unsigned char *)mgmt +
	    (mgmt->blk_desc.offset + mgmt->blk_desc.blk_sz * idx);

	return buf;
}

#if 0
static uint32_t icc_buf_to_idx(struct icc_shm_mgmt *mgmt, unsigned char *buf)
{
	uint32_t idx;

	if(buf > mgmt->blk_desc.offset +
	    mgmt->blk_desc.count * mgmt->blk_desc.blk_sz)
		return ICC_SHM_INVALID_IDX;

	idx = (uint32_t)(buf - mgmt->blk_desc.offset) / mgmt->blk_desc.blk_sz;

	return idx;
}
#endif

/* get first zero bit */
static uint32_t icc_get_first_zero_bit(uint32_t value, uint32_t start)
{
	uint32_t idx;
	uint32_t tmp32;

	value |= (1 << start) - 1;

	/* Invert value */
	value = ~value;

	/* (~value) & (2's complement of value) */
	value = (value & (-value)) - 1;

	/* log2(value) */

	tmp32 = value - ((value >> 1) & 033333333333) - ((value >> 2) & 011111111111);

	idx = ((tmp32 + (tmp32 >> 3)) & 030707070707) % 63;

	/* Obtain index (compiler optimized ) */
	//GET_IDX(idx,value);

	return idx;
}

/* get first bit '1' */
static uint32_t icc_get_first_bit(uint32_t value, uint32_t start)
{
	return icc_get_first_zero_bit(~value, start);
}

static void icc_set_bitmap(struct icc_shm_mgmt *mgmt, uint32_t idx, uint32_t nr)
{
	int i;
	uint32_t *word;
	uint32_t bit_msk_idx = idx / 32;
	uint8_t  bit_idx = idx % 32;

	/* First unaligned word */
	word = &mgmt->blk_desc.bit_mask[bit_msk_idx];
	for(i = bit_idx; i < 32; i++) {
		*word |= (1 << i);
		if(--nr == 0)
			return;
	}
	word++;

	while(nr > 32) {
		*word++ = ~0;
		nr -= 32;
	}

	for(i = 0; i < nr; i++)
		*word |= (1 << i);
}

static void icc_clear_bitmap(struct icc_shm_mgmt *mgmt, uint32_t idx, uint32_t nr)
{
	int i;
	uint32_t *word;
	uint32_t bit_msk_idx = idx / 32;
	uint8_t  bit_idx = idx % 32;

	/* First unaligned word */
	word = &mgmt->blk_desc.bit_mask[bit_msk_idx];
	for(i = bit_idx; i < 32; i++) {
		*word ^= (1 << i);
		if(--nr == 0)
			break;
	}
	word++;

	while(nr > 32) {
		*word++ = 0;
		nr -= 32;
	}

	for(i = 0; i < nr; i++)
		*word ^= (1 << i);
}

static int icc_get_bitmap(struct icc_shm_mgmt *mgmt, uint32_t nr, uint32_t *p_idx)
{
	uint32_t start = 0;
	uint32_t bit_start = 0, bit_end = 0;
	int status = -EINVAL;

refind:
	while(start < mgmt->blk_desc.count / 32) {
		bit_start = icc_get_first_zero_bit(mgmt->blk_desc.bit_mask[start], bit_start);
		if(bit_start < 32)
			break;
		bit_start = 0;
		start++;
	}

	bit_end = (bit_start + 1) % 32;
	bit_start += start * 32;
	if(bit_end == 0)
		start++;

	while(start < mgmt->blk_desc.count / 32) {
		bit_end = icc_get_first_bit(mgmt->blk_desc.bit_mask[start], bit_end);
		if(bit_end < 32)
			break;
		if(bit_end + start * 32 - bit_start >= nr)
			break;
		bit_end = 0;
		start++;
	}

	bit_end += start * 32;

	if(bit_end - bit_start < nr) {
		/* reseach from bit end */
		bit_start = (bit_end + 1) % 32;
		start = (bit_end + 1) / 32;
		goto refind;
	} else {
		*p_idx = bit_start;
		status = 0;
	}

	//printf("<%s %d> %d %d %d\n", __func__, __LINE__, start, bit_start, bit_end);
	return status;
}

/* Both methods are valid, you can choose one that you want */
#if 1
unsigned char *icc_shm_get_buffer(struct icc_shm_mgmt *mgmt, uint32_t nr, uint32_t *idx)
{
	unsigned char          *buff = NULL;
	int                    status = 0;

	/* Acquire lock of SM buffer management block */
	icc_shm_acquire_lock(&mgmt->blk_desc.lock);

	icc_debug("used cnt=%d, total cnt=%d\n", mgmt->blk_desc.used, mgmt->blk_desc.count);

	/* Check if obtained buff index is less than the buff count */
	if (mgmt->blk_desc.used + nr <= mgmt->blk_desc.count) {
		/* Obtain the index of the first available SM buffer */
		status = icc_get_bitmap(mgmt, nr, idx);
		if (status == 0) {
			icc_debug("get buffer idx=%d,nr=%d\n", *idx, nr);

			icc_set_bitmap(mgmt, *idx, nr);
			/* Obtain the address of the SM buffer for the index */
			buff = icc_idx_to_buf(mgmt, *idx);
			if(!buff) {
				icc_shm_release_lock(&mgmt->blk_desc.lock);
				return ERR_PTR(-ENOMEM);
			}
			/* increment used buffer count */
			mgmt->blk_desc.used += nr;
		}
	}

	/* Release lock of SM buffer management block */
	icc_shm_release_lock(&mgmt->blk_desc.lock);

	/* Return a MCAPI buffer to the caller */
	return buff;
}

void icc_shm_free_buffer(struct icc_shm_mgmt *mgmt, uint32_t idx, uint32_t nr)
{
	icc_shm_acquire_lock(&mgmt->blk_desc.lock);
	/* Mark the buffer available */
	icc_clear_bitmap(mgmt, idx, nr);
	mgmt->blk_desc.used -= nr;   /* Decrement used buffer count */
	icc_shm_release_lock(&mgmt->blk_desc.lock);
}
#else
unsigned char *icc_shm_get_buffer(struct icc_shm_mgmt *mgmt, uint32_t nr, uint32_t *idx)
{
	unsigned char          *buff = NULL;
	int                    status = 0;
	unsigned long          bitmap_no;

	/* Acquire lock of SM buffer management block */
	icc_shm_acquire_lock(&mgmt->blk_desc.lock);

	bitmap_no = bitmap_find_next_zero_area_off((unsigned long *)mgmt->blk_desc.bit_mask,
	        mgmt->blk_desc.count, 0, nr, 0, 0);

	bitmap_set((unsigned long *)mgmt->blk_desc.bit_mask, bitmap_no, nr);

	/* increment used buffer count */
	mgmt->blk_desc.used += nr;

	/* Release lock of SM buffer management block */
	icc_shm_release_lock(&mgmt->blk_desc.lock);

	/* Obtain the address of the SM buffer for the index */
	buff = icc_idx_to_buf(mgmt, bitmap_no);
	if(!buff)
		return ERR_PTR(-ENOMEM);

	*idx = bitmap_no;

	printk("[xiaohui] %s %d %d %d",  __func__, __LINE__, bitmap_no, nr);

	/* Return a MCAPI buffer to the caller */
	return buff;
}

void icc_shm_free_buffer(struct icc_shm_mgmt *mgmt, uint32_t idx, uint32_t nr)
{
	icc_shm_acquire_lock(&mgmt->blk_desc.lock);

	/* Mark the buffer available */
	bitmap_clear((unsigned long *)mgmt->blk_desc.bit_mask, idx, nr);

	mgmt->blk_desc.used -= nr;   /* Decrement used buffer count */

	icc_shm_release_lock(&mgmt->blk_desc.lock);
}

#endif

static struct icc_shm_desc_core *icc_get_sm_ring_q(struct icc_shm_mgmt *mgmt,
    uint32_t core_id)
{
	struct icc_shm_desc_core *p_sm_ring_queue = NULL;

	if (core_id < ICC_SHM_NR_NODES) {
		p_sm_ring_queue = &mgmt->core_desc[core_id];
	}

	/* Return pointer to SM ring queue for the unit ID identified */
	return p_sm_ring_queue;
}

static int icc_enqueue_sm_ring_q(struct icc_shm_desc_core *shm_des_q,
    uint32_t buf_idx, uint32_t buf_nr,
    uint32_t msgid, uint32_t priority,
    size_t buff_size)
{
	uint32_t tx_idx;
	int status = 0;
	struct icc_shm_buff_desc *shm_desc;

	/* Acquire lock of the SM packet descriptor queue */
	icc_shm_acquire_lock(&shm_des_q->lock);

	/* Obtain put index into the queue */
	tx_idx = shm_des_q->tx_idx;

	if (shm_des_q->count == ICC_SHM_BUFF_DESC_Q_SIZE) {
		/* Queue is full fail denqueue operation, Try later */
		status = -EAGAIN;
	} else {
		/* Load packet descriptor */
		shm_desc = &shm_des_q->pkt_desc_q[tx_idx];
		shm_desc->priority = priority;
		shm_desc->buf_idx  = buf_idx;
		shm_desc->buf_nr   = buf_nr;
		shm_desc->src_id   = icc_get_core_id();
		shm_desc->payload  = buff_size;
		shm_desc->msg_id   = msgid;

		shm_des_q->tx_idx  = (shm_des_q->tx_idx + 1) % ICC_SHM_BUFF_DESC_Q_SIZE;
		shm_des_q->count++;

		/* Enqueue operation successfully completed */
		status = 0;
	}

	/* Release lock of the SM packet descriptor queue */
	icc_shm_release_lock(&shm_des_q->lock);

	return status;
}

int icc_shm_tx(struct icc_shm_mgmt *mgmt, int target_coreid, int buf_idx, int buf_nr,
    size_t buffer_size, unsigned int msgid,
    unsigned int coreid, unsigned int priority)
{
	struct icc_shm_desc_core *shm_q;
	int                      status = 0;

	/* Invalid mgmt blk */
	icc_shm_cache_invalid(0, mgmt->blk_desc.offset);

	/* Obtain SM ring queue for the destination core ID */
	shm_q = icc_get_sm_ring_q(mgmt, target_coreid);
	if (shm_q) {
		/* Enqueue request to transmit data */
		status = icc_enqueue_sm_ring_q(shm_q, buf_idx, buf_nr, msgid, priority, buffer_size);

		icc_shm_cache_flush(0, mgmt->blk_desc.offset);
		icc_shm_cache_flush(mgmt->blk_desc.offset + buf_idx * PAGE_SIZE,
		    buf_nr * PAGE_SIZE);
	} else {
		/* TX request to unrecognized node ID */
		status = -EINVAL;
	}

	//  icc_shm_dump_mgmt(mgmt);

	/* Failed to queue buffer, free buffer */
	if(status) {
		icc_shm_free_buffer(mgmt, buf_idx, buf_nr);
		icc_shm_cache_flush(0, mgmt->blk_desc.offset);
	}
	return status;
}

/* Return the first pending descriptor, or NULL. */
static struct icc_shm_buff_desc *icc_shm_desc_get_next(struct icc_shm_desc_core *shm_des_q)
{
	if (shm_des_q->count)
		return &shm_des_q->pkt_desc_q[shm_des_q->rx_idx];

	return NULL;
}

/* Make the first pending descriptor available to producers again. */
static void icc_shm_desc_consume(struct icc_shm_desc_core *shm_des_q)
{
	icc_shm_acquire_lock(&shm_des_q->lock);

	/* Update index and count */
	shm_des_q->rx_idx = (shm_des_q->rx_idx + 1) % ICC_SHM_BUFF_DESC_Q_SIZE;
	shm_des_q->count--;

	icc_shm_release_lock(&shm_des_q->lock);
}

void wakeup_icc_shm_poll_rx_thread(void)
{
	up(&rx_sem);
}

static int icc_shm_poll_rx_thread(void *arg)
{
	struct icc_core *icc = arg;
	struct icc_shm_mgmt *mgmt = icc->shm.shm_mgmt;
	struct icc_shm_desc_core *shm_des_q;
	struct icc_shm_buff_desc *shm_des;
	unsigned char *buf;
	int ret;

	struct sched_param param = { .sched_priority = 1 };
	sched_setscheduler(current, SCHED_FIFO, &param);

	shm_des_q = &mgmt->core_desc[icc_get_core_id()];
	while(1) {
		/* Sync cache of mgmt blk */
		icc_shm_cache_invalid(0, mgmt->blk_desc.offset);

		icc_shm_acquire_lock(&shm_des_q->lock);
		shm_des = icc_shm_desc_get_next(shm_des_q);
		icc_shm_release_lock(&shm_des_q->lock);

		/* No pending rx buffer, Sleep */
		if(!shm_des) {
#if 0
			//会丢中断
			set_current_state(TASK_UNINTERRUPTIBLE);
			if (signal_pending(current)) {
			    break;
			}
			schedule_timeout(HZ * 20);
			__set_current_state(TASK_RUNNING);
#endif
			ret = down_timeout(&rx_sem, HZ * 20);
			continue;
		}

		buf = icc_idx_to_buf(mgmt, shm_des->buf_idx);
		if(!buf) {
			BUG();
		}

		/* Sync cache of data blk */
		icc_shm_cache_invalid(mgmt->blk_desc.offset + shm_des->buf_idx * PAGE_SIZE,
		    shm_des->buf_nr * PAGE_SIZE);

		icc_debug("receive msg, idx=%d, nr=%d, msg id=%d, core id=%d, size=%x\n",
		    shm_des->buf_idx, shm_des->buf_nr,
		    shm_des->msg_id, shm_des->src_id,
		    shm_des->payload);

		ret = icc_clients_handle_msg(icc, buf, shm_des->payload,
		        shm_des->msg_id, shm_des->src_id);
		if(ret < 0) {
			BUG();
		}

		icc_shm_desc_consume(shm_des_q);
		icc_shm_free_buffer(mgmt, shm_des->buf_idx, shm_des->buf_nr);

		/* Sync cache of mgmt blk */
		icc_shm_cache_flush(0, mgmt->blk_desc.offset);
	}

	return 0;
}

static int icc_shm_master_node_init(struct icc_shm_mgmt *mgmt)
{
	int i, buf_blk_count = mgmt->blk_desc.count;

	/*
	 * The current node is the first node executing in the system.
	 * Initialize SM driver as master node.
	 */

	icc_shm_lock_init(&mgmt->blk_desc.lock);

	for (i = 0; i < ICC_CORE_TOTAL; i++) {
		icc_shm_lock_init(&mgmt->core_desc[i].lock);
	}

	/* Make all SM buffers available */
	for (i = 0; i < (buf_blk_count + 31) / 32; i++) {
		mgmt->blk_desc.bit_mask[i] = 0;
	}

	/* Initialize used buff count */
	mgmt->blk_desc.used = 0;
	/* Load shared memory initialization complete key */
	mgmt->init_field = SHM_INIT_COMPLETE_KEY;

	/* Return master node initialization status */
	return 0;
}

static int icc_shm_slave_node_init(struct icc_shm_mgmt *mgmt)
{
	/* SM driver has already been initialized by the master node */
	/* Perform slave initialization of SM driver */

	/* Make sure the current node has not been initialized */

	/* Return slave node initialization status */
	return 0;
}

static size_t icc_calc_mgmt_size(size_t size)
{
	size_t mgmt_size = 0;

	/* Smallest mgmt size */
	mgmt_size = ALIGN(sizeof(struct icc_shm_mgmt), ICC_SHM_BUF_LEN - 1);
	if(mgmt_size >= size)
		return 0;

	while(mgmt_size < size) {
		int bit_cnt = (mgmt_size - sizeof(struct icc_shm_mgmt)) * 8;
		int buf_cnt = (size - mgmt_size) / ICC_SHM_BUF_LEN;

		if(buf_cnt == 0)
			return 0;

		if(bit_cnt >= buf_cnt)
			break;

		mgmt_size += ICC_SHM_BUF_LEN;
	}

	return mgmt_size;
}

static int ion_debug_shm_show(struct seq_file *s, void *unused)
{
	struct icc_shm_mgmt *shm_mgmt = s->private;
	int i, j, rx;

	icc_shm_acquire_lock(&shm_mgmt->blk_desc.lock);
	seq_printf(s, "share memory block size=%d,total block=%d used=%d\n",
	    shm_mgmt->blk_desc.blk_sz,
	    shm_mgmt->blk_desc.count,
	    shm_mgmt->blk_desc.used);
	icc_shm_release_lock(&shm_mgmt->blk_desc.lock);

	for(i = 0; i < ICC_SHM_NR_NODES; i++) {
		icc_shm_acquire_lock(&shm_mgmt->core_desc[i].lock);

		seq_printf(s, "  shm core %02d,ring-q total=%03d used=%03d,tx=%03d,rx=%03d\n",
		    i,
		    ICC_SHM_BUFF_DESC_Q_SIZE,
		    shm_mgmt->core_desc[i].count,
		    shm_mgmt->core_desc[i].tx_idx,
		    shm_mgmt->core_desc[i].rx_idx);

		rx = shm_mgmt->core_desc[i].rx_idx;

		for(j = 0; j < shm_mgmt->core_desc[i].count; j++) {
			seq_printf(s, "    pending msg: rx %02d,msg id=0x%08x,blk idx=0x%06x nr=0x%06x,payload=0x%06x,src core=%d\n",
			    rx,
			    shm_mgmt->core_desc[i].pkt_desc_q[rx].msg_id,
			    shm_mgmt->core_desc[i].pkt_desc_q[rx].buf_idx,
			    shm_mgmt->core_desc[i].pkt_desc_q[rx].buf_nr,
			    shm_mgmt->core_desc[i].pkt_desc_q[rx].payload,
			    shm_mgmt->core_desc[i].pkt_desc_q[rx].src_id);
			rx = (rx + 1) % ICC_SHM_BUFF_DESC_Q_SIZE;
		}
		icc_shm_release_lock(&shm_mgmt->core_desc[i].lock);
	}
	return 0;
}

static int ion_debug_shm_open(struct inode *inode, struct file *file)
{
	return single_open(file, ion_debug_shm_show, inode->i_private);
}

static const struct file_operations debug_shm_fops = {
	.open = ion_debug_shm_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int icc_shm_init(struct icc_core *icc)
{
	int ret = 0, buf_blks;
	size_t mgmt_size, shm_size = icc->mem_size;
	/* Obtain Shared memory base address */
	struct icc_shm_mgmt *mgmt = icc->virt_addr;

	shm_icc = icc;

	memset_io(icc->virt_addr, 0, shm_size);

	mgmt_size = icc_calc_mgmt_size(shm_size);
	if(!mgmt_size) {
		return -EINVAL;
	}

	buf_blks = (shm_size - mgmt_size) / ICC_SHM_BUF_LEN;

	mgmt->blk_desc.used  = 0;
	mgmt->blk_desc.count = buf_blks;
	/* Obtain the offset of the SM buffer space */
	mgmt->blk_desc.offset = mgmt_size;
	mgmt->blk_desc.blk_sz = ICC_SHM_BUF_LEN;

	icc_debug("shm size=%x, mgmt size=%x, shm buf size=%lx\n",
	    shm_size, mgmt_size, ICC_SHM_BUF_LEN);
	icc_debug("buffer block cnt=%d\n", buf_blks);

	if (mgmt != NULL) {
		/* Has another node completed SM driver initialization */
		if (mgmt->init_field != SHM_INIT_COMPLETE_KEY) {
			mgmt->init_field = SHM_INIT_COMPLETE_KEY;

			icc_shm_lock_init(&mgmt->init_lock);

			/* Obtain SM driver initialization lock */
			icc_shm_acquire_lock(&mgmt->init_lock);

			/* Initialize SM driver as the Master node */
			ret = icc_shm_master_node_init(mgmt);

			/* Release SM driver initialization lock */
			icc_shm_release_lock(&mgmt->init_lock);
		} else {
			/* Obtain SM driver initialization lock */
			icc_shm_acquire_lock(&mgmt->init_lock);

			/* Initialize SM driver as the Slave node */
			ret = icc_shm_slave_node_init(mgmt);

			/* Release SM driver initialization lock */
			icc_shm_release_lock(&mgmt->init_lock);
		}
	} else {
		ret = -EINVAL;
	}

	icc->shm.shm_mgmt = mgmt;
	icc->shm.shm_buff = (unsigned char *)icc->virt_addr + mgmt_size;

	icc->rcv_tsk = kthread_run(icc_shm_poll_rx_thread, icc, "icc-rx");
	if(!icc->rcv_tsk) {
		return -EINVAL;
	}

	icc->shm.dbg_file = debugfs_create_file("shm", 0644,
	        icc->dbg_dir, icc->shm.shm_mgmt,
	        &debug_shm_fops);

	return ret;
}

int icc_shm_exit(struct icc_core *icc)
{
	struct icc_shm_mgmt *mgmt = icc->shm.shm_mgmt;
	struct icc_shm_desc_core *shm_des_q =
	    &mgmt->core_desc[icc_get_core_id()];

	kthread_stop(icc->rcv_tsk);

	icc_shm_acquire_lock(&shm_des_q->lock);
	shm_des_q->count = 0;
	shm_des_q->rx_idx = 0;
	shm_des_q->tx_idx = 0;
	icc_shm_release_lock(&shm_des_q->lock);

	return 0;
}
