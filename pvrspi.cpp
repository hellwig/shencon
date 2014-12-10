#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pvrspi.h"

typedef struct {
	unsigned char  id[4];
	unsigned long length;
	unsigned char  type[4];
	unsigned short width;
	unsigned short height;
} PVRHDR;

#define	SKIP_GBIX(data) \
	if (memcmp(data,"GBIX",4)==0) {\
		long size = ((long*)data)[1]; \
		data+=size+8; \
	}

enum {ARGB1555,RGB565,ARGB4444,YUV422,BUMP,PAL4,PAL8};

int pvr2binhdr(int &width, int &height, int &img_size,int &bits, PVRHDR *hdr);
int pvr2bindata(unsigned char *pbin,const PVRHDR *hdr);

int pvr2bin(const unsigned char *buf,unsigned char *&res, int &width, int &height, int &img_size, int &bits)
{
	const unsigned char *data;
	PVRHDR *hdr;

	data = buf;
	SKIP_GBIX(data);
	if (memcmp(data,"PVRT",4)) return -1;
	hdr = (PVRHDR*)data;

	pvr2binhdr(width, height, img_size, bits, hdr);
	res = (unsigned char *)malloc(img_size);
	pvr2bindata(res,hdr);

	return 0;
}

static int calc_n(int n)
{
	int sum = 1;
	while(n) {
		n>>=1;
		sum +=n*n;
	}
	return sum;
}

typedef struct {
	int rmask, rmult, radd, rshift;
	int gmask, gmult, gadd, gshift;
	int bmask, bmult, badd, bshift;
	int amask, amult, aadd, ashift;
} SHIFTTBL;

const static SHIFTTBL shifttbl[]= {
	{ /* ARGB1555 */
		0x7c00, 527, 23<<10, 16,
		0x03e0, 527, 23<<5, 11,
		0x001f, 527, 23, 6,
		0x8000, 0x1ff, 0, 16
	},{
		/* RGB565 */
		0xf800, 527, 23<<11, 17,
		0x07e0, 259, 33<<5, 11,
		0x001f, 527, 23, 6,
		0x0000, 0, 0xff, 0
	},{
		/* ARGB4444 */
		0x0f00, 1088, 32<<8, 14,
		0x00f0, 1088, 32<<4, 10,
		0x000f, 1088, 32, 6,
		0xf000, 1088, 32<<12, 18
	}
};

static unsigned long twiddle(unsigned long xSize, unsigned long ySize, unsigned long xPos, unsigned long yPos)
{
	//Initially assume X is the larger size.
	unsigned long minDimension=xSize;
	unsigned long maxValue=yPos;
	unsigned long twiddled=0;
	unsigned long srcBitPos=1;
	unsigned long dstBitPos=1;
	int shiftCount=0;

	//If Y is the larger dimension - switch the min/max values.
	if(xSize > ySize) {
		minDimension = ySize;
		maxValue	 = xPos;
	}

	// Step through all the bits in the "minimum" dimension
	while(srcBitPos < minDimension) {
		if(yPos & srcBitPos) twiddled |= dstBitPos;
		if(xPos & srcBitPos) twiddled |= (dstBitPos << 1);
		srcBitPos <<= 1;
		dstBitPos <<= 2;
		shiftCount += 1;
	}

	// Prepend any unused bits
	maxValue >>= shiftCount;
	twiddled |=  (maxValue << (2*shiftCount));

	return twiddled;
}

enum {R,G,B,A};

