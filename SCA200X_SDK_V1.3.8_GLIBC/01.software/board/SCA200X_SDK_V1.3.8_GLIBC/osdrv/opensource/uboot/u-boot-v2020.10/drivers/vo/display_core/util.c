#include "vo/util.h"

static int queue_pop(sc_queue_t *queue, void **item);
static int queue_insert(sc_queue_t *queue, void *item);
static int get_queue_size(sc_queue_t *queue);
static int look_up_head_item(sc_queue_t *queue, void **item);

extern int gpio_direction_output(unsigned gpio, int value);
extern int gpio_set_value(unsigned gpio, int value);

sc_queue_t *sc_creat_queue(uint32_t queue_size, char *name)
{
	sc_queue_t *queue = NULL;
	queue = (sc_queue_t *)sc_malloc(sizeof(sc_queue_t));
	if(queue && queue_size >= 2 && name) {
		void **data = NULL;
		data = (void **)sc_malloc(sizeof(void *)*queue_size);
		if(data) {
			queue->name = name;
			queue->data = data;
			queue->queue_size = queue_size;
			queue->header = 0;
			queue->tail = queue->header;
			queue->valid_data_num = 0;
			queue->queue_insert = queue_insert;
			queue->queue_pop = queue_pop;
			queue->get_queue_size = get_queue_size;
			queue->look_up_head = look_up_head_item;
			return queue;
		} else {
			if(queue)
				sc_free(queue);
		}
	} else {
		if(queue)
			sc_free(queue);
	}
	sc_err("creat queue failed");
	return NULL;
}
static int queue_pop(sc_queue_t *queue, void **item)
{
	if(!queue || !item) {
		return -1;
	}

	if(queue->valid_data_num == 0) {
		//set to NULL init
		*item = NULL;
		return -1;
	}
	*item = queue->data[queue->header];
	queue->header = (queue->header + 1) % queue->queue_size;
	queue->valid_data_num--;
	return 0;
}

static int get_queue_size(sc_queue_t *queue)
{
	int num = 0;
	if(!queue) {
		return -1;
	}
	num = queue->valid_data_num;
	return num;
}

static int look_up_head_item(sc_queue_t *queue, void **item)
{
	int num = 0;
	if(!queue) {
		return -1;
	}

	if(queue->valid_data_num == 0) {
		//set to NULL init
		*item = NULL;
		return -1;
	}
	*item = queue->data[queue->header];
	return num;
}

static int queue_insert(sc_queue_t *queue, void *item)
{
	if(!queue || !item) {
		return -1;
	}
	if(queue->valid_data_num == queue->queue_size) {
		sc_err("queue %s size=%d is full", queue->name, queue->queue_size);
		return -1;
	}
	queue->data[queue->tail] = item;
	queue->tail = (queue->tail + 1) % queue->queue_size;
	queue->valid_data_num++;
	return 0;
}

int sc_delete_queue(sc_queue_t **queue)
{
	sc_queue_t *p_queue = *queue;

	if(p_queue) {
		if(p_queue->data) {
			sc_free(p_queue->data);
			p_queue->data = NULL;
		}
		sc_free(p_queue);
		*queue = NULL;
	}
	return 0;
}

void write_reg32_Mask(uint32_t regAddr, uint32_t regData, uint32_t regDataMask)
{
	uint32_t u32_regDataTmp;
	volatile uint32_t *ptr_regAddr = (uint32_t *)regAddr;
	uint32_t u32_regDataWithMask = regData & regDataMask;

	u32_regDataTmp = *ptr_regAddr;
	u32_regDataTmp &= ~regDataMask;
	u32_regDataTmp |= u32_regDataWithMask;

	*ptr_regAddr = u32_regDataTmp;
}

void read_write_reg32_mask(uint32_t reg_addr, uint32_t reg_data, uint32_t mask)
{
	uint32_t old = (read_reg32(reg_addr) & (~mask));
	write_reg32(reg_addr, (old | (reg_data & mask)));
}

void sc_lock(sc_lock_t lock_id)
{
	return;
}

void sc_unlock(sc_lock_t lock_id)
{
	return;
}

sc_lock_t sc_creat_lock(void)
{
	return 0;
}

sc_signal_t sc_create_signal(void)
{
	return 0;
}

uint64_t sc_get_timestamp_us(void)
{
	return 0;
}

void sc_enter_critical(void)
{
}

void sc_exit_critical(void)
{
}

void sc_trace_line(void)
{
}

int gpio_set_direct(uint32_t group, uint32_t port, uint32_t inner_gpio, uint32_t dir)
{
	uint32_t  addr;
	uint32_t  value;

	if ((group >= MAX_GPIO_GROUP) || (port >= MAX_GPIO_PORT) ||
	    (inner_gpio >= MAX_GPIO_PIN))
		return -1;

	addr = GPIO_DIR_BASE + group * GPIO_GROUP_STEP + port * GPIO_PORT_DIR_STEP;
	sc_debug("set GPIO_%c%d_%d(addr=0x%x) dir=%d", 'A' + port, group, inner_gpio, addr, dir);
	value = read_reg32(addr);
	if (dir == GPIO_DIR_OUTPUT)
		value |= (1 << inner_gpio);
	else
		value &= (~(1 << inner_gpio));

	write_reg32(addr, value);
	return 0;
}

int gpio_set_val(uint32_t group, uint32_t port, uint32_t inner_gpio, uint32_t val)
{
	uint32_t  addr;
	uint32_t  value;

	if ((group >= MAX_GPIO_GROUP) || (port >= MAX_GPIO_PORT) ||
	    (inner_gpio >= MAX_GPIO_PIN))
		return -1;

	addr = GPIO_OUT_BASE + group * GPIO_GROUP_STEP + port * GPIO_PORT_DIR_STEP;
	sc_debug("set GPIO_%c%d_%d(addr=0x%x) val=%d", 'A' + port, group, inner_gpio, addr, val);
	value = read_reg32(addr);
	if (val == GPIO_DATA_HIGH)
		value |= (1 << inner_gpio);
	else
		value &= (~(1 << inner_gpio));

	write_reg32(addr, value);
	return 0;
}

static void sca200v100_abb_set_pixel(u32 freq)
{
	u32 loop_div, post_div;
	u32 abb_core_reg38;

	sc_debug("freq: %u", freq);

	if (freq > 600000000 || freq < 6250000) {
		printf("Unsupport abb pix freq %d\n", freq);
		return;
	}

	abb_core_reg38 = read_reg32(0x1072098);
	abb_core_reg38 &= ~0x3f;
	write_reg32(0x1072098, abb_core_reg38);

	if (freq > 400000000) {
		loop_div = 5;
		post_div = 1;
	} else if (freq > 200000000) {
		loop_div = 5;
		post_div = 2;
	} else if (freq > 100000000) {
		loop_div = 5;
		post_div = 3;
	} else if (freq > 50000000) {
		loop_div = 5;
		post_div = 4;
	} else if (freq > 25000000) {
		loop_div = 5;
		post_div = 5;
	} else if (freq > 12500000) {
		loop_div = 5;
		post_div = 6;
	} else if (freq >= 6250000) {
		loop_div = 4;
		post_div = 7;
	}

	write_reg32(0x1072098, (loop_div | (post_div << 3)));
	write_reg32(0x1072a10, (u32)(((u64)400000000) *
	        ((u64)(1 << (24 + loop_div - post_div))) /
	        (u64)freq));
}

int  sc_hal_set_pix_clk(float clk)
{
	sca200v100_abb_set_pixel(clk * 1000000);
	return 0;
}

