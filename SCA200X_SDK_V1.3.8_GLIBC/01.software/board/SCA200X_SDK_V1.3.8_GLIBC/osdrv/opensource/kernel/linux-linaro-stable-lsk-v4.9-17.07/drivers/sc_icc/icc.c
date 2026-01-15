/*
 * Icc core driver code
 */
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/gfp.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/mm_types.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>

#include "icc_priv.h"
#include "icc.h"

union icc_ioctl_arg {
	struct icc_ioctl_io_request request;
	struct icc_ioctl_reg_msg    reg_msg;
	struct icc_ioctl_timeout    timeout;
};

union icc_ioctl_arg_32 {
	struct icc_ioctl_io_request_32 request;
	struct icc_ioctl_reg_msg       reg_msg;
	struct icc_ioctl_timeout       timeout;
};

static LIST_HEAD(icc_peer_table_list);

int icc_get_core_id(void)
{
	return ICC_CORE_A_0;
}

void icc_register_peer_tbl(struct icc_peer_table *tbl)
{
	list_add(&tbl->list, &icc_peer_table_list);
}

int icc_vmap_mem(struct icc_core *icc)
{
	struct page **pages;
	int i, num;
	phys_addr_t phys_addr = icc->phys_addr;
	void *vaddr;

	num = icc->mem_size >> PAGE_SHIFT;

	pages = kzalloc(sizeof(struct page *) * num, GFP_KERNEL);
	if(IS_ERR(pages)) {
		return PTR_ERR(pages);
	}

	for(i = 0; i < num; i++) {
		pages[i] = phys_to_page(phys_addr);
		phys_addr += PAGE_SIZE;
	}

	//vaddr = vmap(pages, num, VM_MAP,PAGE_KERNEL/*pgprot_writecombine(PAGE_KERNEL)*/);
	vaddr = vmap(pages, num, VM_MAP, pgprot_noncached(PAGE_KERNEL));
	if (!vaddr) {
		kfree(pages);
		return -ENOMEM;
	}

	icc_debug("icc shm vaddr %p\n", vaddr);

	icc->virt_addr = vaddr;

	kfree(pages);
	return 0;
}

int icc_register_msg(struct icc_client *client, unsigned int msg_id)
{
	int ret = 0, i, pos = -1;
	struct icc_msg_queue *msg_q;

	mutex_lock(&client->mutex);
	for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
		msg_q = &client->msg_rx_queue[i];

		/* Find first unusued msg queue */
		if(msg_q->msg_id == ICC_INVALID_MSG_ID && pos == -1)
			pos = i;

		/* Msg had been regisetered already */
		if(msg_q->msg_id == msg_id) {
			goto unlock_client;
		}
	}
	/* No available msg queue */
	if(pos == -1) {
		ret = -ENOMEM;
		goto unlock_client;
	}

	msg_q = &client->msg_rx_queue[pos];
	msg_q->msg_id     = msg_id;
	msg_q->rx_request = NULL;
	msg_q->msg_num    = 0;
	INIT_LIST_HEAD(&msg_q->msg_list);

unlock_client:
	mutex_unlock(&client->mutex);
	return ret;
}

int icc_cancle_msg(struct icc_client *client, unsigned int msg_id)
{
	int i;
	struct icc_msg_queue *msg_q;
	struct icc_msg *msg, *t_msg;

	mutex_lock(&client->mutex);
	for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
		msg_q = &client->msg_rx_queue[i];

		if(msg_q->msg_id != msg_id) {
			continue;
		}
		if(!list_empty(&msg_q->msg_list)) {
			list_for_each_entry_safe(msg, t_msg, &msg_q->msg_list, list) {
				list_del(&msg->list);
				kfree(msg->kbuf);
				kfree(msg);
			}
		}
		msg_q->msg_id     = ICC_INVALID_MSG_ID;
		msg_q->rx_request = NULL;
		msg_q->msg_num    = 0;
		break;
	}
	mutex_unlock(&client->mutex);
	return 0;
}

int icc_set_timeout(struct icc_client *client, unsigned int timeout)
{
	if(timeout)
		client->timeout = timeout;
	return 0;
}

