#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>

int main(){
	//int fd = creat("helloworld.txt", 0700);
	
	FILE *fp = fopen("helloworld.txt", "w+");
	fprintf(fp, "System Security - Hello World!");
	fclose(fp);
	rename("helloworld.txt", "hw.txt");
	truncate("hw.txt", 2);
	//ftruncate(3, 2);
	int fd = creat("hi.txt", 0777);	

}



