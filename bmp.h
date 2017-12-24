#ifndef BMP_H
#define BMP_H


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef unsigned char BYTE ;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef long LONG;
#pragma pack(1)
typedef struct  {  
	WORD bfType;			/* 说明文件的类型 */  
	DWORD bfSize;			/* 说明文件的大小，用字节为单位 */  
	WORD bfReserved1;		/* 保留，设置为0 */  
	WORD bfReserved2;		/* 保留，设置为0 */  
	DWORD bfOffsetBytes;		/* 说明从BITMAPFILEHEADER结构开始到实际的图像数据之间的字节偏移量 */  
	
}BMPFileHead; //14字节,但是sizeof计算长度时为16字节


typedef struct  {  
	DWORD biSize;			/* 说明结构体所需字节数 */  
	LONG biWidth;			/* 以像素为单位说明图像的宽度 */  
	LONG biHeight;			/* 以像素为单位说明图像的高度 */  
	WORD biPlanes;			/* 说明位面数，必须为1 */  
	WORD biBitCount;		/* 说明位数/像素，1、2、4、8、24 */  
	DWORD biCompression;	/* 说明图像是否压缩及压缩类型BI_RGB，BI_RLE8，BI_RLE4，BI_BITFIELDS */  
	DWORD biSizeImage;		/* 以字节为单位说明图像大小，必须是4的整数倍*/  
	LONG biXPixelsPerMeter;	/*目标设备的水平分辨率，像素/米 */  
	LONG biYPixelsPerMeter;	/*目标设备的垂直分辨率，像素/米 */  
	DWORD biClrUsed;		/* 说明图像实际用到的颜色数，如果为0，则颜色数为2的biBitCount次方 */  
	DWORD biClrImportant;	/*说明对图像显示有重要影响的颜色索引的数目，如果是0，表示都重要。*/  
}BMPHeaderInfo;  //40字节
#pragma pack()

typedef struct  {  
	BYTE rgbBlue;		/*指定蓝色分量*/  
	BYTE rgbGreen;		/*指定绿色分量*/  
	BYTE rgbRed;		/*指定红色分量*/  
	BYTE rgbReserved;	/*保留，指定为0*/  
}RGB;

typedef struct yuvtype {  
	BYTE yuvY;		/*指定Y亮度分量*/  
	BYTE yuvU;		/*指定U（Cb）分量*/  
	BYTE yuvV;		/*指定V（Cr）分量*/  
}YUV;
void get_bmpdata(char * bmpfilename,char * yuvfilename, char yuvmode);
void to_yuv(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi);
void calculateYUV(YUV *yuvcolor,RGB rgbcolor );


void writeyuv2(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode,BMPHeaderInfo bhi);
void writeyuv16(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode,BMPHeaderInfo bhi);
void writeyuv256(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode,BMPHeaderInfo bhi);
void writeyuv24(FILE *fv, BYTE databuf[], int yuvmode,BMPHeaderInfo bhi);
#endif