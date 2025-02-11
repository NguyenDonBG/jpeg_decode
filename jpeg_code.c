// Giải mã JPEG
// Nhập JPEG và xuất Bitmap

#include <stdio.h>
#include <stdlib.h>

#pragma warning(disable : 4996)

unsigned int  BuffIndex;			// Vị trí dữ liệu JPEG
unsigned int  BuffSize;				// Kích thước dữ liệu JPEG
unsigned int  BuffX;				// Kích thước chiều ngang của hình ảnh
unsigned int  BuffY;				// Kích thước chiều dọc của hình ảnh
unsigned int  BuffBlockX; 			// Số lượng MCU theo chiều ngang
unsigned int  BuffBlockY; 			// Số lượng MCU theo chiều dọc
unsigned char *Buff;				// Bộ đệm để lưu dữ liệu giải nén

unsigned char  TableDQT[4][64];		// Bảng lượng hóa
unsigned short TableDHT[4][162]; 	// Bảng Huffman

unsigned short TableHT[4][16];		// Bảng bắt đầu Huffman
unsigned char  TableHN[4][16];		// Số bắt đầu Huffman

unsigned char BitCount = 0;			// Vị trí đọc dữ liệu nén
unsigned int  LineData;				// Dữ liệu sử dụng trong giải nén
unsigned int  NextData;				// Dữ liệu tiếp theo sử dụng trong giải nén

unsigned int  PreData[3];			// Bộ đệm để lưu trữ thành phần DC

unsigned char CompCount;			// Số lượng thành phần
unsigned char CompNum[4];			// Số thành phần (không sử dụng)
unsigned char CompSample[4];	
unsigned char CompDQT[4];			// Bảng DQT của thành phần
unsigned char CompDHT[4];			// Bảng DHT của thành phần
unsigned char CompSampleX, CompSampleY;

// Bảng Zig-Zag đảo ngược
int zigzag_table[]={
	0 ,  1, 8, 16, 9 , 2 , 3 , 10,
	17, 24,32, 25, 18, 11, 4 , 5,
	12, 19,26, 33, 40, 48, 41, 34,
	27, 20,13, 6 , 7 , 14, 21, 28,
	35, 42,49, 56, 57, 50, 43, 36,
	29, 22,15, 23, 30, 37, 44, 51,
	58, 59,52, 45, 38, 31, 39, 46,
	53, 60,61, 54, 47, 55, 62, 63, 0
};

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;

