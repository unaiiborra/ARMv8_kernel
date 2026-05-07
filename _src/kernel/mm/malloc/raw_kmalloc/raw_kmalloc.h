#pragma once


// private for mm system, the public api is under kernel/mm.h
void raw_kmalloc_init();


// internal allocators (page allocator, vmalloc and the mm_mmu) can use them if
// they expose a function that requires locking their state
void raw_kmalloc_lock();
void raw_kmalloc_unlock();

int  raw_kmalloc_lock_defer();
void raw_kmalloc_unlock_defer(int* cleanup);