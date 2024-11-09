/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 02/02/17


    This file has the main entry point to the operating system.

*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
/* Makes things easy to read */

#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))
/* Definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / (4 KB))
#define MEM_HOLE_SIZE ((1 MB) / (4 KB))
/* We have a 1 MB hole in physical memory starting at address 15 MB */

#define TEST_START_ADDR_PROC (4 MB)
#define TEST_START_ADDR_KERNEL (2 MB)
/* Used in the memory test below to generate sequences of memory references. */
/* One is for a sequence of memory references in the kernel space, and the   */
/* other for memory references in the process space. */

#define N_TEST_ALLOCATIONS 
/* Number of recursive allocations that we use to test.  */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H"     /* LOW-LEVEL STUFF   */
#include "console.H"

#include "assert.H"
#include "cont_frame_pool.H"  /* The physical memory manager */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go);
void test_max_alloc(ContFramePool * _pool);
void release_all_frames(ContFramePool * _pool);
void alloc_and_release_last_frame(ContFramePool * _pool);
/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main() {

    Console::init();

    /* -- INITIALIZE FRAME POOLS -- */

    /* ---- KERNEL POOL -- */
    
    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0);
    

    /* ---- PROCESS POOL -- */


    unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

    unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);
    
    ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                   PROCESS_POOL_SIZE,
                                   process_mem_pool_info_frame);
    
    process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);
/**/
    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");

    /* -- TEST MEMORY ALLOCATOR */
    
    test_memory(&kernel_mem_pool, 32);

    /* ---- Add code here to test the frame pool implementation. */
    
    
    test_max_alloc(&process_mem_pool);
    release_all_frames(&process_mem_pool);
    alloc_and_release_last_frame(&process_mem_pool);
   

    /* -- NOW LOOP FOREVER */
    Console::puts("Testing is DONE. We will do nothing forever\n");
    Console::puts("Feel free to turn off the machine now.\n");

    for(;;);

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}


// test the maximum mem we can alloc in process pool, there are 7168 frames - 256 for the mem hole,
// we don't need to worry about the info frame because it exists in the kernel frame pool, call 6912 frames
// mem hole at 3840 to 4096
// This test shows that we can allocated all frames and that the mark_inaccessible function works
// We need 2 allocations because the get_frames function allocates contigously, but because of the
// mem hole we can't allocate contigously.
void test_max_alloc(ContFramePool * _pool)
{
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 6912 Frames "); Console::puts("\n");
    _pool->get_frames(4096);
    _pool->get_frames(2816);
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 0 Frames "); Console::puts("\n");
}

// Release all the Frames allocated by the test_max_alloc test case
void release_all_frames(ContFramePool * _pool)
{
    _pool->release_frames(1024);
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 2816 Frames "); Console::puts("\n");
    _pool->release_frames(4096);
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 6192 Frames "); Console::puts("\n");
}

// Alloc and release the last frame possible edge case
void alloc_and_release_last_frame(ContFramePool * _pool)
{
    _pool->get_frames(4095);
    _pool->get_frames(2816);
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 1 Frame "); Console::puts("\n");
    _pool->get_frames(1);
    _pool->release_frames(1024);
    _pool->release_frames(4096);
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 6191 Frames "); Console::puts("\n");
    _pool->release_frames(8191);
    Console::puts("Free Frames: "); Console::puti(_pool->free_frames()); Console::puts(" Expecting 6192 Frames "); Console::puts("\n");
}


void test_memory(ContFramePool * _pool, unsigned int _allocs_to_go) {
    Console::puts("alloc_to_go = "); Console::puti(_allocs_to_go); Console::puts("\n");
    if (_allocs_to_go > 0) {
        int n_frames = _allocs_to_go % 4 + 1;
        unsigned long frame = _pool->get_frames(n_frames);
        int * value_array = (int*)(frame * (4 KB));        
        for (int i = 0; i < (1 KB) * n_frames; i++) {
            value_array[i] = _allocs_to_go;
        }
        test_memory(_pool, _allocs_to_go - 1);
        for (int i = 0; i < (1 KB) * n_frames; i++) {
            if(value_array[i] != _allocs_to_go){
                Console::puts("MEMORY TEST FAILED. ERROR IN FRAME POOL\n");
                Console::puts("i ="); Console::puti(i);
                Console::puts("   v = "); Console::puti(value_array[i]); 
                Console::puts("   n ="); Console::puti(_allocs_to_go);
                Console::puts("\n");
                for(;;); 
            }
        }
        ContFramePool::release_frames(frame);
    }
}

