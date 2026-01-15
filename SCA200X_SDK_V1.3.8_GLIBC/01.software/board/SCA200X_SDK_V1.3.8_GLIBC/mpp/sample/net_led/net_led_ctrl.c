#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>        
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <netinet/tcp.h>

#include "log.h"
#include "iniparser.h"
#include "dictionary.h"

char server_ip[16] = {0};

int get_server_ip(char * path)
{
	dictionary * ini;
	int i = 0;


	ini = iniparser_load(path);
    if (ini==NULL) {
        log("cannot parse %s\r\n", path);
        return -1 ;
    }
   // iniparser_dump(ini, stderr);
	
	strcpy(server_ip, iniparser_getstring(ini, "common:server_ip", "192.168.31.211"));
	
	iniparser_freedict(ini);

	return 0;
}
int ping_server_ip()
{
	FILE *fp = NULL;
    char line[256] = {0};
	char cmd[32] = {0};
    char * p;
    char n_tranas[3];
    char n_receive[3];

	sprintf(cmd, "ping %s -c 1", server_ip);
	
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        log("error!\n");
        return -1;
    }
    while(!feof(fp))
    {
        if (fgets(line, sizeof(line), fp) != NULL)
        {

            //printf("p:%s", line);

            if ((p =strstr(line, "packets")) !=NULL)

            {
                sscanf(line, "%[^ ] %*s %*s %[^ ]", n_tranas,n_receive);
            }
        }
    }

//	log("trans %s recv %s\r\n", n_tranas, n_receive);

	pclose(fp);


	if(strcmp(n_receive, "0") == 0)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}


int main()
{
	int fd = -1;

	int ret = get_server_ip("/userdata/common.ini");
	if(ret < 0){
		return -1;
	}
	log("get server_ip %s\r\n", server_ip);

	
	while(1)
	{
		fd = ping_server_ip();

//		log("check_net fd = %d\r\n", fd);
		if(fd == -1)
		{
			log("check net error, can not ping %s\r\n", server_ip);
			system("sudo /etc/init.d/led off 5");
		}
		else if(fd == 1)
		{
			system("sudo /etc/init.d/led on 5");
		}
		sleep(5);
	}
	return 0;
}

#if 0
char gateway[16] = {0};
int get_gateway()
{
	char buf[1024] = {0};
	FILE * fp = fopen("/etc/network/interfaces", "r");
	if(fp == NULL){
		log("fp == NULL, open /etc/network/interfaces error\r\n");
		return -1;
	}

	fread(buf, 1, 1024, fp);

	fclose(fp);

	char *data_Start = NULL;
    char *data_ValueStart = NULL;
    char *data_ValueEnd = NULL;

	
	data_Start = strstr(buf, "gateway ");
    data_ValueStart = data_Start + strlen("gateway ");
    data_ValueEnd = strchr(data_ValueStart + 1, 'd');
    strncpy(gateway, data_ValueStart, data_ValueEnd - data_ValueStart - 1);

	return 0;
}

int ping_gateway()
{
	FILE *fp = NULL;
    char line[256] = {0};
	char cmd[32] = {0};
    char * p;
    char n_tranas[3];
    char n_receive[3];

	sprintf(cmd, "ping %s -c 1", gateway);
	
    fp = popen(cmd, "r");
    if (fp == NULL)
    {
        log("error!\n");
        return -1;
    }
    while(!feof(fp))
    {
        if (fgets(line, sizeof(line), fp) != NULL)
        {
            //printf("p:%s", line);
            if ((p =strstr(line, "packets")) !=NULL)
            {
                sscanf(line, "%[^ ] %*s %*s %[^ ]", n_tranas,n_receive);
            }
        }
    }

//	log("trans %s recv %s\r\n", n_tranas, n_receive);

	pclose(fp);

	if(strcmp(n_receive, "0") == 0)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}


int main()
{
	int fd = -1;

	int ret = get_gateway();
	if(ret < 0){
		return -1;
	}
	log("get get_gateway %s\r\n", gateway);

	
	while(1)
	{
		fd = ping_gateway();
//		log("check_net fd = %d\r\n", fd);
		if(fd == -1)
		{
			log("check net error, can not ping %s\r\n", gateway);
			system("sudo /etc/init.d/led off 5");
		}
		else if(fd == 1)
		{
			system("sudo /etc/init.d/led on 5");
		}
		sleep(5);
	}
	return 0;
}
#endif
