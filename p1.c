#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fft.h"

typedef unsigned char U_CHAR;
typedef long INT32;
typedef unsigned short INT16;

#define GET_2B(a,off) ((INT16)(a[off]) + (((INT16)(a[off+1]))<<8))
#define GET_4B(a,off) ((INT32)(a[off]) + (((INT32)(a[off+1]))<<8) + (((INT32)(a[off+2]))<<16) + (((INT32)(a[off+3]))<<24))
#define PI 3.141592653589793

int ReadDataSize(const char *name);
void ReadImageData(const char *name, U_CHAR *filehdr, U_CHAR *infohdr, U_CHAR *colortbl, U_CHAR *data);

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input.bmp output.bmp D0\n", argv[0]);
        return 1;
    }
    const char *infile  = argv[1];
    const char *outfile = argv[2];
    int D0 = atoi(argv[3]);

    /* 1. 讀取 BMP 檔頭與影像資料 */
    U_CHAR filehdr[14], infohdr[40], colortbl[1024];
    int datasz = ReadDataSize(infile);
    U_CHAR *data = malloc(datasz);
    if (!data) { perror("malloc"); return 1; }
    ReadImageData(infile, filehdr, infohdr, colortbl, data);

    /* 2. 轉為 Complex 陣列並做中心化 */
    int width  = GET_4B(infohdr, 4);
    int height = GET_4B(infohdr, 8);
    int padW   = ((width+3)/4)*4;
    COMPLEX *F = malloc(sizeof(COMPLEX)*padW*height);
    if (!F) { perror("malloc"); return 1; }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = i*padW + j;
            double val = data[idx];
            if ((i + j) % 2) val = -val;  // 去 DC 分量中心化
            F[idx].real = val;
            F[idx].imag = 0.0;
        }
    }

    /* 3. Forward FFT */
    if (!FFT2D(F, height, padW, 1)) {
        fprintf(stderr, "FFT2D forward failed\n");
        return 1;
    }

    /* 4. 套用理想低通濾波器：H(u,v)=1 if D(u,v)<=D0 */
    int u0 = height/2, v0 = padW/2;
    for (int u = 0; u < height; u++) {
        int du = u - u0;
        for (int v = 0; v < padW; v++) {
            int dv = v - v0;
            double D = sqrt(du*du + dv*dv);
            if (D > D0) {
                int idx = u*padW + v;
                F[idx].real = F[idx].imag = 0.0;
            }
        }
    }

    /* 5. Inverse FFT */
    if (!FFT2D(F, height, padW, -1)) {
        fprintf(stderr, "FFT2D inverse failed\n");
        return 1;
    }

    /* 6. 去中心化並轉回 8-bit */
    U_CHAR *out = malloc(datasz);
    if (!out) { perror("malloc"); return 1; }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = i*padW + j;
            double v = F[idx].real;
            if ((i + j) % 2) v = -v;
            int iv = (int)(v + 0.5);
            if (iv < 0) iv = 0;
            if (iv > 255) iv = 255;
            out[idx] = (U_CHAR)iv;
        }
    }

    /* 7. 寫出 BMP */
    FILE *fp = fopen(outfile, "wb");
    if (!fp) { perror("fopen"); return 1; }
    fwrite(filehdr, 1, 14, fp);
    fwrite(infohdr, 1, 40, fp);
    fwrite(colortbl, 1, 1024, fp);
    fwrite(out, 1, datasz, fp);
    fclose(fp);

    /* 8. 釋放資源 */
    free(data);
    free(F);
    free(out);
    return 0;
}

/* 以下兩函式與你的範例相同，用以讀取 BMP */
int ReadDataSize(const char *name)
{
    FILE *f = fopen(name, "rb");
    if (!f) { perror("fopen"); exit(1); }
    U_CHAR hdr[14], info[40];
    fread(hdr, 1, 14, f);
    fread(info,1,40,f);
    if (GET_2B(hdr,0)!=0x4D42) { fprintf(stderr,"Not BMP\n"); exit(1); }
    INT32 w = GET_4B(info,4), h = GET_4B(info,8);
    INT16 b = GET_2B(info,14);
    if (b!=8) { fprintf(stderr,"Only 8-bit BMP\n"); exit(1); }
    fclose(f);
    return ((w+3)/4*4)*h;
}

void ReadImageData(const char *name, U_CHAR *filehdr, U_CHAR *infohdr, U_CHAR *colortbl, U_CHAR *data)
{
    FILE *f = fopen(name,"rb");
    if (!f) { perror("fopen"); exit(1); }
    INT32 bfOffBits;
    fread(filehdr,1,14,f);
    fread(infohdr,1,40,f);
    bfOffBits = GET_4B(filehdr,10);
    fread(colortbl,1,1024,f);
    fseek(f, bfOffBits, SEEK_SET);
    int w = GET_4B(infohdr,4), h = GET_4B(infohdr,8);
    fread(data,1,((w+3)/4*4)*h,f);
    fclose(f);
}