int decode_small_vq(unsigned char *out,const unsigned char *in0,const unsigned char *in1,int width,int height,int mode)
{
	unsigned char vqtab[4*4*16];
	const unsigned char *in;

	int x,y,wbyte;
	const SHIFTTBL *s;

	const unsigned short *in_w = (unsigned short *) in1;
	unsigned char *p = vqtab;

	s = &shifttbl[mode];
	for(x=0;x<16*4;x++) {
		int c = in_w[x];
		p[R] = ((c & s->rmask) * s->rmult + s->radd) >> s->rshift;
		p[G] = ((c & s->gmask) * s->gmult + s->gadd) >> s->gshift;
		p[B] = ((c & s->bmask) * s->bmult + s->badd) >> s->bshift;
		p[A] = ((c & s->amask) * s->amult + s->aadd) >> s->ashift;
		p+=4;
	}

	in = in0;
	wbyte = width*4;
	for(y=0;y<height;y+=4) {
		unsigned char *p = out+(height-y-1)*wbyte;
		for(x=0;x<width;x+=2) {
			int c = in[twiddle(width/2,height/4,x/2,y/4)];
			unsigned char *vq;

			vq = &vqtab[(c&15)*16];

			p[0] = vq[0];
			p[1] = vq[1];
			p[2] = vq[2];
			p[3] = vq[3];

			p[4] = vq[8];
			p[5] = vq[9];
			p[6] = vq[10];
			p[7] = vq[11];

			p[0-wbyte] = vq[4];
			p[1-wbyte] = vq[5];
			p[2-wbyte] = vq[6];
			p[3-wbyte] = vq[7];

			p[4-wbyte] = vq[12];
			p[5-wbyte] = vq[13];
			p[6-wbyte] = vq[14];
			p[7-wbyte] = vq[15];

			vq = &vqtab[(c>>4)*16];
			p-=wbyte*2;

			p[0] = vq[0];
			p[1] = vq[1];
			p[2] = vq[2];
			p[3] = vq[3];

			p[4] = vq[8];
			p[5] = vq[9];
			p[6] = vq[10];
			p[7] = vq[11];

			p[0-wbyte] = vq[4];
			p[1-wbyte] = vq[5];
			p[2-wbyte] = vq[6];
			p[3-wbyte] = vq[7];

			p[4-wbyte] = vq[12];
			p[5-wbyte] = vq[13];
			p[6-wbyte] = vq[14];
			p[7-wbyte] = vq[15];

			p+=wbyte*2;
			p+=8;
		}
	}
	return 0;
}

int decode_twiddled_vq(unsigned char *out,const unsigned char *in0,const unsigned char *in1,int width,int height,int mode)
{
	unsigned char vqtab[4*4*256];
	const unsigned char *in;

	int x,y,wbyte;
	const SHIFTTBL *s;

	const unsigned short *in_w = (const unsigned short *)in1;
	unsigned char *p = vqtab;

	s = &shifttbl[mode];
	for(x=0;x<256*4;x++) {
		int c = in_w[x];
		p[R] = ((c & s->rmask) * s->rmult + s->radd) >> s->rshift;
		p[G] = ((c & s->gmask) * s->gmult + s->gadd) >> s->gshift;
		p[B] = ((c & s->bmask) * s->bmult + s->badd) >> s->bshift;
		p[A] = ((c & s->amask) * s->amult + s->aadd) >> s->ashift;
		p+=4;
	}

	in = in0;
	wbyte = width*4;
	for(y=0;y<height;y+=2) {
		unsigned char *p = out+(height-y-1)*wbyte;
		for(x=0;x<width;x+=2) {
			int c = in[twiddle(width/2, height/2, x/2, y/2)];
			unsigned char *vq = &vqtab[c*16];

			p[0] = vq[0];
			p[1] = vq[1];
			p[2] = vq[2];
			p[3] = vq[3];

			p[4] = vq[8];
			p[5] = vq[9];
			p[6] = vq[10];
			p[7] = vq[11];

			p[0-wbyte] = vq[4];
			p[1-wbyte] = vq[5];
			p[2-wbyte] = vq[6];
			p[3-wbyte] = vq[7];

			p[4-wbyte] = vq[12];
			p[5-wbyte] = vq[13];
			p[6-wbyte] = vq[14];
			p[7-wbyte] = vq[15];

			p+=8;
		}
	}
	return 0;
}

int decode_twiddled(unsigned char *out,const unsigned char *in0,int width,int height,int mode)
{
	int x,y,wbyte;
	const SHIFTTBL *s;

  switch(mode){
  case PAL4:
	wbyte = (width/2+3)&~3;
	for(y=0;y<height;y+=2) {
		unsigned char *p = out+(height-y-1)*wbyte;
		const unsigned char *in=in0;
		for(x=0;x<width;x+=2) {
			int c0 = in[twiddle(width, height/2, x, y/2)];
			int c1 = in[twiddle(width, height/2, x+1, y/2)];
			p[x/2      ] = (c0&0x0f) | (c1<<4);
			p[x/2-wbyte] = (c0>>4)   | (c1&0xf0);
		}
	}
	break;
  case PAL8:
	wbyte = (width+3)&~3;
	for(y=0;y<height;y++) {
		unsigned char *p = out+(height-y-1)*wbyte;
		const unsigned char *in=in0;
		for(x=0;x<width;x++) {
			p[x] = in[twiddle(width, height, x, y)];
		}
	}
	break;
  default:
	s = &shifttbl[mode];
	wbyte = width*4;
	for(y=0;y<height;y++) {
		unsigned char *p = out+(height-y-1)*wbyte;
		const unsigned short *in=(const unsigned short *)in0;
		for(x=0;x<width;x++) {
			int c = in[twiddle(width, height, x, y)];
            p[R] = ((c & s->rmask) * s->rmult + s->radd) >> s->rshift;
            p[G] = ((c & s->gmask) * s->gmult + s->gadd) >> s->gshift;
            p[B] = ((c & s->bmask) * s->bmult + s->badd) >> s->bshift;
            p[A] = ((c & s->amask) * s->amult + s->aadd) >> s->ashift;
			p+=4;
		}
	}
  }
  return 0;
}

