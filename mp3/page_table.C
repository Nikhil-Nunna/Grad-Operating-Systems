#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = nullptr;
bool PageTable::paging_enabled = false;
ContFramePool * PageTable::kernel_mem_pool = nullptr;
ContFramePool * PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   // Set private variables
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{  
   // Get page directory frame
   unsigned long page_directory_address = 4096 * kernel_mem_pool->get_frames(1);
   page_directory = (unsigned long *) page_directory_address;
   
   // Get page table frame 
   unsigned long page_table_address = 4096 * kernel_mem_pool->get_frames(1);
   unsigned long *page_table = (unsigned long *) page_table_address;

   // Direct map first 4 MB of memory
   unsigned long address=0;
   unsigned int i;
   
   // Loop thru page table and set all address to present for first 4 mb
   for(i=0; i<1024; i++)
   {
      page_table[i] = address | 3;
      address = address + 4096;
   }

   // Add page table to page_directory
   page_directory[0] = (unsigned long) page_table; 
   page_directory[0] = page_directory[0] | 3;
   
   // Loop thru page directory and set all other PDEs to empty
   for(i=1; i<1024; i++)
   {
      page_directory[i] = 0UL | 2;
   }
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   // Load base register with address of page directory
   write_cr3((unsigned long) page_directory); 
   // set current page table to this object
   current_page_table = this;
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   // Flip bit in cr0 to enable paging
   write_cr0(read_cr0() | 0x80000000);
   // Enable paging in the object
   paging_enabled = true;
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
   // create bools for bits to check
   bool bit_2 = false;
   bool bit_1 = false;
   bool bit_0 = false;

   // create masks for bits to shift
   int mask_2 = 1 << 2;
   int mask_1 = 1 << 1;
   int mask_0 = 1 << 0;

   // get error code
   int code = _r->err_code;
   
   // check bit_1
   if ((code & mask_2) == 0)
   {
      bit_2 = false;
   }
   else
   {
      bit_2 = true;
   }

      // check bit_1
   if ((code & mask_1) == 0)
   {
      bit_1 = false;
   }
   else
   {
      bit_1 = true;
   }
      // check bit_0
   if ((code & mask_0) == 0)
   {
      bit_0 = false;
   }
   else
   {
      bit_0 = true;
   }


   // check whether or not the entry is present in the page directory
   int mask_pde_present = 1;
   if (!bit_0)
   {     
      // Create a new page table entry, check bit 2 to know which frame pool
      unsigned long *page_table_entry;
      if (!bit_2)
      {
         unsigned long page_table_entry_address = 4096 * kernel_mem_pool->get_frames(1);
         page_table_entry = (unsigned long *) page_table_entry_address;
      }
      else
      {
         unsigned long page_table_entry_address = 4096 * process_mem_pool->get_frames(1);
         page_table_entry = (unsigned long *) page_table_entry_address;
      }
      


      // Check the address in the cr2 register
      unsigned long attempted_address = read_cr2();

      

      // get page directory index
      unsigned long binary_pde_index = (attempted_address >> 22) & 1023;

      // Check whether or not the address exists in the page directory
      if ((current_page_table->page_directory[binary_pde_index] & mask_pde_present) == 0)
      {
 
         // put the new page table page in kernel mem
         unsigned long page_table_address = 4096 * kernel_mem_pool->get_frames(1);
         unsigned long *page_table = (unsigned long *) page_table_address;

         unsigned long binary_pte_index = (attempted_address >> 12) & 1023;

         // put the pte in page table
         page_table[binary_pte_index] = (unsigned long) page_table_entry;
         
         // set perms
         if (!bit_2)
         {
            // Kernel only page, read and write, present
            page_table[binary_pte_index] = page_table[binary_pte_index] | 3;
         }
         else
         {
            // Kernel and user page, read and write, present
            page_table[binary_pte_index] = page_table[binary_pte_index] | 7;
         }
         // Add new page to page directory
         current_page_table->page_directory[binary_pde_index] = (unsigned long) page_table;
         current_page_table->page_directory[binary_pde_index] = current_page_table->page_directory[binary_pde_index] | 3;
      }


      // case where PDE is good but PTE is missing 
      else
      {
         // get page directory index
         unsigned long binary_pde_index = (attempted_address >> 22) & 1023;
         
         // get page table address
         unsigned long page_table_address = current_page_table->page_directory[binary_pde_index];

         // set last 12 bits to 0
         page_table_address = page_table_address & 0xFFFFF000;
         
         // access page table
         unsigned long *page_table = (unsigned long *) page_table_address;
         
         // get page table entry address
         unsigned long binary_pte_index = (attempted_address >> 12) & 1023;
         
         // put the pte in page table
         page_table[binary_pte_index] = (unsigned long) page_table_entry;

         // set perms
         if (!bit_2)
         {
            // Kernel only page, read and write, present
            page_table[binary_pte_index] = page_table[binary_pte_index] | 3;
         }
         else
         {
            // Kernel and user page, read and write, present
            page_table[binary_pte_index] = page_table[binary_pte_index] | 7;
         }

      }
      
   }
  Console::puts("handled page fault\n");
}

