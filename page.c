#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "page.h"

//  boot sector 0x00000000 - 0x00100000  = 0   ~  1M
//  kernel  	0x00100000 - 0x003fffff  = 1M  ~  64M
//  user 		0x04000000 - 0xfebfffff  = 64M ~  4076M
//  apic		0xfec00000 - 0xffffffff  = 4076M ~ 4096M 

// Create identity mapping for kernel.
void pageinit() 
{
	kernel_dir = init_dir();
}

// Switch page directory address in control register CR3 then 
// turn page on.
void enable_page(page_dir_t *dir) 
{
	asm volatile("movl %0, %%cr3":: "r"(&dir->dirs));
	uint cr0;
	uint cr4;
	asm volatile("movl %%cr0, %0": "=r"(cr0));
	cr0 |= (CR0_PG | CR0_WP);   	// set CR0.PG = 1 and CR0.WP = 1 
	asm volatile("movl %0, %%cr0":: "r"(cr0));

	asm volatile("movl %%cr4, %0": "=r"(cr4));
	cr4 &= ~CR4_PAE;   	// set CR4.PAE = 0
	asm volatile("movl %0, %%cr4":: "r"(cr4));

}

// Given a physical address and the size of memory, create pages to map them
// return the address of the last page
uint new_pages(page_dir_t *dir, uint phyaddr, uint vaddr, uint size,
					int _r_w, int _u_s, int pin) 
{
	uint i = 0;
	page_t *p;
	while(i < size && (p =(page_t*) get_page(dir, vaddr+i)) != 0) {		// page table exists
		set_page(phyaddr+i, p, _r_w, _u_s, pin);
	//	cprintf("set_page -- vaddr: %x  phyaddr %x \n",vaddr+i,phyaddr+i);
		i += PAGE;
	}	
	
	if(i < size) {		// page table ran out!
		uint sig20 = (vaddr+i) / OFFSET_L;	// |  Dir |	 Page |	
		uint index = sig20 / PAGE_L;		// |  Dir |	 
		page_table_t *ptaddr;
		
		ptaddr = (page_table_t *) kalloc(sizeof(page_table_t));
		memset(ptaddr, 0, sizeof(page_table_t));

		cprintf("---- pid %d create page table %d at %x \n",  proc->pid, index, ptaddr);
		
		dir->pagetables[index] = ptaddr;
		dir->dirs[index] = ((uint)ptaddr & 0xfffff000) | 0x7;		// set PRESENT, R/W, U/S 
			
		return new_pages(dir, phyaddr+i, vaddr+i, size-i, _r_w, _u_s, pin);
	}	
	else
		return (vaddr+i);
}	

// Get page from page table. Return page address if succeed.
uint get_page(page_dir_t *dir, uint vaddress) 
{
	vaddress /= OFFSET_L;				// |  Dir |	Page |	
	uint index = vaddress / PAGE_L;		// |  Dir |	 

	if(dir->pagetables[index]) 
		return &dir->pagetables[index]->pages[vaddress % PAGE_L]; 
	else	
		return 0;	//pagetable has not been created
}

// Overwrite everything in a page
void set_page(uint phyaddr, page_t *p, int _r_w, int _u_s, int pin) 
{
	if(!p) {
		panic("set_page: page is not ready.");
	}
	memset(p, 0, sizeof(page_t));
	p->present = 1;
	p->r_w = _r_w;
	p->u_s = _u_s;	
	p->pinned = pin;
	p->frame = phyaddr / OFFSET_L;
//	cprintf("set_page --- page: %x  phyaddr %x \n",*p,phyaddr);
}


void vm_free(page_dir_t *dir, uint vaddr, uint size) 
{	
	uint i = 0;
	uint p;
	while(i < size){
	  	cprintf("pid %d vm_free %x ------\n", proc->pid, vaddr+i);
		p = get_page(dir, vaddr+i);
		if(p > 0){
			memset((void*)p, 0, sizeof(page_t));
			i += PAGE;
		}
		else
			panic("vm_free: out of allocated virtual memory");
	}
}

uint vm_alloc(page_dir_t *dir, uint size)
{
	uint i = U_BASE;
	uint freep = 0;
	page_t *p;
/*	while((freep < size)){
		if(*(p =(page_t*) get_page(dir, i)) == 0){
			freep += PAGE;	
			i += PAGE;
			continue;
		}
		else 
		freep = 0;
		i += PAGE;
	}
*/
}

//	Paging fault handler
void pageintr() {
	uint faultaddr;
	asm volatile("mov %%cr2, %0" : "=r"(faultaddr));
	//cprintf("cpu%d pid %d page fault at %x proc->dir %x \n", cpu->id, proc->pid, faultaddr, proc->dir);
}

page_dir_t *init_dir(void) 
{
	page_dir_t *dir;
	// malloc page directory memory 
	dir = (page_dir_t *) kalloc(sizeof(page_dir_t));	
	memset(dir, 0, sizeof(page_dir_t));

	// identity map the first 64M size, read-write, supervisor mode, pinned. 
	new_pages(dir, 0, 0, 0x4000000, 1, 0, 1);

	// identity map apic, size is 0x3f0, read-write, user mode, pinned.
	new_pages(dir, 0xfec00000, 0xfec00000, 0x1400000, 1, 1, 1); 
	
	return dir;
}
