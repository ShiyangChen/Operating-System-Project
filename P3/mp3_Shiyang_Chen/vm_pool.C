#include "vm_pool.H"
#include "Console.H"
#include "page_table.H"
//#include "stdlib.h"
//#include <iostream>
//using namespace std;
Regions::Regions(unsigned long _startaddr,unsigned long _number_bytes)
{
	startaddr=_startaddr;    //startaddr is in bytes
	number_bytes=_number_bytes;
	number_pages=number_bytes/4096+1;
	lastRegion=NULL;
	nextRegion=NULL;

}
VMPool::VMPool(unsigned long _base_address, unsigned long _size, FramePool *_frame_pool_1, FramePool *_frame_pool_2, PageTable *_page_table)
{
	base_address=_base_address;
	size=_size;
	process_mem_pool=_frame_pool_1;
	kernel_mem_pool=_frame_pool_2;
	page_table=_page_table;
	pt_firstRegion=NULL;
	if(size%4096)
		total_pages=size/4096+1;
	else
		total_pages=size/4096;
	unsigned long info_frame_no;
	if(total_pages%4096)
		info_frame_no=total_pages/4096+1;
	else
		info_frame_no=total_pages/4096;
	
	indicator=(bool *)kernel_mem_pool->get_frame();
	for(unsigned long i=1;i<=info_frame_no-1;i++)  //分配状态信息所需要的frame
		kernel_mem_pool->get_frame();
	
	page_table->register_vmpool(this); //
	//Console::puts("New VMPool created!\n");
	
	//indicator=new bool[total_pages];
	//indicator=(bool *)malloc(sizeof(bool)*total_pages);
	Console::puts("New VMPool created!\n");
	for (int i=0;i<=total_pages-1;i++)
		indicator[i]=0;  //'0' means the page is free and available, '1' means the page has been allocated thus unavailable
	//Console::puts("New VMPool created!\n");
}
VMPool::VMPool()
{
}

unsigned long VMPool::allocate(unsigned long _size)   //_size is number of bytes, not number of pages
{
	unsigned long pages_needed;
	//_size=_size*4;    //其实是在装整数
	if(_size%4096)
		pages_needed=_size/4096+1;
	else
		pages_needed=_size/4096;
	unsigned long fpindex,lpindex; //fpindex=the index of the page where the first free byte reside in, it is not the first free page though
	unsigned long counter=0;
	if(pt_firstRegion==NULL) //no region has been alliocated, a special case for initialization
	{
		Console::puts("i'm here to allocate");
		pt_firstRegion=(Regions *)(kernel_mem_pool->get_frame()*4096);
		//Console::puts("pt_firstRegion");
		//Console::putui((unsigned int)pt_firstRegion);
		Regions z(base_address,_size);  //z 在栈上所以函数一结束z就没了，但pt_firstRegion还在
		
		pt_firstRegion->startaddr=z.startaddr;
		pt_firstRegion->number_bytes=z.number_bytes;
		pt_firstRegion->number_pages=z.number_pages;
		pt_firstRegion->lastRegion=z.lastRegion;
		pt_firstRegion->nextRegion=z.nextRegion;
		//Console::putui((unsigned int)pt_firstRegion);
		//for(;;);
		
		for (int i=0;i<=pages_needed-1;i++)
			indicator[i]=1;
		return base_address;
	}
	if(_size<=(pt_firstRegion->startaddr)-base_address+1)  //First-Fit
	{	
		fpindex=0;
		lpindex=(pt_firstRegion->startaddr-1-base_address)/4096;
		for (int i=fpindex;i<=lpindex;i++)
			if(!indicator[i])
				counter=counter+1;   //compute total number of free pages 
		if(counter>=pages_needed)
		{
			Regions * p=pt_firstRegion;
			pt_firstRegion=new Regions(base_address,_size);
			pt_firstRegion->nextRegion=p;
			p->lastRegion=pt_firstRegion;
			for (int i=0;i<=pages_needed-1;i++)
				indicator[i]=1;
			return base_address;
		}
	}
	Regions * z=pt_firstRegion;  //if two conditions above are not satisfied, then jump over intervals between allocated regions
	while(z->nextRegion!=NULL)
	{
		counter=0;
		//unsigned long intervals=(z->nextRegion)->startaddr-(z->startaddr)-(z->number_bytes);
		fpindex=(z->startaddr+z->number_bytes-base_address)/4096;
		lpindex=((z->nextRegion)->startaddr-1-base_address)/4096;
		for (int i=fpindex;i<=lpindex;i++)
			if(!indicator[i])
				counter=counter+1;   //compute total number of free pages
		if(counter>=pages_needed)
		{
			if(indicator[fpindex])  //this is the most case
			{
				for(int i=fpindex+1;i<=fpindex+pages_needed;i++)
					indicator[i]=1;
				Regions * r=new Regions((fpindex+1)*4096+base_address,_size);
				r->nextRegion=z->nextRegion;   //insert new region into two existing regions, so regions are ordered by their startaddr
				(z->nextRegion)->lastRegion=r;
				z->nextRegion=r;
				r->lastRegion=z;
				return base_address+(fpindex+1)*4096;
			}
			else
			{
				for(int i=fpindex;i<=fpindex+pages_needed-1;i++)
					indicator[i]=1;
				Regions * r=new Regions(fpindex*4096+base_address,_size);
				r->nextRegion=z->nextRegion;
				(z->nextRegion)->lastRegion=r;
				z->nextRegion=r;
				r->lastRegion=z;
				return base_address+fpindex*4096;
			}
		}
		else   //the current interval cannot fulfill the need, we jump to next interval
			z=z->nextRegion;

	}
	counter=0;
	fpindex=(z->startaddr+z->number_bytes-base_address)/4096;	//now z points to the last allocated region
	lpindex=(size-1)/4096;
	for (int i=fpindex;i<=lpindex;i++)
			if(!indicator[i])
				counter=counter+1;
	if(counter>=pages_needed)
	{
		if(indicator[fpindex])  //this is the most case
		{
			for(int i=fpindex+1;i<=fpindex+pages_needed;i++)
				indicator[i]=1;
			Regions * r=new Regions((fpindex+1)*4096+base_address,_size);  
			z->nextRegion=r;      //put the new region at the end of the linkedlist
			r->lastRegion=z;
			return base_address+(fpindex+1)*4096;
		}
		else
		{
			for(int i=fpindex;i<=fpindex+pages_needed-1;i++)
				indicator[i]=1;
			Regions * r=new Regions(fpindex*4096+base_address,_size);
			z->nextRegion=r;
			r->lastRegion=z;
			return base_address+fpindex*4096;
		}
	}
	else //at this point no interval between regions or the first or last free space can meet the demand, so return 0
		return 0xFFFFFFFF;
}

