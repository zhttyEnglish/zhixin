
#ifndef __SC_MCP_LINUX_H__
#define __SC_MCP__LINUX_H__

void mcp_mutex_get(void);

void mcp_mutex_release(void);

int mcp_enter_critical(int old_flag);

int mcp_exit_critical(int old_flag);

#endif
