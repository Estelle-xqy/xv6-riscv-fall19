// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++)
    initlock(&kmems[i].lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  release(&kmems[id].lock);
}

void *
steal(int skip){
  // printf("cpu id %d\n",getcpu());
  struct run * rs=0;
  for(int i=0;i<NCPU;i++){
    // 当前cpu的锁已经在外面获取了，这里为了避免死锁，需要跳过
    if(holding(&kmems[i].lock)){
      continue;
    }
    acquire(&kmems[i].lock);
    if(kmems[i].freelist!=0){
      rs=kmems[i].freelist;
      kmems[i].freelist=rs->next;
      release(&kmems[i].lock);
      return (void *)rs;
    }
    release(&kmems[i].lock);
  }
  // 不管还有没有，都直接返回，不用panic
  return (void *)rs;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int id = cpuid();
  pop_off();

  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if(r)
    kmems[id].freelist = r->next;
  release(&kmems[id].lock);
  if(!r)
  {
    // 当前cpu对应的freelist为空 需要从其他cpu对应的freelist借
    r=steal(id);
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


