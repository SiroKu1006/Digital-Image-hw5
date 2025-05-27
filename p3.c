#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fft.h" 

typedef unsigned char U_CHAR;
typedef long INT32;
typedef unsigned short INT16;

#define GET_2B(a,off) ((INT16)(a[off]) + (((INT16)(a[off+1]))<<8))
#define GET_4B(a,off) ((INT32)(a[off]) + (((INT32)(a[off+1]))<<8) + \
                       (((INT32)(a[off+2]))<<16) + (((INT32)(a[off+3]))<<24))

int ReadDataSize(const char *name);
void ReadImageData(const char *name,
    U_CHAR *filehdr, U_CHAR *infohdr, U_CHAR *colortbl, U_CHAR *data);

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s in.bmp out.bmp Q\n", argv[0]);
        return 1;
    }
    const char *infile  = argv[1];
    const char *outfile = argv[2];
    double Q = atof(argv[3]);

    /* 1. 讀取 BMP */
    U_CHAR filehdr[14], infohdr[40], colortbl[1024];
    int datasz = ReadDataSize(infile);
    U_CHAR *data = malloc(datasz);
    ReadImageData(infile, filehdr, infohdr, colortbl, data);

    /* 2. 取得影像尺寸 */
    int W = GET_4B(infohdr,4);
    int H = GET_4B(infohdr,8);
    int padW = ((W+3)/4)*4;  // 每列對齊

    /* 3. 建立輸出緩衝區 */
    U_CHAR *out = malloc(datasz);

    /* 4. 3×3 Contraharmonic Mean Filter */
    for(int y=0; y<H; y++){
        for(int x=0; x<W; x++){
            double num = 0.0, den = 0.0;
            for(int dy=-1; dy<=1; dy++){
                for(int dx=-1; dx<=1; dx++){
                    int yy = y+dy, xx = x+dx;
                    /* 邊界重複取最接近的像素 */
                    if (yy<0) yy=0;
                    if (yy>=H) yy=H-1;
                    if (xx<0) xx=0;
                    if (xx>=W) xx=W-1;
                    int idx = yy*padW + xx;
                    double g = data[idx];
                    num += pow(g, Q+1);
                    den += pow(g, Q);
                }
            }
            double v = (den==0.0) ? 0.0 : num/den;
            int iv = (int)(v+0.5);
            if (iv<0) iv=0; if (iv>255) iv=255;
            out[y*padW + x] = (U_CHAR)iv;
        }
        /* 填補每行多餘 padding 部分 */
        for(int x=W; x<padW; x++){
            out[y*padW + x] = 0;
        }
    }

    /* 5. 寫出 BMP */
    FILE *fp = fopen(outfile, "wb");
    if (!fp) { perror("fopen"); return 1; }
    fwrite(filehdr,1,14,fp);
    fwrite(infohdr,1,40,fp);
    fwrite(colortbl,1,1024,fp);
    fwrite(out,1,datasz,fp);
    fclose(fp);

    /* 6. 釋放 */
    free(data);
    free(out);
    return 0;
}

/* 以下函式與你之前的 BMP 讀取程式相同 */
int ReadDataSize(const char *name)
{
    FILE *f = fopen(name,"rb");
    if (!f) { perror("fopen"); exit(1); }
    U_CHAR h[14], i[40];
    fread(h,1,14,f);
    fread(i,1,40,f);
    if (GET_2B(h,0)!=0x4D42) { fprintf(stderr,"Not BMP\n"); exit(1); }
    INT32 w = GET_4B(i,4), hgt = GET_4B(i,8);
    INT16 b = GET_2B(i,14);
    if (b!=8) { fprintf(stderr,"Only 8-bit BMP\n"); exit(1); }
    fclose(f);
    return ((w+3)/4)*4 * hgt;
}

void ReadImageData(const char *name,
    U_CHAR *filehdr, U_CHAR *infohdr, U_CHAR *colortbl, U_CHAR *data)
{
    FILE *f = fopen(name,"rb");
    if (!f) { perror("fopen"); exit(1); }
    INT32 off;
    fread(filehdr,1,14,f);
    fread(infohdr,1,40,f);
    off = GET_4B(filehdr,10);
    fread(colortbl,1,1024,f);
    fseek(f,off,SEEK_SET);
    fread(data,1,ReadDataSize(name),f);
    fclose(f);
}