int icc_add_msg_nolock(struct icc_msg_queue *msg_q, unsigned char *buf,
    unsigned int size,
    unsigned int core_id)
{
	struct icc_msg *msg;

	msg = kmalloc(sizeof(*msg), GFP_KERNEL);
	if(!msg)
		return -ENOMEM;

	msg->kbuf = kmalloc(size, GFP_KERNEL);
	if(!msg->kbuf) {
		kfree(msg);
		return -ENOMEM;
	}

	memcpy(msg->kbuf, buf, size);
	msg->core_id = core_id;
	msg->size    = size;

	msg_q->msg_num++;
	WARN(msg_q->msg_num > ICC_CLIENT_MSG_TRHESHOLD,
	    "client %s, msg id %x, to many pendding msg %x\n",
	    " ",
	    msg_q->msg_id,
	    msg_q->msg_num);

	list_add_tail(&msg->list, &msg_q->msg_list);
	return 0;
}

int icc_peer_create(struct icc_core *icc, struct device_node *node)
{
	struct icc_peer *peer;
	struct icc_peer_table *tbl;
	int found = 0, ret, core_id;
	unsigned char *dname, name[32] = {0};

	ret = of_property_read_string(node, "type", (const char **)&dname);
	if(ret)
		return ret;

	list_for_each_entry(tbl, &icc_peer_table_list, list) {
		if(strcmp(tbl->name, dname) == 0) {
			core_id = tbl->get_coreid(node);
			if(core_id < 0)
				return -EINVAL;

			sprintf(name, "peer-%s-%d", dname, core_id);

			peer = &icc->peer[core_id];
			peer->valid   = 1;
			peer->core_id = core_id;
			peer->icc     = icc;
			peer->name    = kstrdup(name, GFP_KERNEL);
			mutex_init(&peer->mutex);

			ret = tbl->create(peer, node);
			if(ret)
				return -EINVAL;

			peer->peer_tbl = tbl;
			found = 1;
			break;
		}
	}

	if(!found)
		return -EINVAL;

	return 0;
}

static void icc_peer_destroy(struct icc_peer *peer)
{
	int ret;
	struct icc_peer_table *peer_tbl = peer->peer_tbl;

	if(peer_tbl->destroy) {
		ret = peer_tbl->destroy(peer);
		if(ret < 0)
			return;
	}

	if(peer->priv)
		kfree(peer->priv);

	debugfs_remove(peer->dbg_file);

	memset(peer, 0, sizeof(*peer));
}

static int icc_debug_client_show(struct seq_file *s, void *unused)
{
	struct icc_client *client = s->private;
	struct icc_msg_queue *msg_q;
	struct icc_request *request;
	struct icc_msg *msg;
	int i;
#ifdef ICC_RECORD_MSG
	int pos;
#endif

	mutex_lock(&client->mutex);
	seq_printf(s, "client %s\n", client->name);
	for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
		msg_q = &client->msg_rx_queue[i];

		if(client->msg_rx_queue[i].msg_id != ICC_INVALID_MSG_ID) {
			seq_printf(s, "   registered msg id 0x%08x:\n", msg_q->msg_id);

			request = msg_q->rx_request;
			if(request) {
				seq_printf(s, "      task %s is waiting for msg\n",
				    request->task->group_leader->comm);
			} else
				seq_printf(s, "      no pendding request\n");

			if(!list_empty(&msg_q->msg_list)) {
				list_for_each_entry(msg, &msg_q->msg_list, list) {
					seq_printf(s, "      pendding msg: payload=0x%06x, from core %02d\n",
					    msg->size, msg->core_id);
				}
			} else
				seq_printf(s, "      no pending msg\n");
		}
	}

#ifdef ICC_RECORD_MSG
	pos = (client->record_tx_pos + ICC_RECORD_MSG_NUM - 1) % ICC_RECORD_MSG_NUM;
	seq_printf(s, "   History tx\n");
	while(pos != client->record_tx_pos) {
		if(!client->last_tx[pos].task)
			break;
		seq_printf(s, "      task %s, msg id 0x%08x, size 0x%06lx, core %d\n",
		    client->last_tx[pos].task->comm,
		    client->last_tx[pos].msg_id,
		    client->last_tx[pos].size,
		    client->last_tx[pos].core_id);
		pos = (pos + ICC_RECORD_MSG_NUM - 1) % ICC_RECORD_MSG_NUM;
	}

	pos = (client->record_rx_pos + ICC_RECORD_MSG_NUM - 1) % ICC_RECORD_MSG_NUM;
	seq_printf(s, "   History rx\n");
	while(pos != client->record_rx_pos) {
		if(!client->last_rx[pos].task)
			break;
		seq_printf(s, "      task %s, msg id 0x%08x, size 0x%06lx, core %d\n",
		    client->last_tx[pos].task->comm,
		    client->last_rx[pos].msg_id,
		    client->last_rx[pos].size,
		    client->last_rx[pos].core_id);
		pos = (pos + ICC_RECORD_MSG_NUM - 1) % ICC_RECORD_MSG_NUM;
	}
