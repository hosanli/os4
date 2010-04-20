#include "types.h"
#include "defs.h"
#include "param.h"
#include "page.h"

page_dir_t *kernel_dir = 0;
page_dir_t *current_dir = 0;

extern char end[];
static uint kend; 	

void pageinit() {
	kend = ((uint)end + PAGE) & ~(PAGE-1);

	// create a page directory for the kernel
	kernel_dir = (page_dir_t *)kalloc(sizeof(page_dir_t));	
	memset(kernel_dir, 0, sizeof(page_dir_t));

	// identity map kernel code and data 
	uint addr_i = 0;
	uint offs = 0;
//	uint size = kend;
	uint size = 0x4000000;
	page_t *p;
	while(addr_i < size) {
		// pass the physical address of kernel to the second parameter
		// as if it was a virtual address
		p = get_page(kernel_dir, addr_i + offs, 1/*create a page*/);
		set_page(addr_i + offs, p, 1/*read-only*/, 1/*supervisor mode*/);
//		cprintf("---- create page %d at %x \n", addr_i/PAGE, p);
		addr_i += PAGE;
	}

	// identity map apic
	addr_i = 0;
	offs = APIC_START_A;
	size = 0xffffffff - APIC_START_A;
	while(addr_i < size) {
		p = get_page(kernel_dir, addr_i + offs, 1/*create a page*/);
		set_page(addr_i + offs, p, 1/*read-only*/, 1/*supervisor mode*/);
	//	cprintf("---- create page %d at %x \n", addr_i/PAGE, p);
		addr_i += PAGE;
	}

	switch_dir(kernel_dir);
}

void switch_dir(page_dir_t *dir) {
	current_dir = kernel_dir;
	asm volatile("movl %0, %%cr3":: "r"(&dir->dirs));
	uint cr0;
	asm volatile("movl %%cr0, %0": "=r"(cr0));
	cr0 |= (CR0_PG | CR0_WP); 		// set CR0.PG = 1 and CR0.WP = 1 
	asm volatile("movl %0, %%cr0":: "r"(cr0));
}

// Get page's physical address according to its virtual address
// if new is set to 1 and the page doesn't exist, then create a new page 
page_t *get_page(page_dir_t *dir, uint vaddress, int new) {
	vaddress /= OFFSET_L;				// high 20 bits 
	uint index = vaddress / PAGE_L;		// high 10 bits 
										// which is the index of page table in directory

	if(dir->pagetables[index] != 0) {
		return &dir->pagetables[index]->pages[vaddress % PAGE_L]; // low 10 bits in vaddress is now the
					        							  // index of page in page table
	}
	// when the page table does not exist
	else if (new){
		page_table_t *ptaddr;
		ptaddr = (page_table_t *)kalloc(sizeof(page_table_t));
//		ksize += sizeof(page_table_t);
		memset(ptaddr, 0, sizeof(page_table_t));
		cprintf("---- create page table %d at %x \n", index, ptaddr);
		
		dir->pagetables[index] = ptaddr;
		dir->dirs[index] = (uint)ptaddr | 0x7;		// set PRESENT, R/W, U/S 
	//	cprintf("---- dir->dirs at %x \n", &dir->dirs);
	//	cprintf("---- dir->paget at %x \n", &dir->pagetables);
		
		return &dir->pagetables[index]->pages[vaddress % PAGE_L];
	}
	else
		return 0;
}

void *set_page(uint addr, page_t *p, int _r_w, int _u_s) {
	if(!p) {
		panic("set_page");
	}
	p->present = 1;
	p->r_w = _r_w;
	p->u_s = _u_s;	
	p->phyaddr = addr / OFFSET_L;
}

//	Paging fault handler
void pageintr() {
	uint faultaddr;
	asm volatile("mov %%cr2, %0" : "=r"(faultaddr));
	cprintf("page fault at %x\n", faultaddr);
}
