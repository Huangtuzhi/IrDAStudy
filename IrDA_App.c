#include <stdio.h>
#include <unistd.h>

int main(int argc,int *argv[])
{
	int buffer[256];
	int i=0;
	int value=-1;
	printf("Displaying the Code Length of IrDA...\n");
	int fd=open("/dev/IrDAStudy",0);
	if (fd < 0){
		perror("Open study device");
		return 1;
	}
	
	int len=read(fd,buffer,sizeof buffer-1);
	
	printf("The number of learned code %d \n",len);
	if(len >0)
	{
		for(i=0;i<len;i++)
		{
			printf("Length Value: %d\n",buffer[i]);
		}

	}
	else
	{
		perror("read device");
		return 1;
	}
	close(fd);
}
