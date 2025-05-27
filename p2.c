/* butterworthNotch.c
 * Usage: ./butterworthNotch input.bmp output.bmp D0 n uk vk [uk2 vk2 ...]
 *   input.bmp   : 要處理的 8-bit BMP（Moiré 圖）
 *   output.bmp  : 濾波後輸出檔名
 *   D0          : 截止頻率
 *   n           : Butterworth 濾波階數
 *   uk vk       : Notch 中心座標（相對於 spectrum 中心的偏移量）
 *                 可以輸入多組，不同干擾頻率成對出現
 */

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
    if (argc < 7 || ((argc-5)%2)!=0) {
        fprintf(stderr, "Usage: %s in.bmp out.bmp D0 n uk vk [uk2 vk2 ...]\n", argv[0]);
        return 1;
    }
    const char *infile  = argv[1];
    const char *outfile = argv[2];
    double D0 = atof(argv[3]);
    int n = atoi(argv[4]);
    int notchCount = (argc - 5)/2;
    double *uk = malloc(sizeof(double)*notchCount);
    double *vk = malloc(sizeof(double)*notchCount);
    for (int i = 0; i < notchCount; i++) {
        uk[i] = atof(argv[5 + 2*i]);
        vk[i] = atof(argv[5 + 2*i+1]);
    }

    /* 1. 讀取 BMP */
    U_CHAR filehdr[14], infohdr[40], colortbl[1024];
    int datasz = ReadDataSize(infile);
    U_CHAR *data = malloc(datasz);
    ReadImageData(infile, filehdr, infohdr, colortbl, data);

    /* 2. 建立 Complex 陣列並中心化 */
    int W = GET_4B(infohdr,4), H = GET_4B(infohdr,8);
    int padW = ((W+3)/4)*4;
    COMPLEX *F = malloc(sizeof(COMPLEX)*padW*H);
    for (int y=0; y<H; y++){
      for(int x=0; x<padW; x++){
        int idx = y*padW + x;
        double v = (x<W && y<H) ? data[idx] : 0;
        if ((x+y)%2) v = -v;
        F[idx].real = v;  F[idx].imag = 0;
      }
    }

    /* 3. Forward FFT */
    FFT2D(F, H, padW, 1);

    /* 4. 套用 Butterworth Notch Reject Filter */
    int u0 = H/2, v0 = padW/2;
    for (int u=0; u<H; u++){
      for (int v=0; v<padW; v++){
        int idx = u*padW + v;
        double Huv = 1.0;
        for (int k=0; k<notchCount; k++){
          // 對稱的兩個 notch at (u0+uk, v0+vk) 與 (u0-uk, v0-vk)
          double du1 = u - (u0 + uk[k]), dv1 = v - (v0 + vk[k]);
          double D1 = sqrt(du1*du1 + dv1*dv1);
          double du2 = u - (u0 - uk[k]), dv2 = v - (v0 - vk[k]);
          double D2 = sqrt(du2*du2 + dv2*dv2);
          Huv *= (1.0 / (1.0 + pow(D0/D1, 2*n)))
               * (1.0 / (1.0 + pow(D0/D2, 2*n)));
        }
        F[idx].real *= Huv;
        F[idx].imag *= Huv;
      }
    }

    /* 5. Inverse FFT */
    FFT2D(F, H, padW, -1);

    /* 6. 去中心、量化輸出 */
    U_CHAR *out = malloc(datasz);
    for (int y=0; y<H; y++){
      for(int x=0; x<W; x++){
        int idx = y*padW + x;
        double v = F[idx].real;
        if ((x+y)%2) v = -v;
        int iv = (int)(v + 0.5);
        if (iv<0) iv=0; if (iv>255) iv=255;
        out[idx] = iv;
      }
    }

    /* 7. 寫檔 */
    FILE *fp = fopen(outfile,"wb");
    fwrite(filehdr,1,14,fp);
    fwrite(infohdr,1,40,fp);
    fwrite(colortbl,1,1024,fp);
    fwrite(out,1,datasz,fp);
    fclose(fp);

    return 0;
}

/* BMP 讀檔函式同前例 */
int ReadDataSize(const char *name) {
    FILE *f=fopen(name,"rb"); U_CHAR h[14],i[40]; fread(h,1,14,f); fread(i,1,40,f);
    INT16 b=GET_2B(i,14); if (b!=8) { fprintf(stderr,"Only 8-bit BMP\n"); exit(1);}
    INT32 w=GET_4B(i,4), hgt=GET_4B(i,8); fclose(f);
    return ((w+3)/4*4)*hgt;
}
void ReadImageData(const char *name,U_CHAR *fh,U_CHAR *ih,U_CHAR *ct,U_CHAR *d){
    FILE *f=fopen(name,"rb"); INT32 off; fread(fh,1,14,f); fread(ih,1,40,f);
    off=GET_4B(fh,10); fread(ct,1,1024,f); fseek(f,off,SEEK_SET);
    INT32 w=GET_4B(ih,4),h=GET_4B(ih,8); fread(d,1,((w+3)/4*4)*h,f); fclose(f);
}
