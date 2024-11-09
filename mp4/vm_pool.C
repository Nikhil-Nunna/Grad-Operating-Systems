/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {
    // set base address
    base_address = _base_address;
    // set size
    size = _size;
    // set frame pool ptr
    frame_pool = _frame_pool;
    // set page table ptr
    page_table = _page_table;
    // register vm pool
    page_table->register_pool(this);
    // set the allocated array to the first page
    alloc_array = (struct region *) (base_address);
    alloc_array[0].base_page = 0;
    alloc_array[0].lenght = 8192;
    // set the free array to the second page
    free_array = (struct region *) (base_address + 4096);
    free_array[0].base_page = 2;
    free_array[0].lenght = size - 8192; 
    // set all the other locations in the arrays to 0
    for (int i = 1; i < 512; i++)
    {
        alloc_array[i].base_page = 0;
        alloc_array[i].lenght = 0;
        free_array[i].base_page = 0;
        free_array[i].lenght = 0;
    }
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
    // This is always a multiple of 4096

    for (int i = 0; i < 512; i++)
    {
        // set pages and size to 0 in loop
        unsigned long pages = 0;
        unsigned long size = 0;
        // check if size is greater than 4096
        if (4096 >= _size)
        {
            // only 1 page needed
            pages = 1;
            size = 4096;
        }
        else
        {
            // Find out if an extra page is needed for anything that doesn't fit
            int remainder = _size % 4096;
            pages = _size / 4096;
            
            // add extra page 
            if (remainder != 0)
            {
                pages++;
            }
            // set size of mem
            size = pages * 4096;
        }
        
        // find enough mem in free mem
        if (free_array[i].lenght >= size)
        {
            // find open slot in alloc mem
            for (int j = 0; j < 512; j++)
            {
                if(alloc_array[j].lenght == 0)
                {
                    // transfer the base page
                    alloc_array[j].base_page = free_array[i].base_page;
                    // set the memory needed
                    alloc_array[j].lenght = size;
                    // set the free array to the next free page and subtract the size from the lenght of the block
                    free_array[i].base_page = free_array[i].base_page + pages;
                    free_array[i].lenght = free_array[i].lenght - size;
                    // return the address of the memory
                    return base_address + (alloc_array[j].base_page * 4096);
                }
            }
            
        }


    }
    Console::puts("Allocated region of memory.\n");
    return 0;
}

void VMPool::release(unsigned long _start_address) {
    // check that the start address is valid
    if (is_legitimate(_start_address))
    {
        // get the base page no
        unsigned long page_no = (_start_address - base_address) / 4096;
        // find allocation with matching base page
        for (int i = 0; i < 512; i++)
        {
            if(alloc_array[i].base_page == page_no)
            {
                // free the pages in the allocation
                for(int j = 0; j < (alloc_array[i].lenght / 4096); j++)
                {
                    unsigned long free_address = base_address + ((alloc_array[i].base_page + j) * 4096);
                    page_table->free_page(free_address);
                }
                // move memory back to the free array
                for (int j = 0; j < 512; j++)
                {
                    // move mem into an free array slot not being used
                    if (free_array[j].lenght == 0)
                    {
                        // transfer the mem location and lenght
                        free_array[j].base_page = alloc_array[i].base_page;
                        free_array[j].lenght = alloc_array[i].lenght;
                        // deallocate from the allocated array
                        alloc_array[i].base_page = 0;
                        alloc_array[i].lenght = 0;
                        return;
                    }
                    
                }
            }
        }
        
    }
    
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    // handle case for first 2 pages as they should always be in mem
    unsigned long array_space = base_address + 8192;
    if ((_address >= base_address) && (_address < array_space))
    {
        return true;
    }
    
    // Check if the address passed in is in a valid memory region
    // for (int i = 0; i < 512; i++)
    // {
    //     if ((_address >= (base_address + (alloc_array[i].base_page * 4096)) ) && (_address < (base_address + (alloc_array[i].base_page * 4096) + alloc_array[i].lenght)))
    //     {
    //         return true;
    //     }
    // }
    return true;
    Console::puts("Checked whether address is part of an allocated region.\n");
}