#endif
	seq_printf(s, "\n");

	mutex_unlock(&client->mutex);
	return 0;
}

static int icc_debug_client_open(struct inode *inode, struct file *file)
{
	return single_open(file, icc_debug_client_show, inode->i_private);
}

static const struct file_operations debug_client_fops = {
	.open = icc_debug_client_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int icc_client_check(struct icc_core *icc, struct icc_client *client)
{
	struct icc_client *client_t;
	int found = 0;

	mutex_lock(&icc->mutex);
	list_for_each_entry(client_t, &icc->client_list, list) {
		if(client_t == client) {
			found = 1;
			break;
		}
	}
	mutex_unlock(&icc->mutex);
	return found;
}

struct icc_client *icc_client_create(struct icc_core *icc,
    const char *name)
{
	struct icc_client *client;
	struct task_struct *task;
	struct icc_msg_queue *msg_q;
	pid_t pid, id;
	int i;
	unsigned char t_name[32] = {0};

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	get_task_struct(current->group_leader);
	task_lock(current->group_leader);
	pid = task_pid_nr(current->group_leader);
	/*
	 * don't bother to store task struct for kernel threads,
	 * they can't be killed anyway
	 */
	if (current->group_leader->flags & PF_KTHREAD) {
		put_task_struct(current->group_leader);
		task = NULL;
	} else {
		task = current->group_leader;
	}

	id = idr_alloc(&icc->idr, client, 1, 0, GFP_KERNEL);

	sprintf(t_name, "%s-%d", name, id);

	client->id   = id;
	client->name = kstrdup(t_name, GFP_KERNEL);
	client->tsk  = task;
	client->icc  = icc;
	client->timeout = ICC_DEFAULT_TIMEOUT;
#ifdef ICC_RECORD_MSG
	for(i = 0; i < ICC_RECORD_MSG_NUM; i++) {
		client->last_tx[i].msg_id  = -1;
		client->last_tx[i].size    = -1;
		client->last_tx[i].core_id = -1;
		client->last_tx[i].task    = NULL;

		client->last_rx[i].msg_id  = -1;
		client->last_rx[i].size    = -1;
		client->last_rx[i].core_id = -1;
		client->last_rx[i].task    = NULL;
	}
	client->record_rx_pos = 0;
	client->record_tx_pos = 0;
#endif

	task_unlock(current->group_leader);

	mutex_init(&client->mutex);

	/* Init msg queue's mutex */
	for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
		msg_q = &client->msg_rx_queue[i];
		msg_q->msg_id = ICC_INVALID_MSG_ID;
		INIT_LIST_HEAD(&msg_q->msg_list);
	}

	mutex_lock(&icc->mutex);
	list_add(&client->list, &icc->client_list);
	mutex_unlock(&icc->mutex);

	client->dbg_file = debugfs_create_file(client->name, 0644,
	        icc->clients_dbg_dir, client,
	        &debug_client_fops);

	return client;
}

void icc_client_destroy(struct icc_client *client)
{
	int i;
	struct icc_core *icc = client->icc;
	struct icc_request *request;
	struct icc_msg *msg, *t_msg;

	mutex_lock(&client->mutex);
	/* Delete msg list */
	for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
		struct icc_msg_queue *msg_q = &client->msg_rx_queue[i];

		/* Unused msg queue */
		if(msg_q->msg_id == ICC_INVALID_MSG_ID)
			continue;

		/* Cancle pending request */
		request = msg_q->rx_request;
		if(request) {
			request->res = -EINVAL;
			request->size = 0;
			complete(&request->done);
		}

		/* No pending msg */
		if(list_empty(&msg_q->msg_list))
			continue;

		list_for_each_entry_safe(msg, t_msg, &msg_q->msg_list, list) {
			list_del(&msg->list);
			if(msg->kbuf)
				kfree(msg->kbuf);
			kfree(msg);
		}
	}
	mutex_unlock(&client->mutex);

	debugfs_remove(client->dbg_file);

	idr_remove(&icc->idr, client->id);

	mutex_lock(&icc->mutex);
	list_del(&client->list);
	mutex_unlock(&icc->mutex);
	kfree(client->name);
	kfree(client);
}

