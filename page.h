#ifndef PAGE_H
#define PAGE_H

/*
// Page directory entry
typedef struct {
	unsigned int present : 1;	
	unsigned int r_w	 : 1;	// Read/write
	unsigned int u_s	 : 1;	// User/supervisor
	unsigned int accessed: 1;
	unsigned int unused  : 8;
	unsigned int phyaddr : 20; 	// Physical address
}__attribute__((packed)) dir_t;	 
*/

// Page table entry
struct page {
	unsigned int present : 1;	
	unsigned int r_w	 : 1;	// Read/write
	unsigned int u_s	 : 1;	// User/supervisor
	unsigned int unused1 : 2;
	unsigned int accessed: 1;
	unsigned int dirty	 : 1;
	unsigned int unused  : 4;
	unsigned int pinned  : 1;	// Can it be swapped out?
	unsigned int frame	 : 20; 	// Physical address
}__attribute__((packed));	 

typedef struct page page_t; 

// A Page table
struct page_table {
	page_t pages[1024];
};

typedef struct page_table page_table_t; 

// A Directory
struct page_dir {
	uint dirs[1024];		// the array of directory entries
	page_table_t *pagetables[1024];
};

typedef struct page_dir page_dir_t;

page_dir_t *kernel_dir;

// Control register
#define CR0_PG 			0x80000000		// Paging Enable 
#define CR0_WP			0x00010000		// Write protect 
#define CR4_PAE			0x00000020

// Memory offsets
//	+---------+---------+----------+
//	|	Dir	  |	 Page	|	Offset |
//	+---------+---------+----------+
//		10		  10		12
#define DIR_L			0x400
#define PAGE_L			0x400
#define OFFSET_L		0x1000

// Some addresses
#define KSTART_A			0x00100000

#endif
