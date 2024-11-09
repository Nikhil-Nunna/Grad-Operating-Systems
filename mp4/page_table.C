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
VMPool * PageTable::Head = nullptr;
VMPool * PageTable::Tail = nullptr;


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
   unsigned long page_directory_address = 4096 * process_mem_pool->get_frames(1);
   page_directory = (unsigned long *) page_directory_address;
   
   // Get page table frame 
   unsigned long page_table_address = 4096 * process_mem_pool->get_frames(1);
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

   // Recursive page table look-up
   page_directory[1023] = (unsigned long) page_directory;
   page_directory[1023] = page_directory[1023] | 3;
   
   // Loop thru page directory and set all other PDEs to empty
   for(i=1; i<1023; i++)
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

unsigned long * PageTable::PDE_address(unsigned long addr)
{
   //PDE = 1023 + 1023 + given address first 10 bits plus 2 0 bits offset
   unsigned long PDE = (1023 << 22) | (1023 << 12) | ((addr & 1023) << 2);
   return (unsigned long *) PDE;
   
}

unsigned long * PageTable::PTE_address(unsigned long addr)
{
   //PTE = 1023 + given address first 10 bits + given address second 10 bits plus 2 0 bits offset
   unsigned long PTE = (1023 << 22) | (((addr >> 22) & 1023) << 12) | (((addr >> 12) & 1023) << 2);
   return (unsigned long *) PTE;

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

   // Check the address in the cr2 register
   unsigned long attempted_address = read_cr2();

   // Region check to see if address is in registered VM pool
   VMPool* loop = Head;
   // set pool var to false flip if address is in pool
   bool in_pool = false;
   // loop till there are no more pools
   while (loop != nullptr)
   {
      // call is legitimate to check if address belongs to pool
      in_pool = loop->is_legitimate(attempted_address);
      // break if address in pool
      if (in_pool)
      {
         break;
      }
      // got to next pool
      loop = loop->next;
   }

   // aborting if address is not legit
   if (!in_pool)
   {
      return;
   }

   // check page present bit
   if (!bit_0)
   {

      // get page directory index
      unsigned long * PDE = PDE_address(attempted_address);

      // Check whether or not the address exists in the page directory
      int mask_pde_present = 1;
      if ((*PDE & mask_pde_present) == 0)
      {
         // put the new page table page in the Page Directory
         *PDE = 4096 * process_mem_pool->get_frames(1);      

         // Kernel and user page, read and write, present PDE
         *PDE = *PDE | 7;
      }

     // Load new page into page table 
      unsigned long * PTE = PTE_address(attempted_address);
      *PTE = 4096 * process_mem_pool->get_frames(1);

       // Create a new page table entry, check bit 2 to know which permissions
      if (!bit_2)
      {
         // Kernel only page, read and write, present
         *PTE = *PTE | 3;
      }
      else
      {
         // Kernel and user page, read and write, present
         *PTE = *PTE | 7;
      }

      
   }
  Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
   // check if there is anything in the list
   // if there isn't add to it
   if (Head == nullptr)
   {
      Head = _vm_pool;
      Tail = _vm_pool;
   }
   // since there is something already in the list append to it
   else
   {
      Tail->next = _vm_pool;
      _vm_pool->prev = Tail;
      Tail = _vm_pool;
   }
   Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
   // get the PTE so we can check the present bit
   unsigned long * PTE = PTE_address(4096 * _page_no);
   int present = (*PTE & 1);
   if (present == 1)
   {
      // if present then get physical address and convert to frame number
      unsigned long physical_address = ((*PTE >> 12) << 12);
      process_mem_pool->release_frames(physical_address / 4096);
      // Turn off the present bit
      *PTE = *PTE ^ 1;
      // Reload page table to clear the TLB
      load();
   }
   
   Console::puts("freed page\n");
}