int icc_send(struct icc_client *client, struct icc_request *req)
{
	struct icc_core *icc = client->icc;
	struct icc_peer *peer;
	unsigned char *buf;
	int ret;
	uint32_t idx, nr;

	peer = &icc->peer[req->core_id];

	if(!peer->valid) {
		icc_error("peer not ready\n");
		return -EINVAL;
	}

	if(!peer->peer_ops->notify) {
		icc_error("peer has no notification interface\n");
		return -EINVAL;
	}

	if(!(req->flag & ICC_REQ_FLAG_UBUF) && !req->kbuf) {
		return -EINVAL;
	}

	if(!(req->flag & ICC_REQ_FLAG_TX)) {
		return -EINVAL;
	}

	nr = (req->size + ICC_SHM_BUF_LEN - 1) / ICC_SHM_BUF_LEN;

	buf = icc_shm_get_buffer(icc->shm.shm_mgmt, nr, &idx);
	if(!buf) {
		icc_error("no available buffer\n");
		return -EAGAIN;
	}

	if(req->flag & ICC_REQ_FLAG_UBUF) {
		if (copy_from_user(buf, req->ubuf, req->size))
			return -EFAULT;
	} else {
		memcpy(buf, req->kbuf, req->size);
	}

	ret = icc_shm_tx(icc->shm.shm_mgmt, req->core_id, idx, nr,
	        req->size, req->msg_id, icc_get_core_id(), 0);
	if(ret < 0) {
		return ret;
	}

	/* Notify peer core */
	ret = peer->peer_ops->notify(peer);
	if(ret < 0)
		return ret;

#ifdef ICC_RECORD_MSG
	client->last_tx[client->record_tx_pos].task    = current;
	client->last_tx[client->record_tx_pos].msg_id  = req->msg_id;
	client->last_tx[client->record_tx_pos].size    = req->size;
	client->last_tx[client->record_tx_pos].core_id = req->core_id;
	client->record_tx_pos = (client->record_tx_pos + 1) % ICC_RECORD_MSG_NUM;
#endif

	icc_debug("send msg 0x%08x, size 0x%x, dst core %d, task %s\n",
	    req->msg_id, req->size, req->core_id, req->task->group_leader->comm);

	return 0;
}

