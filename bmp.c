#include "bmp.h"


/*
	输入参数：
		bmpfilename：bmp文件名；
		yuvfilename：yuv文件名
		yuvmode：yuv的格式
	功能：将bmp文件转换为yuv格式，模式可选
	返回值：无
*/

void get_bmpdata(char * bmpfilename,char * yuvfilename, char yuvmode)
{
	FILE * fp, *fv;
	int i;
	WORD lineSize;				//一行的字节个数
	BMPFileHead bfh;			//定义文件头信息
	BMPHeaderInfo bhi;			//定义bmp头信息
	//bmp有单色，16色，256色和24位无调色板四种模式。
	//色板作为数据的颜色映射（直接映射表项），不改动，访问频率高，所以采用数组来存储
	RGB  color[256]={0};		
	YUV  yuvcolor[256]={0};		//RGB的YUV映射
	BYTE * databuf;				//数据缓存

	if((fp = fopen(bmpfilename, "r+b"))== NULL){
		printf("open bmpfile error!\n");
		exit(EXIT_FAILURE);
	}
	//读取文件头信息
	if ((fread(&bfh,sizeof(WORD), 7, fp)) < 7){
		printf("read file error!\n");
		exit(EXIT_FAILURE);
	}
	//读取位图信息头
	if ((fread(&bhi,sizeof(WORD),20 , fp)) < 20){
		printf("read file error!\n");
		exit(EXIT_FAILURE);
	}
	if (bhi.biHeight%2 == 1 || bhi.biWidth%2 ==1){
		printf("the size of picture is wrong!\n");
		exit(EXIT_FAILURE);
	}
	//如果不是24位色则获取调色板
	if (bhi.biBitCount != 0x18){
		for (i=0;i<pow(2,bhi.biBitCount);i++){
			fread(&(color[i]),sizeof(BYTE), 4, fp);
			calculateYUV((yuvcolor+i), color[i]);
		}
	}
	lineSize = bhi.biSizeImage/bhi.biHeight;
	//分配内存获取bmp数据
	databuf = (BYTE *)malloc(sizeof(BYTE)*bhi.biSizeImage);
	if (databuf == NULL){
		printf("mallocation error!\n");
		exit(EXIT_FAILURE);
	}
	//一次性读取全部数据
	for (i = bhi.biSizeImage-lineSize;i>=0;i=i-lineSize){
		if ((fread(databuf+i, sizeof(BYTE),  lineSize, fp)) < lineSize){
			printf("get data error!\n");
			exit(EXIT_FAILURE);
		}
	}
	fclose(fp);
	//打开要写入的yuv文件
	if((fv = fopen(yuvfilename, "w+b"))== NULL){
		printf("open yuv file error!\n");
		exit(EXIT_FAILURE);
	}
	//格式转换
	to_yuv(fv,databuf, yuvcolor, yuvmode,bhi);
	//释放内存关闭文件
	free(databuf);
	fclose(fv);
}




/*
	输入参数：
		fv：yuv文件指针
		databuf：从bmp读取的4字节数据
		yuvcolor：rgb的yuv颜色映射表
		yuvmode：yuv写的模式
		bhi:bmp的格式信息
	功能：根据databuf的数据查找yuvcolor中的颜色，按照yuvmode的格式写入fv指向的文件中
	返回值：无；
*/
void to_yuv(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode, BMPHeaderInfo bhi)
{
	switch (bhi.biBitCount){
	case 1://1个像素1位,2种颜色
		writeyuv2(fv, databuf, yuvcolor, yuvmode,bhi);
		break;
	case 4://1个像素4位， 16种颜色
		writeyuv16(fv, databuf, yuvcolor, yuvmode,bhi);
		break;
	case 8://1个像素8位，256种颜色
		writeyuv256(fv, databuf, yuvcolor, yuvmode,bhi);
		break;
	case 24://24位无调色板
		writeyuv24(fv, databuf, yuvmode,bhi);
		break;
	default:{
		printf("bmp error!\n");
		exit(EXIT_FAILURE);
			}
	}
}


