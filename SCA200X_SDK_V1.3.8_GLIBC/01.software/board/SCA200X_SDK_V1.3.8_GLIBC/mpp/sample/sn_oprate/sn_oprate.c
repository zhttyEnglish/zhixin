#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "/userdata/SN"

void show_usage()
{
	printf("usage: \r\n");
	printf("./sn_oprate read \r\n");
	printf("./sn_oprate write serial_number \r\n");
}

int main(int argc, char ** argv)
{
	FILE * fp = NULL;
	char buf[32] = {0};

	if(argc != 2 && argc != 3){
		show_usage();
		return -1;
	}

	if(strncmp(argv[1], "read", 4) == 0)
	{
		fp = fopen(PATH, "r+");
		fread(buf, 1, 32, fp);
		printf("%s\r\n", buf);
		fclose(fp);
	}
	else if(strncmp(argv[1], "write", 5) == 0)
	{
		fp = fopen(PATH, "w+");
		int len = 0;
		len += sprintf(&buf[len], "serial_number:%s", argv[2]);
		fwrite(buf, len, 1, fp);
		printf("done\r\n");
		fclose(fp);
	}
	else
	{
		show_usage();
		return -1;
	}

	return 0;
}
