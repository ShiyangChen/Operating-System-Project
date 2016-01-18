#include "page_table.H"
#include "Console.H"
PageTable * PageTable::current_page_table; //initialization for these static variable
unsigned int PageTable::paging_enabled;     /* is paging turned on (i.e. are addresses logical)? */
FramePool * PageTable::kernel_mem_pool;    /* Frame pool for the kernel memory */
FramePool * PageTable::process_mem_pool;   /* Frame pool for the process memory */
unsigned long PageTable::shared_size;        /* size of shared address space */


void PageTable::init_paging(FramePool * _kernel_mem_pool,FramePool * _process_mem_pool,const unsigned long _shared_size)
{
	kernel_mem_pool=_kernel_mem_pool;
	process_mem_pool=_process_mem_pool;
	shared_size=_shared_size;
	Console::puts("init_paging hello \n");
}
PageTable::PageTable()
{
	page_directory=(unsigned long *)(kernel_mem_pool->get_frame()*4096);
	
	*page_directory=((kernel_mem_pool->get_frame())*4096) | 3;  //*page_directory means 1st PDE
	unsigned long * first_pagetable_addr=(unsigned long *) ((*page_directory) & 0xFFFFF000);
	
	for(unsigned long i=0;i<=1023;i++)//start fill in every entries of the first page table
		*(first_pagetable_addr+i)=i*4096 | 3; //3=011 in binary, 011 is a flag,now every entry stores a 32bit value, computer now treat this value as an long integer
	
	for(unsigned long i=1;i<=1023;i++)
		*(page_directory+i)=0;  //set other 1023 entries of page directory as "not present in memory" 
	Console::puts("PageTable Hello \n");
	
}

void PageTable::load()
{
	write_cr3((unsigned long)page_directory);
	//current_page_table = this;
	Console::putui((unsigned int)read_cr3());
	Console::puts("load Hello \n");
}
void PageTable::enable_paging()
{
	write_cr0(read_cr0() | 0x80000000);        //test found if I wrote write_cr0(read_cr0()|0x8000000) as tutorial did, bochs won't function properly
	Console::puts("enable_paging Hello \n");   //this statement help me spot the error, because when bochs don't display "enable_paging Hello", I realize problem came from enable_paging() method
}

void PageTable::handle_fault(REGS * _r)
{
	if(!(_r->err_code & 0x2))
			Console::puts("It's read only.\n");
	if(!(_r->err_code & 0x1)) //if the requested page is not in memory
	{
		unsigned long virtualaddr=read_cr2();
		unsigned long pdindex=virtualaddr>>22;           //we need first 10bit 
		unsigned long * pd=(unsigned long *)read_cr3(); //get physical memory address which is also vm address of page directory
		
		
		Console::putui((unsigned int)pd[pdindex]);
		if((pd[pdindex] & 0x1)==0) //pagetable is not present
		{
			
			pd[pdindex]=(kernel_mem_pool->get_frame()*4096) | 3; //request frame address for storing page table,
															//by calling kernel_mem_pool, we assume pagetable for a process reside in kernel memory
			/*pt=(unsigned long *)(pd[pdindex] & 0xFFFFF000); 
			for(unsigned long i;i<=1023;i++)
				pt[i]=2;*/
		} 
		unsigned long * pt=(unsigned long *)(pd[pdindex] & 0xFFFFF000); 
		//Console::putui((unsigned int)pd[pdindex]);
		unsigned long ptindex=(virtualaddr>>12) & 0x03FF;   //we need next 10bit in the middle
			
		pt[ptindex]=(process_mem_pool->get_frame()*4096) | 3; //request frame address for storing the page
		//Console::putui((unsigned int)pt[ptindex]);
		//for(;;);
	}
	
}