typedef struct tagBITMAPFILEHEADER {
	WORD	bfType;
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

//
// Lưu Bitmap
// file: Tên tệp
// x,y: Kích thước hình ảnh
// b: Số byte (số byte của 1 điểm)
//
void BmpSave(char *file,unsigned char *buff,unsigned int x,unsigned int y,unsigned int b)
{
	BITMAPFILEHEADER lpBf;
	BITMAPINFOHEADER lpBi;
	unsigned char tbuff[4];
	FILE *fp;
	unsigned char str;
	int i,k;

	if((fp = fopen(file,"wb")) == NULL)
	{
		perror(0);
		exit(0);
	}

	// Thiết lập thông tin tệp
	tbuff[0] = 'B';
	tbuff[1] = 'M';
	fwrite(tbuff,2,1,fp);
	tbuff[3] = ((14 +40 +x *y *b) >> 24) & 0xff;
	tbuff[2] = ((14 +40 +x *y *b) >> 16) & 0xff;
	tbuff[1] = ((14 +40 +x *y *b) >>	8) & 0xff;
	tbuff[0] = ((14 +40 +x *y *b) >>	0) & 0xff;
	fwrite(tbuff,4,1,fp);
	tbuff[1] = 0;
	tbuff[0] = 0;
	fwrite(tbuff,2,1,fp);
	fwrite(tbuff,2,1,fp);
	tbuff[3] = 0;
	tbuff[2] = 0;
	tbuff[1] = 0;
	tbuff[0] = 54;
	fwrite(tbuff,4,1,fp);

	// Thiết lập thông tin
	lpBi.biSize				= 40;
	lpBi.biWidth			= x;
	lpBi.biHeight			= y;
	lpBi.biPlanes			= 1;
	lpBi.biBitCount			= b*8;
	lpBi.biCompression	 	= 0;
	lpBi.biSizeImage		= x*y*b;
	lpBi.biXPelsPerMeter	= 300;
	lpBi.biYPelsPerMeter	= 300;
	lpBi.biClrUsed		 	= 0;
	lpBi.biClrImportant		= 0;
	fwrite(&lpBi,1,40,fp);

	// Lật ngược chiều dọc
	for(k=0;k<y/2;k++)
	{
		for(i=0;i<x*3;i++)
		{
			str = buff[k*x*3+i];
			buff[k*x*3+i] = buff[((y-1)*x*3 -k*x*3) +i];
			buff[((y-1)*x*3-k*x*3) +i] = str;
		}
	}

	fwrite(buff,1,x*y*b,fp);

	fclose(fp);
}

// Lấy 1 Byte
unsigned char get_byte(unsigned char *buff)
{
	if(BuffIndex >= BuffSize) return 0;
    BuffIndex = BuffIndex + 1;
	return buff[BuffIndex];
}
//
// Lấy 2 Byte
unsigned short get_word(unsigned char *buff)
{
	unsigned char h,l;
	h = get_byte(buff);
	l = get_byte(buff);
	return (h<<8)|l;
}

// Lấy dữ liệu 32 bit (chỉ sử dụng trong giải nén)
unsigned int get_data(unsigned char *buff)
{
	unsigned char str = 0;
	unsigned int data = 0;
	str = get_byte(buff);	

	// Trong JPEG, 0xFF được sử dụng làm ký tự đánh dấu đặc biệt. Nếu giá trị của pixel là 0xFF, nó sẽ được lưu trữ là 0xFF00 để tránh nhầm lẫn với ký tự đặc biệt 0xFF.
	if(str == 0xff) 
		if(get_byte(buff) == 0x00) 
			str = 0xFF; 
		else 
			str = 0x00;
	data = str;
	str = get_byte(buff);
	if(str == 0xff) 
		if(get_byte(buff) == 0x00) 
			str = 0xFF; 
		else 
			str = 0x00;
	data = (data << 8) | str;
	str = get_byte(buff);
	if(str == 0xff) 
		if(get_byte(buff) == 0x00) 
			str = 0xFF; 
		else 
			str = 0x00;
	data = (data << 8) | str;
	str = get_byte(buff);
	if(str == 0xff) 
		if(get_byte(buff) == 0x00) 
			str = 0xFF; 
		else 
			str = 0x00;
	data = (data << 8) | str;
	return data;
}

// Xử lý APP0
void GetAPP0(unsigned char *buff)
{
	unsigned short data;
	unsigned char str;
	unsigned int i;
	
	data = get_word(buff);		// Tổng chiều dài trường APP0
								// APP0 có thể bỏ qua, không cần sao chép
	for(i = 0;i < data - 2;i++)
	{
		str = get_byte(buff);
	}
#if 0
	str = get_byte(buff);		// (5 ký tự, "JFIF"[00])
	str = get_byte(buff);
	str = get_byte(buff);
	str = get_byte(buff);
	str = get_byte(buff);
	data = get_word(buff);		// Phiên bản ổn định
	str = get_byte(buff);		// Đơn vị độ phân giải
	data = get_word(buff);		// Độ phân giải theo chiều ngang
	data = get_word(buff);		// Độ phân giải theo chiều dọc
	data = get_word(buff);		// Số pixel theo chiều ngang của hình thu nhỏ
	data = get_word(buff);		// Số pixel theo chiều dọc của hình thu nhỏ
	data = get_word(buff);		// Hình thu nhỏ (chỉ trong một số trường hợp)
#endif
}

// Xử lý DQT
void GetDQT(unsigned char *buff)
{
	unsigned short data;
	unsigned char str;
	unsigned int i;
	unsigned int tablenum;

	data = get_word(buff);				// Chiều dài trường
	str = get_byte(buff); 				// Số bảng
	
	printf("*** Bảng DQT %d\n",str);
	for(i = 0;i < 64;i++)
	{
		TableDQT[str][i] = get_byte(buff);
		printf(" %2d: %2x\n",i,TableDQT[str][i]);
	}
}

// Xử lý DHT
void GetDHT(unsigned char *buff)
{
	unsigned short data;
	unsigned char str;
	unsigned int i;
	unsigned char max,count;
	unsigned short ShiftData = 0x8000,HuffmanData = 0x0000;
	unsigned int tablenum;

	data = get_word(buff);			// Chiều dài trường
	str = get_byte(buff);			// ID và loại bảng

	switch(str)
	{
		case 0x00:
			// Thành phần DC Y
			tablenum = 0x00;
			break;
		case 0x10:
			// Thành phần AC Y
			tablenum = 0x01;
			break;
		case 0x01:
			// Thành phần DC CbCr
			tablenum = 0x02;
			break;
		case 0x11:
			// Thành phần AC CbCr
			tablenum = 0x03;
			break;
	}

	printf("*** Bảng DHT/Số %d\n",tablenum);
	
	max = 0;
	for(i = 0;i < 16;i++)					// 16 byte biểu thị số lượng mã với độ dài khác nhau
	{
		count = get_byte(buff);				// 
		TableHT[tablenum][i] = HuffmanData;
		TableHN[tablenum][i] = max;
		printf(" %2d: %4x,%2x\n",i,TableHT[tablenum][i],TableHN[tablenum][i]);
		max = max + count;
		while(!(count == 0))
		{
			HuffmanData += ShiftData;
			count--;
		}
		ShiftData = ShiftData >> 1; 					// Dịch phải 1 bit
	}

	printf("*** Bảng DHT %d\n",tablenum);
	for(i=0;i<max;i++){
		TableDHT[tablenum][i] = get_byte(buff);
		printf(" %2d: %2x\n",i,TableDHT[tablenum][i]);
	}
}

// Xử lý SOF
void GetSOF(unsigned char *buff)
{
	unsigned short data;
	unsigned char str;
	unsigned int i;
	unsigned char count;
	unsigned char num;

	data = get_word(buff);						// Chiều dài dữ liệu trường
	str = get_byte(buff);						// Độ chính xác (thường là 8 bit)
	BuffY = get_word(buff); 					// Chiều cao hình ảnh
	BuffX = get_word(buff); 					// Chiều rộng hình ảnh
	CompCount = get_byte(buff); 				// Số lượng thành phần dữ liệu
	printf(" Số lượng thành phần: %d\n", CompCount);
	for(i = 0;i < CompCount;i++)
	{
		str = get_byte(buff); 					// Số thành phần
		num = str;
		printf(" Thành phần[%d]: %02X\n", i, str);
		str = get_byte(buff); 		 			// Tỷ lệ mẫu
		CompSample[num] = str;
		printf(" Mẫu[%d]: %02X\n", i, str);
		str = get_byte(buff); 					// Số bảng DQT
		CompDQT[num] = str;
		printf(" DQT[%d]: %02X\n", i, str);
	}
	
	if(CompCount == 1)
	{
		CompSampleX = 1;
		CompSampleY = 1;
	}
	else
	{
		CompSampleX =  CompSample[1]       & 0x0F; // Tỷ lệ mẫu chiều ngang, thấp bốn bit là chiều dọc
		CompSampleY = (CompSample[1] >> 4) & 0x0F; 
	}

	// Tính toán kích thước MCU (một khối)
	BuffBlockX = (int)(BuffX / (8 * CompSampleX));
	if(BuffX % (8 * CompSampleX) > 0) 
		BuffBlockX++;
	BuffBlockY = (int)(BuffY / (8 * CompSampleY));
	if(BuffY % (8 * CompSampleY) >0) 
		BuffBlockY++;
	Buff = (unsigned char*)malloc(BuffBlockY*(8 * CompSampleY)*BuffBlockX*(8 * CompSampleX)*3);

	printf(" Kích thước : %d x %d,(%d x %d)\n",BuffX,BuffY,BuffBlockX,BuffBlockY);
}

// Xử lý SOS
void GetSOS(unsigned char *buff)
{
	unsigned short data;
	unsigned char str;
	unsigned int i;
	unsigned char count;
	unsigned char num;

	data = get_word(buff);					// Chiều dài tổng trường
	count = get_byte(buff);					// Số lượng thành phần màu
	for(i = 0;i < count;i++)
	{
		str = get_byte(buff);				// Nhận ID thành phần màu
		num = str;
		printf(" CompNum[%d]: %02X\n", i, str);
		str = get_byte(buff);				// Bảng hệ số DC, AC
		CompDHT[num] = str;
		printf(" CompDHT[%d]: %02X\n", i, str);
	}
	str = get_byte(buff);			// Bắt đầu chọn quang phổ
	str = get_byte(buff);			// Kết thúc chọn quang phổ
	str = get_byte(buff);			// Chọn quang phổ
}

// Giải mã Huffman + đảo ngược lượng hóa + đảo ngược Zig-Zag
void HuffmanDecode(unsigned char *buff, unsigned char table, int *BlockData)
{
	unsigned int data;
	unsigned char zero;
	unsigned short code,huffman;
	unsigned char count =0;
	unsigned int BitData;
	unsigned int i;
	unsigned char tabledqt,tabledc,tableac,tablen;
	unsigned char ZeroCount,DataCount;
	int DataCode;

	for(i = 0;i < 64;i++) 
		BlockData[i] = 0x0;		// Đặt lại dữ liệu

	// Đặt số bảng
	if(table == 0x00)
	{
		tabledqt = 0x00;
		tabledc = 0x00;
		tableac = 0x01;
	}
	else if(table == 0x01)
	{
		tabledqt = 0x01;
		tabledc	= 0x02;
		tableac	= 0x03;
	}
	else
	{
		tabledqt = 0x01;
		tabledc	= 0x02;
		tableac	= 0x03;
	}

	count = 0; // Để phòng tránh
	while(count < 64)
	{
		// Khi vị trí đếm bit vượt quá 32, nhận dữ liệu mới.
		if(BitCount >= 32)
		{
			LineData = NextData;
			NextData = get_data(buff);
			BitCount -= 32;
		}
		// Thay thế dữ liệu giải mã Huffman
		if(BitCount > 0)
		{
			BitData = (LineData << BitCount) | (NextData >> (32 - BitCount));
		}
		else
		{
			BitData = LineData;
		}
		//printf(" Dữ liệu Bit Huffman(%2d,%2d): %8x\n",table,count,BitData);

		// Chọn bảng sử dụng
		if(count == 0) 
			tablen = tabledc;
		else 
			tablen = tableac;
		code = (unsigned short)(BitData >> 16);			// Sử dụng 16 bit
		// Tìm mã Huffman ở đâu
		for(i = 0;i < 16;i++) 
		{
		//	printf(" Hit Huffman(%2d:%2d): %8x,%8x\n",table,i,TableHT[tablen][i],code);
			if(TableHT[tablen][i] > code) 
				break;
		}
		i--;

		code	= (unsigned short)(code >> (15 - i));	// Căn chỉnh mã xuống
		huffman = (unsigned short)(TableHT[tablen][i] >> (15 - i));

		// printf(" Số Dht Sử Dụng(%2d): %8x,%8x,%8x\n",i,code,huffman,TableHN[tablen][i]);

		// Tính toán vị trí bảng Huffman
        // printf(" PreUse Dht Number(%2d): %8x,%8x,%8x\n",i,code,huffman,TableHN[tablen][i]);

        // Tính toán vị trí của bảng huffman
        code = code - huffman + TableHN[tablen][i];

        // printf(" Use Dht Number: %8x\n",code);

        ZeroCount = (TableDHT[tablen][code] >> 4) & 0x0F; // Số lượng số không được giữ lại
        DataCount = (TableDHT[tablen][code]) & 0x0F;      // Độ dài bit của dữ liệu tiếp theo
        // printf(" Dht Table: %8x,%8x\n",ZeroCount,DataCount);

        // Bỏ qua mã huffman, lấy dữ liệu
        DataCode = (BitData << (i + 1)) >> (16 + (16 - DataCount));

        // Nếu bit đầu tiên là "0", thì dữ liệu là số âm, số dương tương ứng được lấy đối
        if(!(DataCode & (1<<(DataCount-1))) && DataCount !=0)
        {
            DataCode |= (~0) << DataCount;
            DataCode += 1;
        }

        //printf(" Use Bit: %d\n",(i + DataCount +1)); // Xuất số bit đã sử dụng (số bit mã + số bit dữ liệu)
        BitCount += (i + DataCount + 1); // Thêm vào số bit đã sử dụng

        if(count == 0)
        {
            // Trong trường hợp thành phần C, trở thành dữ liệu.
            if(DataCount == 0) 
                DataCode = 0x0; // Nếu DataCount bằng 0, thì dữ liệu là 0.
            PreData[table] += DataCode; // Thành phần DC phải cộng lại
            // Ngược lượng hóa + z
            BlockData[zigzag_table[count]] = PreData[table] * TableDQT[tabledqt][count];
            count++;
        }
        else
        {
            if(ZeroCount == 0x0 && DataCount == 0x0)            
            {                                            
                break; // Kết thúc khi gặp ký hiệu EOB trong thành phần AC
            }
            else if(ZeroCount == 0xF && DataCount == 0x0)
            {                                    
                count += 15; // Nếu mã ZRL đến, coi như 15 dữ liệu số không
            }
            else
            {
                count += ZeroCount;
                // Ngược lượng hóa + z
                BlockData[zigzag_table[count]] = DataCode * TableDQT[tabledqt][count];
            }
            count++;
        }
    }
}

const int C1_16 = 4017; // cos( pi/16) x4096
const int C2_16 = 3784; // cos(2pi/16) x4096
const int C3_16 = 3406; // cos(3pi/16) x4096
const int C4_16 = 2896; // cos(4pi/16) x4096
const int C5_16 = 2276; // cos(5pi/16) x4096
const int C6_16 = 1567; // cos(6pi/16) x4096
const int C7_16 = 799;  // cos(7pi/16) x4096

// Ngược DCT
void DctDecode(int *BlockIn, int *BlockOut)
{
    int i;
    int s0,s1,s2,s3,s4,s5,s6,s7;
    int t0,t1,t2,t3,t4,t5,t6,t7;

    /*
    printf("-----------------------------\n");
    printf(" iDCT(In)\n");
    printf("-----------------------------\n");
    for(i=0;i<64;i++){
        printf("%2d: %8x\n",i,BlockIn[i]);
    }
    */

    for(i = 0; i < 8; i++) 
    {
        s0 = (BlockIn[0] + BlockIn[4]) * C4_16;
        s1 = (BlockIn[0] - BlockIn[4]) * C4_16;
        s3 = (BlockIn[2] * C2_16) + (BlockIn[6] * C6_16);
        s2 = (BlockIn[2] * C6_16) - (BlockIn[6] * C2_16);

        s7 = (BlockIn[1] * C1_16) + (BlockIn[7] * C7_16);
        s4 = (BlockIn[1] * C7_16) - (BlockIn[7] * C1_16);
        s6 = (BlockIn[5] * C5_16) + (BlockIn[3] * C3_16);
        s5 = (BlockIn[5] * C3_16) - (BlockIn[3] * C5_16);

        /*
        printf("s0:%8x\n",s0);
        printf("s1:%8x\n",s1);
        printf("s2:%8x\n",s2);
        printf("s3:%8x\n",s3);
        printf("s4:%8x\n",s4);
        printf("s5:%8x\n",s5);
        printf("s6:%8x\n",s6);
        printf("s7:%8x\n",s7);
        */

        t0 = s0 + s3;
        t1 = s1 + s2;
        t3 = s0 - s3;
        t2 = s1 - s2;

        t4 = s4 + s5;
        t5 = s4 - s5;
        t7 = s7 + s6;
        t6 = s7 - s6;

        /*	
        printf("t0:%8x\n",t0);
        printf("t1:%8x\n",t1);
        printf("t2:%8x\n",t2);
        printf("t3:%8x\n",t3);
        printf("t4:%8x\n",t4);
        printf("t5:%8x\n",t5);
        printf("t6:%8x\n",t6);
        printf("t7:%8x\n",t7);
        */

        s6 = (t5 + t6) * 181 / 256; // 1/sqrt(2)
        s5 = (t6 - t5) * 181 / 256; // 1/sqrt(2)

        /*	
        printf("s5:%8x\n",s5);
        printf("s6:%8x\n",s6);
        */

        *BlockIn++ = (t0 + t7) >> 11;
        *BlockIn++ = (t1 + s6) >> 11;
        *BlockIn++ = (t2 + s5) >> 11;
        *BlockIn++ = (t3 + t4) >> 11;
        *BlockIn++ = (t3 - t4) >> 11;
        *BlockIn++ = (t2 - s5) >> 11;
        *BlockIn++ = (t1 - s6) >> 11;
        *BlockIn++ = (t0 - t7) >> 11;
    }

    BlockIn -= 64;

    /*
    printf("-----------------------------\n");
    printf(" iDCT(Middle)\n");
    printf("-----------------------------\n");
    for(i=0;i<64;i++){
        printf("%2d: %8x\n",i,BlockIn[i]);
    }
    */

    for(i = 0; i < 8; i++)
    {
        s0 = (BlockIn[0] + BlockIn[32]) * C4_16;
        s1 = (BlockIn[0] - BlockIn[32]) * C4_16;
        s3 = BlockIn[16] * C2_16 + BlockIn[48] * C6_16;
        s2 = BlockIn[16] * C6_16 - BlockIn[48] * C2_16;
        s7 = BlockIn[8] * C1_16 + BlockIn[56] * C7_16;
        s4 = BlockIn[8] * C7_16 - BlockIn[56] * C1_16;
        s6 = BlockIn[40] * C5_16 + BlockIn[24] * C3_16;
        s5 = BlockIn[40] * C3_16 - BlockIn[24] * C5_16;

        /*
        printf("s0:%8x\n",s0);
        printf("s1:%8x\n",s1);
        printf("s2:%8x\n",s2);
        printf("s3:%8x\n",s3);
        printf("s4:%8x\n",s4);
        printf("s5:%8x\n",s5);
        printf("s6:%8x\n",s6);
        printf("s7:%8x\n",s7);
        */

        t0 = s0 + s3;
        t1 = s1 + s2;
        t2 = s1 - s2;
        t3 = s0 - s3;
        t4 = s4 + s5;
        t5 = s4 - s5;
        t6 = s7 - s6;
        t7 = s6 + s7;

        /*
        printf("t0:%8x\n",t0);
        printf("t1:%8x\n",t1);
        printf("t2:%8x\n",t2);
        printf("t3:%8x\n",t3);
        printf("t4:%8x\n",t4);
        printf("t5:%8x\n",t5);
        printf("t6:%8x\n",t6);
        printf("t7:%8x\n",t7);
        */

        s5 = (t6 - t5) * 181 / 256; // 1/sqrt(2)
        s6 = (t5 + t6) * 181 / 256; // 1/sqrt(2)

        /*
        printf("s5:%8x\n",s5);
        printf("s6:%8x\n",s6);
        */

        BlockOut[0] = ((t0 + t7) >> 15);
        BlockOut[56] = ((t0 - t7) >> 15);
        BlockOut[8] = ((t1 + s6) >> 15);
        BlockOut[48] = ((t1 - s6) >> 15);
        BlockOut[16] = ((t2 + s5) >> 15);
        BlockOut[40] = ((t2 - s5) >> 15);
        BlockOut[24] = ((t3 + t4) >> 15);
        BlockOut[32] = ((t3 - t4) >> 15);
        
        BlockIn++;
        BlockOut++;
    }
    BlockOut -= 8;
    /*
    printf("-----------------------------\n");
    printf(" iDCT(Out)\n");
    printf("-----------------------------\n");
    for(i=0;i<8;i++){
        printf(" %2d: %04x;\n",i+ 0,BlockOut[i+ 0]&0xFFFF);
        printf(" %2d: %04x;\n",i+56,BlockOut[i+56]&0xFFFF);
        printf(" %2d: %04x;\n",i+ 8,BlockOut[i+ 8]&0xFFFF);
        printf(" %2d: %04x;\n",i+48,BlockOut[i+48]&0xFFFF);
        printf(" %2d: %04x;\n",i+16,BlockOut[i+16]&0xFFFF);
        printf(" %2d: %04x;\n",i+40,BlockOut[i+40]&0xFFFF);
        printf(" %2d: %04x;\n",i+24,BlockOut[i+24]&0xFFFF);
        printf(" %2d: %04x;\n",i+32,BlockOut[i+32]&0xFFFF);
    }
    */
}

// Giải mã 4:1:1
void Decode411(unsigned char *buff, int *BlockY, int *BlockCb, int *BlockCr)
{
    int BlockHuffman[64];
    int BlockYLT[64];
    int BlockYRT[64];
    int BlockYLB[64];
    int BlockYRB[64];
    int Block[64];
    unsigned int i;
    unsigned int m, n;

    for(n = 0; n < CompSampleY; n++)
    {
        for(m = 0; m < CompSampleX; m++)
        {
            //printf("BlockY:%d,%d\n",m,n);
            HuffmanDecode(buff, 0x00, BlockHuffman);
            DctDecode(BlockHuffman, Block);
            for(i = 0; i < 64; i++)
            {
                BlockY[(int)(i/8)*8*CompSampleX + (i%8) + (m*8) + (n*64*CompSampleY)] = Block[i];
            }
        }
    }

    if(CompCount > 1)
    {
        // Khác biệt màu xanh
        //printf("Block:10\n");
        HuffmanDecode(buff, 0x01, BlockHuffman);
        DctDecode(BlockHuffman, BlockCb);

        // Khác biệt màu đỏ
        //printf("Block:11\n");
        HuffmanDecode(buff, 0x02, BlockHuffman);
        DctDecode(BlockHuffman, BlockCr);
    }
}

// Chuyển đổi YUV→RGB
void DecodeYUV(int *y, int *cb, int *cr, unsigned char *rgb)
{
    int r, g, b;
    int p, i;

    //printf("----RGB----\n");
    for(i = 0; i < ((CompCount > 1) ? 256 : 64); i++)
    {
        p = ((int)(i / 32) * 8) + ((int)((i % 16) / 2));

        r = 128 + y[i] + ((CompCount > 1) ? cr[p] * 1.402 : 0);
        //r = (r & 0xffffff00) ? (r >> 24) ^ 0xff : r;
        if (r >= 255)
            r = 255;
        else if (r <= 0)
            r = 0;

        g = 128 + y[i] - ((CompCount > 1) ? cb[p] * 0.34414 : 0) - ((CompCount > 1) ? cr[p] * 0.71414 : 0);
        //g = (g & 0xffffff00) ? (g >> 24) ^ 0xff : g;
        if (g >= 255)
            g = 255;
        else if (g <= 0)
            g = 0;

        b = 128 + y[i] + ((CompCount > 1) ? cb[p] * 1.772 : 0);
        //b = (b & 0xffffff00) ? (b >> 24) ^ 0xff : b;
        if (b >= 255)
            b = 255;
        else if (b <= 0)
            b = 0;

        rgb[i * 3 + 0] = b;
        rgb[i * 3 + 1] = g;
        rgb[i * 3 + 2] = r;
        /*	
        printf("[RGB]%3d: %3x,%3x,%3x = %2x,%2x,%2x\n", i,
            y[i] & 0x1FF, cr[p] & 0x1FF, cb[p] & 0x1FF,
            rgb[i * 3 + 2], rgb[i * 3 + 1], rgb[i * 3 + 0]);
        */
    }
}

// Giải mã hình ảnh
void Decode(unsigned char *buff, unsigned char *rgb)
{
    int BlockY[256];
    int BlockCb[256];
    int BlockCr[256];
    int x, y, i, p;
    for(y = 0; y < BuffBlockY; y++)
    {
        for(x = 0; x < BuffBlockX; x++)
        {
            Decode411(buff, BlockY, BlockCb, BlockCr); // Giải mã 4:1:1
            DecodeYUV(BlockY, BlockCb, BlockCr, rgb); // Chuyển đổi YUV-RGB
            for(i = 0; i < ((CompCount > 1) ? 256 : 64); i++)
            {
                if((x * (8 * CompSampleX) + (i % (8 * CompSampleX)) < BuffX) && 
                   (y * (8 * CompSampleY) + i / (8 * CompSampleY) < BuffY))
                {
                    p = y * (8 * CompSampleY) * BuffX * 3 + x * (8 * CompSampleX) * 3 + 
                        (int)(i / (8 * CompSampleY)) * BuffX * 3 + (i % (8 * CompSampleX)) * 3;
                    Buff[p + 0] = rgb[i * 3 + 0];
                    Buff[p + 1] = rgb[i * 3 + 1];
                    Buff[p + 2] = rgb[i * 3 + 2];

                    //printf("RGB[%4d,%4d]: %2x,%2x,%2x\n", x * (8 * CompSampleX) + (i % (8 * CompSampleX)), y * (8 * CompSampleY) + i / (8 * CompSampleY), rgb[i * 3 + 2], rgb[i * 3 + 1], rgb[i * 3 + 0]);
                }
            }
        }
    }
}

void JpegDecode(unsigned char *buff)
{
    unsigned short data;
    unsigned int i;
    unsigned int Image = 0;
    unsigned char RGB[256 * 3];
    while(!(BuffIndex >= BuffSize))
    {
        if(Image == 0)
        {
            data = get_word(buff);
            switch(data)
            {
                case 0xFFD8: // SOI
                    printf("Header: SOI\n");
                    break;
                case 0xFFE0: // APP0
                    printf("Header: APP0\n");
                    GetAPP0(buff);
                    break;
                case 0xFFDB: // DQT
                    printf("Header: DQT\n");
                    GetDQT(buff);
                    break;
                case 0xFFC4: // DHT
                    printf("Header: DHT\n");
                    GetDHT(buff);
                    break;
                case 0xFFC0: // SOF
                    printf("Header: SOF\n");
                    GetSOF(buff);
                    break;
                case 0xFFDA: // SOS
                    printf("Header: SOS\n");
                    GetSOS(buff);
                    Image = 1;
                    // Chuẩn bị dữ liệu
                    PreData[0] = 0x00;
                    PreData[1] = 0x00;
                    PreData[2] = 0x00;
                    LineData = get_data(buff); // Lấy dữ liệu hình ảnh 32 bit
                    NextData = get_data(buff);
                    BitCount = 0;
                    break;
                case 0xFFD9: // EOI
                    printf("Header: EOI\n");
                    break;
                default:
                    // Xác định đọc không đầy đủ tiêu đề
                    printf("Header: other(%X)\n", data);
                    if((data & 0xFF00) == 0xFF00 && !(data == 0xFF00))
                    {
                        data = get_word(buff);
                        for(i = 0; i < data - 2; i++)
                        {
                            get_byte(buff);
                        }
                    }
                    break;
            }
        }
        else
        {
            // Giải mã (SOS đã đến)
            printf("/****Image****/\n");
            Decode(buff, RGB);
        }
    }
}

// Hàm chính
int main(int argc, char* argv[])
{
    unsigned char *buff;
    FILE *fp;

    // Xuất kích thước của các loại dữ liệu
    printf(" sizeof(char):           %02d\n", sizeof(char));
    printf(" sizeof(unsigned char):  %02d\n", sizeof(unsigned char));
    printf(" sizeof(short):          %02d\n", sizeof(short));
    printf(" sizeof(unsigned short): %02d\n", sizeof(unsigned short));
    printf(" sizeof(int):            %02d\n", sizeof(int));
    printf(" sizeof(unsigned int):   %02d\n", sizeof(unsigned int));
    printf(" sizeof(long):           %02d\n", sizeof(long));
    printf(" sizeof(unsigned long):  %02d\n", sizeof(unsigned long));

    printf(" sizeof:                 %02d\n", sizeof(BITMAPFILEHEADER));
    printf(" sizeof:                 %02d\n", sizeof(BITMAPINFOHEADER));

    if((fp = fopen(argv[1], "rb")) == NULL) // Mở ảnh JPG để giải mã
    {
        // perror(s) dùng để in ra lý do lỗi của hàm trước đó ra stderr
        // Chuỗi s sẽ được in ra trước, sau đó là lý do lỗi
        perror(0);
        
        // exit(1) biểu thị quá trình thoát bình thường, trả về 1
        // exit(0) biểu thị quá trình thoát không bình thường, trả về 0
        exit(0);
    }

    BuffSize = 0; // Lấy kích thước file
    while(!feof(fp)) // Kiểm tra kết thúc file
    { 
        fgetc(fp); // Đọc một ký tự từ file
        BuffSize++;
    }
    BuffSize--;
    rewind(fp); // Đặt con trỏ file về đầu

    buff = (unsigned char *)malloc(BuffSize); // Cấp phát bộ nhớ cùng kích thước để lưu dữ liệu hình ảnh
    fread(buff, 1, BuffSize, fp); // Đọc dữ liệu hình ảnh vào bộ nhớ đã cấp phát
    BuffIndex = 0;

    JpegDecode(buff); // Giải mã JPEG

    printf("Finished decode\n");

    BmpSave(argv[2], Buff, BuffX, BuffY, 3); // Lưu Bitmap

    printf("Saved BMP\n");

    fclose(fp);
    free(buff);

    return 0;
}