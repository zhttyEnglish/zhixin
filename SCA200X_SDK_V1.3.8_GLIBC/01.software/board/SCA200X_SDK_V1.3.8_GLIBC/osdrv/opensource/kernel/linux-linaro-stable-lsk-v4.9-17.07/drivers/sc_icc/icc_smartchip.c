/*
 * SmartChip Icc driver
 */
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/io.h>
#include "icc_priv.h"

#define ICC_TOKEN_STR_MAXLEN 32
#define ICC_TOKEN_ID_NUM 32

struct icc_token_id {
	char token[ICC_TOKEN_STR_MAXLEN];
	unsigned short msgid;
};

struct smartchip_icc_dev {
	struct mutex mutex;
	struct icc_core *icc;
};

static struct icc_token_id g_icc_token_id[ICC_TOKEN_ID_NUM] = {0};

struct smartchip_icc_dev *smartchip_icc_device;

void smartchip_wakeup_rx_thread(void)
{
	if (smartchip_icc_device && smartchip_icc_device->icc) {
#if 0
		//spin_lock(&smartchip_icc_device->icc->lock);
		if(smartchip_icc_device->icc->rcv_tsk)
		    wake_up_process(smartchip_icc_device->icc->rcv_tsk);
		//spin_unlock(&smartchip_icc_device->icc->lock);
#endif
		wakeup_icc_shm_poll_rx_thread();
	}
}

static int smartchip_icc_probe(struct platform_device *pdev)
{
	struct smartchip_icc_dev *icc_dev;
	int ret;

	icc_dev = kzalloc(sizeof(*icc_dev), GFP_KERNEL);
	if(IS_ERR(icc_dev)) {
		return PTR_ERR(icc_dev);
	}

	icc_dev->icc = icc_core_create();
	if(IS_ERR(icc_dev->icc)) {
		return PTR_ERR(icc_dev->icc);
	}

	pdev->dev.platform_data = icc_dev->icc;
	ret = icc_of_mem_init(pdev);
	if(ret)
		goto out;

	ret = icc_vmap_mem(icc_dev->icc);
	if(ret)
		goto out;

	ret = icc_shm_init(icc_dev->icc);
	if(ret)
		goto out;

	ret = icc_of_peer_init(pdev);
	if(ret)
		goto out;

	/* Store global ptr */
	smartchip_icc_device = icc_dev;
	mutex_init(&smartchip_icc_device->mutex);
	return ret;
out:
	kfree(icc_dev);
	return ret;
}

static int smartchip_icc_remove(struct platform_device *pdev)
{
	struct icc_core *icc = pdev->dev.platform_data;
	struct smartchip_icc_dev *icc_dev =
	    container_of(&icc, struct smartchip_icc_dev, icc);

	/* unmap shm */
	vunmap(icc->virt_addr);

	icc_core_destroy(icc);

	kfree(icc_dev);
	return 0;
}

static const struct of_device_id smartchip_ion_match_table[] = {
	{.compatible = "smartchip,icc"},
	{},
};

static struct platform_driver smartchip_icc_driver = {
	.probe = smartchip_icc_probe,
	.remove = smartchip_icc_remove,
	.driver = {
		.name = "icc-smartchip",
		.of_match_table = smartchip_ion_match_table,
	},
};

static int __init smartchip_icc_init(void)
{
	return platform_driver_register(&smartchip_icc_driver);
}

static void __exit smartchip_icc_exit(void)
{
	platform_driver_unregister(&smartchip_icc_driver);
}

MODULE_AUTHOR("smartchip");
MODULE_DESCRIPTION("smartchip icc inter-core communication driver");
MODULE_LICENSE("GPL");
subsys_initcall(smartchip_icc_init);
module_exit(smartchip_icc_exit);

/* smartchip icc kernel api */
void *sc_icc_client_create(void)
{
	unsigned char name[32] = {0};

	sprintf(name, "%s-%d-%d", current->comm, current->group_leader->pid, current->pid);

	return icc_client_create(smartchip_icc_device->icc, name);
}

void sc_icc_client_destroy(void *client)
{
	int found;

	found = icc_client_check(smartchip_icc_device->icc, client);
	if(!found)
		return;

	icc_client_destroy(client);
}