void VMPool::release(unsigned long _start_address)  //缺一种情况：最后一个region被释放了，此时pt_firstRegion=NULL
{
	unsigned long fpindex; //means the page index in which _address reside in
	unsigned long page_number; // actual page number in whole virtual memory
	Regions * z=pt_firstRegion;
	Console::puts("Rock a Roll \n");
	//for(;;);
	if (z==NULL)
		Console::puts("no region has been allocated, thus cannot released \n");
	while(z->nextRegion!=NULL)
	{
		Console::puts("it's a trap \n");
		if(z->startaddr==_start_address)
		{
			fpindex=(z->startaddr-base_address)/4096;
			for(int i=fpindex;i<=fpindex+z->number_pages-1;i++)
			{
				indicator[i]=0;
				page_number=fpindex+base_address/4096;
				page_table->free_page(page_number);
			}
			(z->nextRegion)->lastRegion=z->lastRegion;
			(z->lastRegion)->nextRegion=z->nextRegion;
			delete z;
			return;   
		}
		else
			z=z->nextRegion;
	}
	/*at this moment, z points to the last region*/
	if(z->startaddr==_start_address)
	{
		Console::puts("I am glad \n");
		//for(;;);
		fpindex=(z->startaddr-base_address)/4096;
		Console::putui((unsigned int)fpindex);
		Console::putui((unsigned int)z->number_pages);
		//for(;;);
		for(int i=fpindex;i<=fpindex+z->number_pages-1;i++)
		{
			indicator[i]=0;
			page_number=i+base_address/4096;
			Console::putui((unsigned int)base_address);
			Console::putui((unsigned int)page_number);
			page_table->free_page(page_number);
		}
		//unsigned long * pd=(unsigned long *)0xFFFFF000;
		//pd[128]=2;
		if(z->lastRegion==NULL)   //这代表其实只有1个region
		{
			Console::puts("last step to release \n");
			//for(;;);
			FramePool::release_frame(((unsigned long)pt_firstRegion)/4096);   //这句话也有问题，可能要release多个frame
			pt_firstRegion=NULL;
			return;
		}
		//(z->lastRegion)->nextRegion=NULL;
		
		//return;
	}
	else  //if the last region doesn't match
		Console::puts("no such a region with a start_address you specified \n");
}


BOOLEAN VMPool::is_legitimate(unsigned long _address)
{
	if(_address<base_address || _address>base_address+size-1)
		return 0;
	else 
	{
		int i=(_address-base_address)/4096;  //compute the index of the corresponding page
		if(indicator[i])   //has been allocated thus reference to it is valid
		{
			//Console::puts("I am here \n");
			//for(;;);
			return 1;
		}
		else
			return 0;
	}
}
