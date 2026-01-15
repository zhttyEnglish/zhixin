#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH "/userdata/alg_version"

void show_usage()
{
	printf("usage: \r\n");
	printf("./alg_version read \r\n");
	printf("./alg_version write v1.1.1 \r\n");
}

int main(int argc, char ** argv)
{
	FILE * fp = NULL;
	char buf[128] = {0};

	if(argc != 2 && argc != 3){
		show_usage();
		return -1;
	}

	if(strncmp(argv[1], "read", 4) == 0)
	{
		fp = fopen(PATH, "r+");
		fread(buf, 1, 128, fp);
		printf("%s\r\n", buf);
		fclose(fp);
	}
	else if(strncmp(argv[1], "write", 5) == 0)
	{
		fp = fopen(PATH, "w+");
		int len = 0;
		len += sprintf(&buf[len], "lenovo_algo_version:%s", argv[2]);

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
