#include "bmp.h"
#include <stdio.h>
#include <stdlib.h>
//帮助信息
void usage(void);


int main(int argc, char * argv[])
{
	
	int i=0;
	char option[3]= {'0','2','4'};//YUV文件的3个模式选项，分别为420，422，444
	if (argc ==4){
		if(argv[1][1] == option[0] || argv[1][1] == option[1] \
			||argv[1][1] == option[2] || argv[2]!= NULL ||argv[3]!= NULL){
				get_bmpdata(argv[2],argv[3],argv[1][1] );
				//get_bmpdata("16_20_20.bmp","1620_20yuv420.yuv",'0' );
		}
		else{
			usage();
		}
	}
	else {
		usage();
	}
}

//help
void usage(void)
{
	printf("\n**************BMP TO YUV**************\n");
	printf("\tBmp文件的长和宽的像素个数必须为偶数\n");
	printf("command:\n\t-0\t将bmp转化为I420(4:2:0)的yuv文件\n");
	printf("\t-2\t将bmp转化为YUY2（4:2:2）的文件格式\n");
	printf("\t-4\t将bmp转化为YUV444(4:4:4)的文件格式\n");
	printf("example:  BMPtoYUV.exe -0 input.bmp ouput.yuv\n");
}