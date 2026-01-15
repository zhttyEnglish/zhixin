/******************************************************************************
 * Because Nor & Nand use the same QSPI;
 * add the Mutex function for MCP (Nor & Nand) Die switch function;
 *****************************************************************************/

#define pr_fmt(fmt) "sc_mcp_mutex: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>

static DEFINE_MUTEX(mcp_mutex);
static int enter = 0, exit = 0;

void mcp_mutex_get(void)
{
	mutex_lock(&mcp_mutex);
}
EXPORT_SYMBOL(mcp_mutex_get);

void mcp_mutex_release(void)
{
	mutex_unlock(&mcp_mutex);

	BUG_ON(exit != enter);
}
EXPORT_SYMBOL(mcp_mutex_release);

int mcp_enter_critical(int old_flag)
{
	if (!old_flag)
		mcp_mutex_get();

	return 1;
}
EXPORT_SYMBOL(mcp_enter_critical);

int mcp_exit_critical(int old_flag)
{
	if (!old_flag)
		mcp_mutex_release();

	return old_flag;
}
EXPORT_SYMBOL(mcp_exit_critical);

