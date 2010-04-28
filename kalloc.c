// Physical memory allocator, intended to allocate
// memory for user processes. Allocates in 4096-byte "pages".
// Free list is kept sorted and combines adjacent pages into
// long runs, to make it easier to allocate big segments.
// One reason the page size is 4k is that the x86 segment size
// granularity is 4k.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"

struct run {
  struct run *next;
  int len; // bytes
};

struct spinlock vmem_lock;

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// Initialize free list of physical pages.
// This code cheats by just considering one megabyte of
// pages after end.  Real systems would determine the
// amount of memory available in the system and use it all.
void
kinit(int len)
{
  extern char end[];
  char *p;
  int vlen = len / 4;

  initlock(&kmem.lock, "kmem");
  initlock(&vmem_lock, "vmem");
  p = (char*)(((uint)end + PAGE) & ~(PAGE-1));
 
  cprintf(" mem =  %d pages = %d base  %x\n", len, vlen, p);
  kfree(p, (vlen - 256) * PAGE);
}

// Free the len bytes of memory pointed at by v,
// which normally should have been returned by a
// call to kalloc(len).  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v, int len)
{
  struct run *r, *rend, **rp, *p, *pend;

  if(len <= 0 || len % PAGE)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, len);

  acquire(&kmem.lock);
  p = (struct run*)v;
  pend = (struct run*)(v + len);
  for(rp=&kmem.freelist; (r=*rp) != 0 && r <= pend; rp=&r->next){
    rend = (struct run*)((char*)r + r->len);
    if(r <= p && p < rend)
      panic("freeing free page");
    if(rend == p){  // r before p: expand r to include p
      r->len += len;
      if(r->next && r->next == pend){  // r now next to r->next?
        r->len += r->next->len;
        r->next = r->next->next;
      }
      goto out;
    }
    if(pend == r){  // p before r: expand p to include, replace r
      p->len = len + r->len;
      p->next = r->next;
      *rp = p;
      goto out;
    }
  }
  // Insert p before r in list.
  p->len = len;
  p->next = r;
  *rp = p;

 out:
  release(&kmem.lock);
}

// Allocate n bytes of physical memory.
// Returns a kernel-segment pointer.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(int n)
{
  char *p;
  struct run *r, **rp;

  if(n % PAGE || n <= 0)
    panic("kalloc");

  acquire(&kmem.lock);
  for(rp=&kmem.freelist; (r=*rp) != 0; rp=&r->next){
    if(r->len >= n){
      r->len -= n;
      p = (char*)r + r->len;
	  if(r->len == 0)
		  *rp = r->next;
      release(&kmem.lock);
      return p;
    }
  }
  release(&kmem.lock);

  cprintf("kalloc: out of memory\n");
  return 0;
}


typedef long Align;

union header {
 struct {
   union header *ptr;
   uint size;
 } s;
 Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
_vfree(void *ap)
{
 Header *bp, *p;

 bp = (Header*) ap - 1;
 for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
   if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
     break;
 if(bp + bp->s.size == p->s.ptr){
   bp->s.size += p->s.ptr->s.size;
   bp->s.ptr = p->s.ptr->s.ptr;
 } else
   bp->s.ptr = p->s.ptr;
 if(p + p->s.size == bp){
   p->s.size += bp->s.size;
   p->s.ptr = bp->s.ptr;
 } else
   p->s.ptr = bp;
 freep = p;
}

void
vfree(void *ap) {
 acquire(&vmem_lock);
 _vfree(ap);
 release(&vmem_lock);
}

static Header*
morecore(uint nu)
{
 char *p;
 Header *hp;

 if(nu < PAGE)
   nu = PAGE;
 p = kalloc(nu * sizeof(Header));
 if(p == (char*) -1)
   return 0;
 hp = (Header*)p;
 hp->s.size = nu;
 _vfree((void*)(hp + 1));
 return freep;
}

void*
vmalloc(uint nbytes)
{
 Header *p, *prevp;
 uint nunits;

 acquire(&vmem_lock);
 nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
 if((prevp = freep) == 0){
   base.s.ptr = freep = prevp = &base;
   base.s.size = 0;
 }
 for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
   if(p->s.size >= nunits){
     if(p->s.size == nunits)
       prevp->s.ptr = p->s.ptr;
     else {
       p->s.size -= nunits;
       p += p->s.size;
       p->s.size = nunits;
     }
     freep = prevp;
     release(&vmem_lock);
     return (void*) (p + 1);
   }
   if(p == freep)
     if((p = morecore(nunits)) == 0) {
       release(&vmem_lock);
       return 0;
     }
 }
}