int icc_receive(struct icc_client *client, struct icc_request *request)
{
	struct icc_msg *msg;
	int i, ret;
	struct icc_msg_queue *msg_q;

	if((request->flag & ICC_REQ_FLAG_TX)) {
		return -EINVAL;
	}

	mutex_lock(&client->mutex);
	/* Check msg list, if has pending message, get this message */
	for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
		msg_q = &client->msg_rx_queue[i];

		if(msg_q->msg_id != request->msg_id)
			continue;

		/* No pending mesage */
		if(list_empty(&msg_q->msg_list)) {
			break;
		}

		msg = list_first_entry(&msg_q->msg_list, struct icc_msg, list);

		if(request->size < msg->size) {
			icc_error("buffer size smaller than msg size 0x%lx, 0x%x, msg_id = 0x%x\n",
			    request->size, msg->size, request->msg_id);
			mutex_unlock(&client->mutex);
			return -EOVERFLOW;
		}

		list_del(&msg->list);

		msg_q->msg_num--;

		request->kbuf    = msg->kbuf;
		request->core_id = msg->core_id;
		request->size    = msg->size;
		request->res     = 0;
		/* Do not free msg->kbuf */
		kfree(msg);
		mutex_unlock(&client->mutex);
		return 0;
	}

	/* Msg id not registered */
	if(i == ARRAY_SIZE(client->msg_rx_queue)) {
		mutex_unlock(&client->mutex);
		return -EINVAL;
	}

	/* If another thread is waiting for this msg id, panic */
	if(msg_q->rx_request) {
		icc_error("Another thread: (%s, %s) (%d %d)is pending for msgid 0x%x on the client\n"\
		    "current thread (%s,%s), (%d,%d)\n",
		    msg_q->rx_request->task->group_leader->comm, msg_q->rx_request->task->comm,
		    msg_q->rx_request->task->pid, msg_q->rx_request->task->tgid,
		    request->msg_id,
		    current->group_leader->comm, current->comm,
		    current->pid, current->tgid
		);
		BUG();
	}

	/* No pending msg, Wait for msg */
	init_completion(&request->done);

	msg_q->rx_request = request;
	msg_q->rx_request->kbuf = NULL;
	msg_q->rx_request->res = ENOMSG;
	mutex_unlock(&client->mutex);

	/* Sleep and wait for timeout*/
	ret = wait_for_completion_killable_timeout(&request->done,
	        msecs_to_jiffies(client->timeout));

	mutex_lock(&client->mutex);

	/*
	 * Fix these race condition:
	 * 1) icc irq come before wait_for_completion, icc msg copy to the rx_request.
	 * 2) after wait_for_completion, signal received, at this time msg_q->rx_request is not NULL, icc msg copy to the rx_request
	 *    ret < 0, but the msg already in request.
	 * so do not need to check ret, check the data available condition from request->res == 0 is enough.
	*/
	if (ret > 0 || request->res == 0) {
		if (request->res == 0 && request->kbuf == NULL)
			BUG();
		msg_q->rx_request = NULL;
		mutex_unlock(&client->mutex);

#ifdef ICC_RECORD_MSG
		client->last_rx[client->record_rx_pos].task    = current;
		client->last_rx[client->record_rx_pos].msg_id  = request->msg_id;
		client->last_rx[client->record_rx_pos].size    = request->size;
		client->last_rx[client->record_rx_pos].core_id = request->core_id;
		client->record_rx_pos = (client->record_rx_pos + 1) % ICC_RECORD_MSG_NUM;
#endif
		/* complete normally, return request->res, which is the return value */
		return request->res;

	} else {
		/*
		 * for wait timeout or interrupt, and no msg
		*/
		msg_q->rx_request = NULL;

		/* interrupted */
		if(ret < 0) {
			icc_debug("interrupted 0x%x\n", request->msg_id);
			mutex_unlock(&client->mutex);
			return -EINVAL;
		}
		/* time out */
		if(ret == 0) {
			icc_debug("time out 0x%x\n", request->msg_id);
			mutex_unlock(&client->mutex);
			return -ETIME;
		}
	}

	return 0;
}

int icc_clients_handle_msg(struct icc_core *icc, unsigned char *buf, unsigned int size,
    unsigned int msg_id, unsigned int core_id)
{
	struct icc_client *client;
	struct icc_msg_queue *msg_q;
	struct icc_request *request;
	int i, ret;

	mutex_lock(&icc->mutex);
	/* Check each client */
	list_for_each_entry(client, &icc->client_list, list) {
		mutex_lock(&client->mutex);
		for(i = 0; i < ARRAY_SIZE(client->msg_rx_queue); i++) {
			msg_q = &client->msg_rx_queue[i];

			if(msg_q->msg_id != msg_id)
				continue;

			/* Find registered msgid */
			request = msg_q->rx_request;
			/* Client is waiting for msg id */
			if(request) {
				/*
				 * If request buffer size is smaller than message size,
				 * add message to msg list and return error code to request thread
				 *
				 * Then the request thread can receive message with a bigger buffer next time
				 */
				if(request->size < size) {
					ret = icc_add_msg_nolock(msg_q, buf, size, core_id);
					if(ret < 0) {
						mutex_unlock(&client->mutex);
						mutex_unlock(&icc->mutex);
						return ret;
					}

					request->size = 0;
					request->res  = -EOVERFLOW;
				} else {
					request->kbuf = kmalloc(size, GFP_KERNEL);
					if(!request->kbuf) {
						BUG();
					}
					memcpy(request->kbuf, buf, size);
					request->size    = size;
					request->core_id = core_id;
					request->res     = 0;
				}

				/* This request is finished, remove it */
				msg_q->rx_request = NULL;
				complete(&request->done);
			}
			/* Client is not waiting for msg id */
			else {
				ret = icc_add_msg_nolock(msg_q, buf, size, core_id);
				if(ret < 0) {
					mutex_unlock(&client->mutex);
					mutex_unlock(&icc->mutex);
					return ret;
				}
			}
			break;
		}
		mutex_unlock(&client->mutex);
	}
	mutex_unlock(&icc->mutex);
	return 0;
}

void icc_wakeup_rx_thread(struct icc_peer *peer)
{
	//struct icc_core *icc = peer->icc;

	/* Wake up receive thread */
	//spin_lock(&icc->lock);
	//wake_up_process(icc->rcv_tsk);
	//spin_unlock(&icc->lock);

	wakeup_icc_shm_poll_rx_thread();
}

