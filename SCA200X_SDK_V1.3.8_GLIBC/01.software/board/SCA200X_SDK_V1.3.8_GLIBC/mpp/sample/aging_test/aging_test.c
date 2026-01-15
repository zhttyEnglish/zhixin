#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

void show_usage()
{
	printf("usage: ./aging_test x(hours) \r\n");
}

int main(int argc, char ** argv)
{
	if(argc != 2){
		show_usage();
		return -1;
	}

	int aging_time = 0;
	sscanf(argv[1], "%d", &aging_time);

	FILE * fp = fopen("/userdata/aging_test.tmp", "w");
	char * buf1 = "start aging test";
	fwrite(buf1, 1, 16, fp);
	fclose(fp);


	struct timeval tv;
	gettimeofday(&tv, NULL);
	long long int time1 = tv.tv_sec;
	printf("---------- start aging test %lld----------\r\n", time1);
	char cmd[64] = {0};
	sprintf(cmd, "sudo /userdata/stress-ng --cpu 4 -t %dh &", aging_time);	
	system(cmd);

	while(1)
	{
		system("sudo /etc/init.d/led on all");
		sleep(0.5);
		system("sudo /etc/init.d/led off all");
		sleep(0.5);
		gettimeofday(&tv, NULL);
		long long int time2 = tv.tv_sec;
		if(time2 - time1 >= aging_time * 3600){
			break;
		}
	}

	fp = fopen("/userdata/aging_test.tmp", "w");
	char * buf2 = "aging test success";
	fwrite(buf2, 1, 18, fp);
	fclose(fp);

	return 0;
}