/*
	输入参数：
		fv：yuv文件指针；
		databuf：数据缓冲区；
		yuvcolor：rgb的yuv颜色映射；
		yuvmode：模式；
		bhi：bmp文件信息
	功能：将单色的bmp文件转换为指定格式的yuv文件
	返回值：无
*/
void writeyuv2(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode,BMPHeaderInfo bhi)
{
	unsigned long i;
	int flag=0, count=0;
	BYTE bit = 0x80;
	DWORD lineSize = bhi.biSizeImage/bhi.biHeight;

	if (yuvmode == '0'){
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvY), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)){
					count = 0;
					//写完Y之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=1;
			}	
		}
		//写U
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvU), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)/2){
					count = 0;
					//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
					if((i+1)%lineSize == 0 && (i+1)>=lineSize)
						i = i+lineSize;
					else
						i = i+ (lineSize-(i+1)%lineSize)+ lineSize;
					break;
				}
				bit >>=2;
			}
		}
		//写V
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvV), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)/2){
					count = 0;
					//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
					if((i+1)%lineSize == 0 && (i+1)>=lineSize)
						i = i+lineSize;
					else
						i = i+ (lineSize-(i+1)%lineSize)+ lineSize;
					break;
				}
				bit >>=2;
			}
		}

	} 
	else if(yuvmode == '2'){
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvY), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)){
					count = 0;
					//写完Y之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=1;
			}	
		}
		//写U
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvU), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)/2){
					count = 0;
					//写完U之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=2;
			}
		}
		//写V
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvV), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)/2){
					count = 0;
					//写完V之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=2;
			}
		}
	}
	else if(yuvmode == '4'){
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvY), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)){
					count = 0;
					//写完Y之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=1;
			}	
		}
		//写U
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvU), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)){
					count = 0;
					//写完U之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=1;
			}
		}
		//写V
		for (i=0; i<(bhi.biSizeImage*sizeof(BYTE)); i++){
			bit = 0x80;
			while (bit != 0){
				flag=0;
				if ((databuf[i]&bit) == bit){
					flag=1;
				}
				if ((fwrite(&(yuvcolor[flag].yuvV), sizeof(BYTE), 1, fv))== 0){
					printf("write error!\n");
					exit(EXIT_FAILURE);
				}
				count++;
				if(count == (bhi.biWidth)){
					count = 0;
					//写完V之后，i加上该行剩余的空字节
					if((i+1)%lineSize != 0)
						i = i+ (lineSize-(i+1)%lineSize);
					break;
				}
				bit >>=1;
			}
		}
	}
	else{
		printf("bmp to yuv error!（2）\n");
		exit(EXIT_FAILURE);
	}
}



/*
	输入参数：
		fv：yuv文件指针；
		databuf：数据缓冲区；
		yuvcolor：rgb的yuv颜色映射；
		yuv：模式；
		bhi：bmp文件信息
	功能：将16色的bmp文件转换为指定格式的yuv文件
	返回值：无
*/
void writeyuv16(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode,BMPHeaderInfo bhi)
{
	DWORD lineSize = bhi.biSizeImage/bhi.biHeight;
	unsigned long i, count=0;
	BYTE bit = 0x0f;

	if (yuvmode == '0'){
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
				continue;
			}
			bit = databuf[i] & 0x0f;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写U
		count =0;
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize == 0 && (i+1)>=lineSize)
					i = i+lineSize;
				else
					i = i+ (lineSize-(i+1)%lineSize)+ lineSize;
			}
			
		}
		//写V
		count =0;
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize == 0 && (i+1)>=lineSize)
					i = i+lineSize;
				else
					i = i+ (lineSize-(i+1)%lineSize)+ lineSize;
			}
		}
	}
	else if(yuvmode == '2'){
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;		
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
				continue;
			}
			bit = databuf[i] & 0x0f;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写U
		count =0;
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写V
		count =0;
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
	}
	else if(yuvmode == '4'){
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			bit = databuf[i] & 0x0f;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
				continue;
			}
			if ((fwrite(&(yuvcolor[bit].yuvY), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写U
		count =0;
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			bit = databuf[i] & 0x0f;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
				continue;
			}
			if ((fwrite(&(yuvcolor[bit].yuvU), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写V
		count =0;
		for (i=0; i<bhi.biSizeImage*sizeof(BYTE); i++){
			bit = databuf[i] & 0xf0;
			bit >>=4;
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			bit = databuf[i] & 0x0f;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
				continue;
			}
			if ((fwrite(&(yuvcolor[bit].yuvV), sizeof(BYTE), 1, fv))== 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
	}
}


/*
	输入参数：
		fv：yuv文件指针；
		databuf：数据缓冲区；
		yuvcolor：rgb的yuv颜色映射；
		yuv：模式；
		bhi：bmp文件信息
	功能：将256色的bmp文件转换为指定格式的yuv文件
	返回值：无
*/
void writeyuv256(FILE *fv, BYTE databuf[], YUV yuvcolor[], int yuvmode,BMPHeaderInfo bhi)
{
	unsigned long i, count=0;
	DWORD lineSize = bhi.biSizeImage/bhi.biHeight;

	if (yuvmode == '0'){
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i++){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvY),sizeof(BYTE), 1, fv)) == 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写U
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i=i+2){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvU),sizeof(BYTE), 1, fv)) ==0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize == 0 && (i+1)>=lineSize)
					i = i+lineSize;
				else
					i = i+ (lineSize-(i+1)%lineSize)+ lineSize;
			}
		}
		//写V
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i=i+2){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvV),sizeof(BYTE), 1, fv)) ==0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize == 0 && (i+1)>=lineSize)
					i = i+lineSize;
				else
					i = i+ (lineSize-(i+1)%lineSize)+ lineSize;
			}
		}
	} 
	else if(yuvmode == '2'){
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i++){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvY),sizeof(BYTE), 1, fv)) == 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写U
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i=i+2){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvU),sizeof(BYTE), 1, fv)) ==0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写V
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i=i+2){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvV),sizeof(BYTE), 1, fv)) ==0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
	}
	else if(yuvmode == '4'){
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i++){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvY),sizeof(BYTE), 1, fv)) == 0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写U
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i++){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvU),sizeof(BYTE), 1, fv)) ==0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count ++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
		//写V
		for (i=0;i<bhi.biSizeImage*sizeof(BYTE); i++){
			if ((fwrite(&(yuvcolor[databuf[i]].yuvV),sizeof(BYTE), 1, fv)) ==0){
				printf("write error!\n");
				exit(EXIT_FAILURE);
			}
			count ++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if((i+1)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize);
			}
		}
	}

}


