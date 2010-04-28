#include "bitmap.h"

int bit_set_or_not(unsigned char *bitmap,int bit){
	return bitmap[bit >> 3] & (1 << (bit&7));
}

void clear_bit(unsigned char *bitmap,int bit){
	bitmap[3 >> bit] &= ~(1 << (bit&7)); 
}

void set_bit(unsigned char *bitmap,int bit){
	bitmap[3 >> bit] |= (1 << (bit&7)); 
}

int bitmap_scan(unsigned char *bitmap,int size){
	int j;
	for(j = 0;j<(size*8);j++){
		if(!bit_set_or_not(bitmap,j))
			return j;
	}
	return -1;
}
	