static int icc_open(struct inode *inode, struct file *file)
{
	struct miscdevice *miscdev = file->private_data;
	struct icc_core *icc = container_of(miscdev, struct icc_core, dev);
	struct icc_client *client;
	char debug_name[64];

	snprintf(debug_name, 64, "%s-%u-%u",
	    current->group_leader->comm,
	    task_pid_nr(current->group_leader),
		task_pid_nr(current));

	client = icc_client_create(icc, debug_name);
	if (IS_ERR(client))
		return PTR_ERR(client);

	file->private_data = client;

	return 0;
}

static int icc_release(struct inode *inode, struct file *file)
{
	struct icc_client *client = file->private_data;

	icc_client_destroy(client);
	return 0;
}

static long icc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct icc_client *client = file->private_data;
	int ret;
	struct icc_request request;
	union icc_ioctl_arg data;
	unsigned long res;

	if (_IOC_SIZE(cmd) > sizeof(data))
		return -EINVAL;

	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd)))
		return -EFAULT;

	memset(&request, 0, sizeof(request));

	switch (cmd) {
	case ICC_IOC_SEND:
		request.client    = client;
		request.task      = current;
		request.msg_id    = data.request.msg_id;
		request.core_id   = data.request.target_core;
		request.size      = data.request.size;
		request.ubuf      = (unsigned char __user *)data.request.buf;
		request.flag      |= ICC_REQ_FLAG_UBUF;
		request.flag      |= ICC_REQ_FLAG_TX;

		ret = icc_send(client, &request);
		if(ret) {
			return ret;
		}
		break;
	case ICC_IOC_RECEIVE:
		request.client    = client;
		request.task      = current;
		request.msg_id    = data.request.msg_id;
		request.core_id   = data.request.target_core;
		request.size      = data.request.size;
		request.ubuf      = (unsigned char __user *)data.request.buf;
		request.flag      |= ICC_REQ_FLAG_UBUF;

		ret = icc_receive(client, &request);
		if(ret) {
			return ret;
		}

		if(put_user(request.size, &(((struct icc_ioctl_io_request __user *)arg)->size))) {
			kfree(request.kbuf);
			return -EINVAL;
		}

		res = copy_to_user(request.ubuf, request.kbuf, request.size);
		if(res) {
			kfree(request.kbuf);
			return -EINVAL;
		}
		kfree(request.kbuf);
		break;
	case ICC_IOC_REGISTER_MSG:
		ret = icc_register_msg(client, data.reg_msg.msg_id);
		break;
	case ICC_IOC_CANCLE_MSG:
		ret = icc_cancle_msg(client, data.reg_msg.msg_id);
		break;
	case ICC_IOC_SET_TIMEOUT:
		ret = icc_set_timeout(client, data.reg_msg.msg_id);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static long compat_icc_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct icc_client *client = file->private_data;
	int ret = 0;
	struct icc_request request;
	union icc_ioctl_arg_32 data;
	unsigned long res;

	if (_IOC_SIZE(cmd) > sizeof(data)) {
		printk(KERN_ERR "%s %d size error %d %ld\n", __func__, __LINE__, _IOC_SIZE(cmd), sizeof(data));
		return -EINVAL;
	}

	if (copy_from_user(&data, (void __user *)arg, _IOC_SIZE(cmd))) {
		printk(KERN_ERR "%s %d copy_from_user error\n", __func__, __LINE__);
		return -EFAULT;
	}

	memset(&request, 0, sizeof(request));

	switch (cmd) {
	case ICC_IOC_SEND_32:
		request.client    = client;
		request.task      = current;
		request.msg_id    = data.request.msg_id;
		request.core_id   = data.request.target_core;
		request.size      = data.request.size;
		request.ubuf      = (unsigned char __user *)((unsigned long)data.request.buf);
		request.flag      |= ICC_REQ_FLAG_UBUF;
		request.flag      |= ICC_REQ_FLAG_TX;

		ret = icc_send(client, &request);
		if(ret) {
			printk(KERN_ERR "%s %d icc_send error\n", __func__, __LINE__);
			return ret;
		}
		break;
	case ICC_IOC_RECEIVE_32:
		request.client    = client;
		request.task      = current;
		request.msg_id    = data.request.msg_id;
		request.core_id   = data.request.target_core;
		request.size      = data.request.size;
		request.ubuf      = (unsigned char __user *)((unsigned long)data.request.buf);
		request.flag      |= ICC_REQ_FLAG_UBUF;

		ret = icc_receive(client, &request);
		if(ret) {
			printk(KERN_ERR "%s %d icc_receive error\n", __func__, __LINE__);
			return ret;
		}

		if(put_user(request.size, &(((struct icc_ioctl_io_request_32 __user *)arg)->size))) {
			printk(KERN_ERR "%s %d put_user error\n", __func__, __LINE__);
			kfree(request.kbuf);
			return -EINVAL;
		}

		res = copy_to_user(request.ubuf, request.kbuf, request.size);
		if(res) {
			printk(KERN_ERR "%s %d copy_to_user error\n", __func__, __LINE__);
			kfree(request.kbuf);
			return -EINVAL;
		}
		kfree(request.kbuf);
		break;
	case ICC_IOC_REGISTER_MSG_32:
		ret = icc_register_msg(client, data.reg_msg.msg_id);
		break;
	case ICC_IOC_CANCLE_MSG_32:
		ret = icc_cancle_msg(client, data.reg_msg.msg_id);
		break;
	case ICC_IOC_SET_TIMEOUT_32:
		ret = icc_set_timeout(client, data.reg_msg.msg_id);
		break;
	default:
		printk(KERN_ERR "%s %d cmd %d error %d\n", __func__, __LINE__, cmd, ret);
		return -EINVAL;
	}

	if(ret) {
		printk(KERN_ERR "%s %d cmd %d error %d\n", __func__, __LINE__, cmd, ret);
	}

	return ret;
}

