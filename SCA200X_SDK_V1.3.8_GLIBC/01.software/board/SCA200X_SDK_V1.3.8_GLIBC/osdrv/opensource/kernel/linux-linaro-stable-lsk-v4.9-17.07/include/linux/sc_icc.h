#ifndef __SC_ICC_ICC_LINUX_H__
#define __SC_ICC_ICC_LINUX_H__

#include <linux/types.h>

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

/* smartchip kernel api */
void *sc_icc_client_create(void);
int sc_icc_client_destroy(void *client);
int sc_icc_client_register_msgid(void *client, uint32_t msg_id);
int sc_icc_client_unregister_msgid(void *client, uint32_t msg_id);
int sc_icc_client_send(void *client,
    unsigned char *buf, uint32_t size,
    uint32_t msg_id, uint32_t core_id);
int sc_icc_client_receive(void *client, unsigned char *buf, uint32_t size, uint32_t msg_id);
int sc_icc_client_set_timeout(void *client, uint32_t timeout);

#endif
