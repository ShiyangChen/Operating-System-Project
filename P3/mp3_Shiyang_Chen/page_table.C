#include "page_table.H"
#include "Console.H"
#include "paging_low.H"
#include "vm_pool.H"

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
	page_directory=(unsigned long *)(process_mem_pool->get_frame()*4096);
	//Console::putui((unsigned int)page_directory);
	//for(;;);
	page_directory[1023]=((unsigned long)page_directory) | 3;
	
	*page_directory=((process_mem_pool->get_frame())*4096) | 3;  //*page_directory means 1st PDE
	unsigned long * first_pagetable_addr=(unsigned long *) ((*page_directory) & 0xFFFFF000);
	
	for(unsigned long i=0;i<=1023;i++)//start fill in every entries of the first page table
		*(first_pagetable_addr+i)=i*4096 | 3;
	
	for(unsigned long i=1;i<=1022;i++)
		page_directory[i]=2;  //set other 1022 entries of page directory as "not present in memory" 
	//vmarray=new VMPool[10];
	for(int i=0;i<=9;i++)
		vmarray[i]=NULL;
	Console::puts("PageTable Hello \n");
}

void PageTable::load()
{
	write_cr3((unsigned long)page_directory);  
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
	
	if((_r->err_code & 1)==0)  //check if the requested page is in memory or not
	{
		Console::puts("I am here to handle fault \n");
		//for(;;);
		unsigned long * pagetable_phyaddr;
		unsigned long * pagetable_vaddr;
		unsigned long virtualaddr=read_cr2();
		Console::putui((unsigned int)virtualaddr);
		//for(;;);
		unsigned long pdindex=virtualaddr>>22;           //we need first 10bit
		unsigned long ptindex=(virtualaddr>>12) & 0x03FF;   //we need next 10bit in the middle
		
		unsigned long * pd=(unsigned long *)0xFFFFF000;	 //0xFFFFF000(virtual memory) refers to the first PDE in page_directory
		pagetable_vaddr=(unsigned long *)(0xFFC00000 + (pdindex * 4096));
		if((pd[pdindex] & 1)==0)  //if PDE is null, ie, a page table dosen't exist
		{
			pagetable_phyaddr=(unsigned long *)(process_mem_pool->get_frame()*4096);
			Console::putui((unsigned int)pagetable_phyaddr);
			//for(;;);
			pd[pdindex]=((unsigned long)pagetable_phyaddr) | 3;
			Console::putui((unsigned int)pd[pdindex]);
			//for(;;);
			for(unsigned long i=0;i<=1023;i++)
				pagetable_vaddr[i]=2;
		}																	
		
		if((pagetable_vaddr[ptindex] & 1)==0)
		{
			pagetable_vaddr[ptindex]=(process_mem_pool->get_frame()*4096) | 3;
			Console::putui((unsigned int)pagetable_vaddr);
			Console::putui((unsigned int)pagetable_vaddr[ptindex]);
		}
		else Console::puts("bad happen \n");
		//for(;;);
	}
}


void PageTable::free_page(unsigned long _page_no)
{
	Console::puts("I am to free page\n");
	unsigned long virtualaddr=_page_no*4096;
	unsigned long pdindex=virtualaddr>>22;
	unsigned long ptindex=(virtualaddr>>12) & 0x03FF;
	Console::putui((unsigned int)pdindex);
	Console::putui((unsigned int)ptindex);
	unsigned long frame_phyaddr;
	unsigned long * pagetable_vaddr;
	unsigned long * pd=(unsigned long *)0xFFFFF000;
	Console::putui((unsigned int)(pd[pdindex]));
	//for(;;);
	if((pd[pdindex] & 1)==0)  //if PDE is null, meaning this page has been mapped, thus cannot freed	
		Console::puts("this PageTable doesn't exist \n");
	else
	{
		pagetable_vaddr=(unsigned long *)(0xFFC00000 + (pdindex * 4096));
		Console::putui((unsigned int)pagetable_vaddr);
		Console::putui((unsigned int)pagetable_vaddr[ptindex]);
		//for(;;);
		if((pagetable_vaddr[ptindex] & 1)==0)
			Console::puts("this page hasn't been mapped\n");
		else
		{
			frame_phyaddr=pagetable_vaddr[ptindex] & 0xFFFFF000; //brush off low 12bits, the high 20 bits are physical address of a frame
			FramePool::release_frame(frame_phyaddr/4096);  //release mapped frame
			Console::putui((unsigned int)(frame_phyaddr/4096));
			pagetable_vaddr[ptindex] =0x2;   //update PTE
			
			Console::putui((unsigned int)pagetable_vaddr[ptindex]);
			//for(;;);
		}
	}
}

void PageTable::register_vmpool(VMPool *_pool)
{
	for(int i=0;i<=9;i++) //the maximum virtualmemory pool one pagetable can relate is ten
		if(vmarray[i]==NULL)
		{
			vmarray[i]=_pool;
			return;
		}
	Console::puts("no more VMPool can be related with this pagetable\n");
}