int decode_rectangle(unsigned char *out,const unsigned char *in0,int width,int height,int mode)
{
	int x,y,wbyte;
	const SHIFTTBL *s;
	if (mode<=2)
		s = &shifttbl[mode];

	wbyte = width*4;
	for(y=0;y<height;y++) {
		unsigned char *p = out+(height-y-1)*wbyte;
		const unsigned short *in = (const unsigned short *)in0;
		for(x=0;x<width;x++) {
			int c = in[x];
			p[R] = ((c & s->rmask) * s->rmult + s->radd) >> s->rshift;
			p[G] = ((c & s->gmask) * s->gmult + s->gadd) >> s->gshift;
			p[B] = ((c & s->bmask) * s->bmult + s->badd) >> s->bshift;
			p[A] = ((c & s->amask) * s->amult + s->aadd) >> s->ashift;
			p+=4;
		}
	}
	return 0;
}

int decode_rectangle_twiddled(unsigned char *out,const unsigned char *in0,int width,int height,int mode)
{
	int x,y,wbyte;
	const SHIFTTBL *s;
	if (mode<=2)
		s = &shifttbl[mode];

	wbyte = width*4;
	for(y=0;y<height;y++) {
		unsigned char *p = out+(height-y-1)*wbyte;
		const unsigned short *in = (const unsigned short *)in0;
		for(x=0;x<width;x++) {
			int c = in[twiddle(width, height, x, y)];
			p[R] = ((c & s->rmask) * s->rmult + s->radd) >> s->rshift;
			p[G] = ((c & s->gmask) * s->gmult + s->gadd) >> s->gshift;
			p[B] = ((c & s->bmask) * s->bmult + s->badd) >> s->bshift;
			p[A] = ((c & s->amask) * s->amult + s->aadd) >> s->ashift;
			p+=4;
		}
	}
	return 0;
}

int pvr2binhdr(int &width, int &height, int &img_size,int &bits, PVRHDR *hdr)
{
    // !!MH!! change handling to include both 24 and 32 bit color depth.
    // Right now the alpha channel is always used.
	switch(hdr->type[0]){
	case 0x00: /* ARGB155 */
	case 0x01: /* RGB565 */
	case 0x02: /* ARGB4444 */
	case 0x03: /* YUV422 */
		bits = 32; break;
	case 0x04: /* BUMP */
	case 0x05: /* 4BPP */
		bits = 4; break;
	case 0x06: /* 8BPP */
		bits = 8; break;
	default:
        break;
	}
	width = hdr->width;
	height = hdr->height;
	img_size = ((hdr->width*bits/8+3)&~3) * hdr->height;
	return 0;
}

int pvr2bindata(unsigned char *pbin,const PVRHDR *hdr)
{
	const unsigned char *data = (const unsigned char*)(hdr+1);

	switch(hdr->type[1]){
	case 0x01: /* twiddled */
		decode_twiddled(pbin,data,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x02: /* twiddled & mipmap */
		decode_twiddled(pbin,data+calc_n(hdr->width)*2,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x03: /* VQ */
		decode_twiddled_vq(pbin,data+2048,data,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x04: /* VQ & MIPMAP */
		decode_twiddled_vq(pbin,data+2048+calc_n(hdr->width/2),data,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x05: /* twiddled 4bit? */
		decode_twiddled(pbin,data,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x06: /* twiddled & mipmap 4bit? */
		decode_twiddled(pbin,data+calc_n(hdr->width)/2,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x07: /* twiddled 8bit? */
		decode_twiddled(pbin,data,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x08: /* twiddled & mipmap 8bit? */
		decode_twiddled(pbin,data+calc_n(hdr->width),hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x09: /* RECTANGLE */
		decode_rectangle(pbin,data,hdr->width,hdr->height,hdr->type[0]);
		break;
    case 0x0d: /* RECTANGLE twiddled */
		decode_rectangle_twiddled(pbin,data,hdr->width,hdr->height,hdr->type[0]);
        break;
	case 0x10: /* SMALL_VQ */
		decode_small_vq(pbin,data+128,data,hdr->width,hdr->height,hdr->type[0]);
		break;
	case 0x11: /* SMALL_VQ & MIPMAP */
		decode_small_vq(pbin,data+128+calc_n(hdr->width/2)/2,data,hdr->width,hdr->height,hdr->type[0]);
		break;
	default:
		break;
	}
	return 0;
}