/*
	输入参数：
		fv：yuv文件指针；
		databuf：数据缓冲区；
		yuv：模式；
		bhi：bmp文件信息
	功能：将rgb共24位的bmp文件转换为指定格式的yuv文件
	返回值：无
*/
void writeyuv24(FILE *fv, BYTE databuf[], int yuvmode,BMPHeaderInfo bhi)
{
	unsigned long i, count=0;
	YUV yuv24;
	DWORD lineSize = bhi.biSizeImage/bhi.biHeight;

	if (yuvmode == '0'){
		for (i=0; i<bhi.biSizeImage; i=i+3){
			yuv24.yuvY = 0.257*databuf[i+2] + \
				0.504*databuf[i+1] + 0.098*databuf[i] + 16;
			if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
		//写U
		for (i=0; i<bhi.biSizeImage; i=i+6){
			yuv24.yuvU = -0.148*databuf[i+2] - \
				0.291*databuf[i+1] + 0.439*databuf[i] + 128;
			if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完U之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+3)%lineSize == 0 && (i+3)>=lineSize)
					i = i+lineSize;
				else
					i = i+ (lineSize-(i+1)%lineSize)-2+ lineSize;
			}
		}
		//写V
		for (i=0; i<bhi.biSizeImage; i=i+6){
			yuv24.yuvV = 0.439*databuf[i+2] - \
				0.368*databuf[i+1] - 0.071*databuf[i] + 128;
			if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)/2){
				count = 0;
				//写完V之后，i加上该行剩余的空字节和下一行的字节，跳过下一行
				if((i+3)%lineSize == 0 && (i+3)>=lineSize)
					i = i+lineSize;
				else
					i = i+ (lineSize-(i+1)%lineSize)-2+ lineSize;
			}
		}
	} 
	else if(yuvmode == '2'){
		for (i=0; i<bhi.biSizeImage; i=i+3){
			yuv24.yuvY = 0.257*databuf[i+2] + \
				0.504*databuf[i+1] + 0.098*databuf[i] + 16;
			if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
		//写U
		for (i=0; i<bhi.biSizeImage; i=i+6){
			yuv24.yuvU = -0.148*databuf[i+2] - \
				0.291*databuf[i+1] + 0.439*databuf[i] + 128;
			if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
		//写V
		for (i=0; i<bhi.biSizeImage; i=i+6){
			yuv24.yuvV = 0.439*databuf[i+2] - \
				0.368*databuf[i+1] - 0.071*databuf[i] + 128;
			if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
	}
	else if(yuvmode == '4'){
		for (i=0; i<bhi.biSizeImage; i=i+3){
			yuv24.yuvY = 0.257*databuf[i+2] + \
				0.504*databuf[i+1] + 0.098*databuf[i] + 16;
			if ((fwrite(&(yuv24.yuvY), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			if(count == (bhi.biWidth)){
				count = 0;
				//写完Y之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
		//写U
		for (i=0; i<bhi.biSizeImage; i=i+3){
			yuv24.yuvU = -0.148*databuf[i+2] - \
				0.291*databuf[i+1] + 0.439*databuf[i] + 128;
			if ((fwrite(&(yuv24.yuvU), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完U之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
		//写V
		for (i=0; i<bhi.biSizeImage; i=i+3){
			yuv24.yuvV = 0.439*databuf[i+2] - \
				0.368*databuf[i+1] - 0.071*databuf[i] + 128;
			if ((fwrite(&(yuv24.yuvV), sizeof(BYTE), 1, fv)) == 0){
				printf("wtite error!\n");
				exit(EXIT_FAILURE);
			}
			count++;
			if(count == (bhi.biWidth)){
				count = 0;
				//写完V之后，i加上该行剩余的空字节
				if((i+3)%lineSize != 0)
					i = i+ (lineSize-(i+1)%lineSize)-2;
			}
		}
	}
}


//映射yuv的颜色表
void calculateYUV(YUV *yuvcolor,RGB rgbcolor )
{
	yuvcolor->yuvY = 0.257*rgbcolor.rgbRed + \
		0.504*rgbcolor.rgbGreen + 0.098*rgbcolor.rgbBlue + 16;
	yuvcolor->yuvU = -0.148*rgbcolor.rgbRed - \
		0.291*rgbcolor.rgbGreen + 0.439*rgbcolor.rgbBlue + 128;
	yuvcolor->yuvV = 0.439*rgbcolor.rgbRed - \
		0.368*rgbcolor.rgbGreen - 0.071*rgbcolor.rgbBlue + 128;
}
