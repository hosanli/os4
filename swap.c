#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "spinlock.h"
#include "bitmap.h"

//Constants for swapping
#define SECTOR 512
#define SECTOR_PAGE (PAGE/SECTOR)
#define SWAP_SIZE 10

struct swap {
	
	struct spinlock lock;
	unsigned char bitmap[4096];
}state;

/*Set swap space and bitmap */
void swap_init(void){

	int j;
	int avail_space_in_swap;
	
	initlock(&state.lock,"swap_lock");
	
	memset(&state.bitmap,0Xff,sizeof(PAGE));
	
	avail_space_in_swap = SWAP_SIZE < PAGE ? SWAP_SIZE : PAGE;
	for(j = 0;j<avail_space_in_swap;j++){	
		clear_bit(state.bitmap,j);
	}
}

//Free swap pages 

void swap_free_page(int block){
	acquire(&state.lock);
	clear_bit(state.bitmap,block);
	release(&state.lock);
	
}

//Find a free page in swap and return the block address of the page allocated

int allocate_page_for_swap(){

	int block;
	acquire(&state.lock);
	block = bitmap_scan(state.bitmap,sizeof(state.bitmap));
	 
	if(block >= 0){
		set_bit(state.bitmap,block);	
	}
	release(&state.lock);
	return block;
}

//Write pages to disk but make sure you allocate pages before writing via allocate_page_for_swap

void swap_page_to_disk(int block,char *target_page){

	int start_addr = block*SECTOR_PAGE;	
	int j = 0;
	for(;j<SECTOR_PAGE;j++){
		writeswapblock(start_addr+j,target_page+(j*SECTOR));
	}
}


// Read page from the block id given

void swap_page_from_disk(int block,char *target_page){

	int start_addr = block*SECTOR_PAGE;	
	int j = 0;
	for(;j<SECTOR_PAGE;j++){
		readswapblock(start_addr+j,target_page+(j*SECTOR));
	}
}