static const struct file_operations icc_fops = {
	.owner          = THIS_MODULE,
	.open           = icc_open,
	.release        = icc_release,
	.unlocked_ioctl = icc_ioctl,
	.compat_ioctl   = compat_icc_ioctl,
};

/* An example to dump each cpu's stack periodically */
#if 0
#include <linux/timer.h>
#include <linux/smp.h>

static struct timer_list icc_timer = {0};

void icc_dump_statck(void *info)
{
	dump_stack();
}

void icc_timer_function(unsigned long arg)
{
	on_each_cpu(icc_dump_statck, NULL, 0);
	mod_timer(&icc_timer, jiffies + msecs_to_jiffies(5000));
}

void icc_timer_init(void)
{
	icc_timer.function = icc_timer_function;
	icc_timer.data = 0;
	icc_timer.expires = jiffies + HZ;
	add_timer(&icc_timer);
}
#endif

struct icc_core *icc_core_create(void)
{
	struct icc_core *icc;
	int ret;

	icc = kzalloc(sizeof(*icc), GFP_KERNEL);
	if(IS_ERR(icc)) {
		return ERR_PTR(-ENOMEM);
	}

	icc->dev.minor  = MISC_DYNAMIC_MINOR;
	icc->dev.name   = "icc";
	icc->dev.fops   = &icc_fops;
	icc->dev.parent = NULL;
	icc->core_id    = ICC_CORE_A_0;

	INIT_LIST_HEAD(&icc->client_list);

	mutex_init(&icc->mutex);
	spin_lock_init(&icc->lock);

	ret = misc_register(&icc->dev);
	if (ret) {
		icc_error("failed to register misc device.\n");
		kfree(icc);
		return ERR_PTR(ret);
	}

	idr_init(&icc->idr);

	icc->dbg_dir = debugfs_create_dir("icc", NULL);
	if(!icc->dbg_dir) {
		icc_debug("create debug fs dir fail\n");
	} else {
		icc->clients_dbg_dir = debugfs_create_dir("clients", icc->dbg_dir);
	}

	return icc;
}

int icc_core_destroy(struct icc_core *icc)
{
	struct icc_client *client, *t_client;
	struct icc_peer *peer;
	int i;

	icc_shm_exit(icc);

	mutex_lock(&icc->mutex);
	/* destroy all client */
	list_for_each_entry_safe(client, t_client, &icc->client_list, list) {
		list_del(&client->list);
		icc_client_destroy(client);
	}

	for(i = 0; i < ICC_CORE_TOTAL; i++) {
		peer = &icc->peer[i];
		if(peer->valid) {
			icc_peer_destroy(peer);
		}
	}

	idr_destroy(&icc->idr);

	mutex_unlock(&icc->mutex);
	misc_deregister(&icc->dev);
	return 0;
}