static unsigned short sc_icc_name_hash(const char *pName)
{
	unsigned short Hash = 0;
	const char *pChar = pName;
	while ((*pChar) != 0 ) {
		Hash = ( (Hash & 1) ? 0x8000 : 0) + (Hash >> 1) + (*pChar);
		pChar++;
	}
	//debug_print("pName=%s, Hash=0x%x\r\n",pName,Hash);
	return Hash;
}

//robin: here we use a array to save all the token and id so that they can be dumped easily.
unsigned short sc_icc_ftok(const char *path)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_icc_token_id); i++) {
		//if this token existed, return the id
		if (g_icc_token_id[i].token[0]) {
			if(strcmp(g_icc_token_id[i].token, path) == 0) {
				if(g_icc_token_id[i].msgid) {
					return g_icc_token_id[i].msgid;
				} else {
					pr_debug("Find duplicated token, but no msgid, will try to create a msgid.\r\n");
					break;
				}
			}
		} else {
			strncpy(g_icc_token_id[i].token, path, ICC_TOKEN_STR_MAXLEN);
			g_icc_token_id[i].token[ICC_TOKEN_STR_MAXLEN - 1] = 0;

			break;
		}
	}

	if(i >= ARRAY_SIZE(g_icc_token_id)) {
		pr_err("ICC token reaches MAX: %ld\n", ARRAY_SIZE(g_icc_token_id));
		return -1;
	}

	g_icc_token_id[i].msgid = sc_icc_name_hash(path);

	return g_icc_token_id[i].msgid;
}

void sc_icc_dump_token_id(void)
{
	int i = 0;

	for(i = 0; i < ARRAY_SIZE(g_icc_token_id); i++) {
		if(g_icc_token_id[i].token[0]) {
			pr_debug("token: %s, msgid: %d\r\n", g_icc_token_id[i].token, g_icc_token_id[i].msgid);
		}
	}
}

int sc_icc_client_register_msgid(void *client, uint32_t msg_id)
{
	int found;

	found = icc_client_check(smartchip_icc_device->icc, client);
	if(!found)
		return -EINVAL;

	return icc_register_msg(client, msg_id);
}

int sc_icc_client_unregister_msgid(void *client, uint32_t msg_id)
{
	int found;

	found = icc_client_check(smartchip_icc_device->icc, client);
	if(!found)
		return -EINVAL;

	return icc_cancle_msg(client, msg_id);
}

int sc_icc_client_set_timeout(void *client, uint32_t timeout)
{
	int found = 0;

	found = icc_client_check(smartchip_icc_device->icc, client);
	if(!found)
		return -EINVAL;

	return icc_set_timeout(client, timeout);
}

int sc_icc_client_send(void *client,
    unsigned char *buf, uint32_t size,
    uint32_t msg_id, uint32_t core_id)
{
	int ret, found = 0;
	struct icc_request request = {0};

	found = icc_client_check(smartchip_icc_device->icc, client);
	if(!found)
		return -EINVAL;

	request.client  = client;
	request.task    = current;
	request.core_id = core_id;
	request.kbuf    = buf;
	request.msg_id  = msg_id;
	request.res     = 0;
	request.size    = size;
	request.flag    |= ICC_REQ_FLAG_TX;

	ret = icc_send(client, &request);
	if(ret < 0)
		return ret;

	return request.res;
}

int sc_icc_client_receive(void *client, unsigned char *buf, uint32_t size, uint32_t msg_id)
{
	int ret, found = 0;
	struct icc_request request = {0};

	found = icc_client_check(smartchip_icc_device->icc, client);
	if(!found)
		return -EINVAL;

	request.client  = client;
	request.task    = current;
	request.msg_id  = msg_id;
	request.size    = size;

	ret = icc_receive(client, &request);
	if(ret < 0)
		return ret;

	if(!request.kbuf)
		return -EINVAL;

	memcpy(buf, request.kbuf, request.size);

	kfree(request.kbuf);

	return request.size;
}

EXPORT_SYMBOL(sc_icc_ftok);
EXPORT_SYMBOL(sc_icc_dump_token_id);
EXPORT_SYMBOL(sc_icc_client_create);
EXPORT_SYMBOL(sc_icc_client_destroy);
EXPORT_SYMBOL(sc_icc_client_register_msgid);
EXPORT_SYMBOL(sc_icc_client_unregister_msgid);
EXPORT_SYMBOL(sc_icc_client_set_timeout);
EXPORT_SYMBOL(sc_icc_client_send);
EXPORT_SYMBOL(sc_icc_client_receive);
