#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


//structure
//header
//type, 2 bits
//RLE size, 2 bits
//size in bytes? - 12? bits


//type 0, reference frame

//type 1, delta frame, no ref, no offset

//type 2, delta frame, no ref
//first two bytes are offset into the image, in pixels for the start


//type 3, delta frame
//first two bytes? are offset into the iamge, in pixels for the start
//second byte is how far back to grab?
//might merge into first two bytes, like a 13-3 split
//might get rid of it if i cant store many frames in ram... about 3 frames per 2k



#define getplace(byte,place) (!!((byte) & (1 << (place))));
#define getbit(data, bit)(getplace((data)[(bit)/8],(bit)%8))

unsigned int width;
unsigned int height;

int getbits(int place, unsigned char * buf, int numbits){
	int data = 0;
	int i;
	for(i = 0; i < numbits; i++){
		int pbit = place+i;
		int pushbyte = pbit/8;
		int pushbit = pbit%8;
		int bit = (buf[pushbyte]>>pushbit) & 1;
		data |= (bit << i);
	}
	return data;
}

int setbits(int place, unsigned char * buf, int data, int numbits){
	//silly slow version lol
	int i;
	for(i = 0; i< numbits; i++){
		int bit = (data >> i) & 1;
		int pbit = place+i;
		int pushbyte = pbit /8;
		int pushbit = pbit %8;
		buf[pushbyte] = bit << pushbit | (buf[pushbyte] & ~(1 << pushbit));
	}
	return place+numbits;
}


int rledecode(unsigned char * out, unsigned char * in, unsigned int insize, int nbits){
	int i;
	int outplace;
	int compare = 0;
//	for(i = outplace = 0; i < insize; i+=nbits){
	for(i = outplace = 0; i < insize; i++){
//		int det = getbits(i, in, nbits);
		int det = in[i];
		int kek;
		for(kek = 0; kek < det; kek++, outplace++){
			out[outplace] = compare;
		}
		compare = !compare;
	}
	return i;
}


//returns len in bytes
//insize is in pixels
int rlencode(unsigned char * out, unsigned char * in, unsigned int insize, int nbits){
	//find end
	int i;
	int prev = out[insize-1];
	i = insize -2;
//	for(i = insize -2; i>=0; i--) if(out[i] != prev) break;
	i++;
	//i should now be size it needs to actually encode
	int mx = i;
	int compare = 0;
	int place;
	int mxcnt = (1 << nbits) -1;
	printf("maxcnt is %i\n", mxcnt);
//	for(i = place = 0; i < mx; place += nbits){
	for(i = place = 0; i < mx; place ++){
		int cnt;
		for(cnt = 0; in[i+cnt] == compare && cnt < mxcnt; cnt++);
		if(cnt >= mxcnt) cnt = mxcnt -1;
		i+= mxcnt;
		out[place] = (unsigned char )cnt;
//		setbits(place, out, cnt, nbits);
		compare = !compare;
	}
	return place;
}

unsigned char * dtemp = 0;
int getdelta(unsigned char * out, unsigned char * cimg, unsigned char *oldimg){
	if(!dtemp) dtemp = malloc(width * height);
	int i;
	int mx = width * height;
	for(i = 0; i < mx; i++){
		dtemp[i] = (cimg[i] != oldimg[i]);
	}
	rlencode(out, dtemp, mx, 8);
	return 0;
}


int main(const int argc, const char ** argv){
	int x, y, n;
	unsigned char *data = stbi_load(argv[1], &x, &y, &n, 1);
	unsigned char * newdata = malloc(x*y);
	printf("loaded %s %i %i %i\n", argv[1], x,  y, n);
	width =x;
	height =y;
	int fx, fy;
	for(fy = 0; fy < y; fy++){
		for(fx = 0; fx < x; fx++){
			putc(data[fy*x+fx] ? '0' : ' ' ,stdout);
		}
		putc('\n', stdout);
	}
/*
	int kx, ky;
	for(ky = 0; ky < y/2; ky++){
	for(kx = 0; kx < x/2; kx++){
		newdata[ky * x/2 + kx] = data[ky * x + kx];
	}}
	data = newdata;
	x/=2;
	y/=2;
*/
	int i;
	int place = 0;
	int chuckie = 1;
	for(chuckie = 0; chuckie < 16; chuckie++){
	for(i = place = 0;place < x*y; i+=(2*(chuckie+1))){
		int cntr;
		for(cntr = 0; place < x*y && cntr < (1 << chuckie) && !data[place]; cntr++,place++);
		for(cntr = 0; place < x*y && cntr < (1 << chuckie) &&  data[place]; cntr++,place++);
	}
		printf("%i %i %i\n", chuckie, 1 << chuckie, i);
	}


	unsigned char *compressdata = malloc(x * y);
	unsigned char *outdata = malloc(x * y);
	memset(compressdata, 0, x*y);
	memset(newdata, 0, x*y);
	memset(outdata, 0, x*y);

	int s = rlencode(compressdata, data, x * y, 8);
	printf("yas %i\n", s);
	rledecode(outdata, compressdata, s, 8);
	for(fy = 0; fy < y; fy++){
		for(fx = 0; fx < x; fx++){
			putc(outdata[fy*x+fx] ? '0' : ' ' ,stdout);
		}
		putc('\n', stdout);
	}
	return 0;
}
