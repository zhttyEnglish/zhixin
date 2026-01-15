#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "/userdata/version"

void show_usage()
{
	printf("usage: \r\n");
	printf("./version_oprate read \r\n");
	printf("./version_oprate write v1.1.1 alpha 202510301327 \r\n");
}

int main(int argc, char ** argv)
{
	FILE * fp = NULL;
	char buf[256] = {0};

	if(argc != 2 && argc != 5){
		show_usage();
		return -1;
	}

	if(strncmp(argv[1], "read", 4) == 0)
	{
		fp = fopen(PATH, "r+");
		fread(buf, 1, 256, fp);
		printf("%s\r\n", buf);
		fclose(fp);
	}
	else if(strncmp(argv[1], "write", 5) == 0)
	{
		fp = fopen(PATH, "w+");
		int len = 0;
		len += sprintf(&buf[len], "Version_label=%s\r\nBuild_time=%s\r\nFirmware_version=ZX_A1-%s-%s+%s"
									"\r\nHardware_version=dp2004t_v20_20250901", 
			argv[3], argv[4], argv[2], argv[3], argv[4]);

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
