#include "frame_pool.H"
#include "Console.H"



FramePool::FramePool(unsigned long _base_frame_no,unsigned long _nframes,unsigned long _info_frame_no)
{
	startframe_no=_base_frame_no;
	frame_no=_nframes;
	infoframe_no=_info_frame_no;
	//unsigned long e_index;
	//unsigned long e_offset;
	unsigned long i;
	if(frame_no%32)
		entry_no=frame_no/32+1;
	else entry_no=frame_no/32;
	
	if(!infoframe_no)
	{
		bitmap=(unsigned long *)0x200000;
		bitmap[0]=1;
		for(i=1;i<=entry_no-1;i++)
			bitmap[i]=0;
	}
	else
	{
		bitmap=(unsigned long *)(infoframe_no * 4096);
		for(i=0;i<=entry_no-1;i++)
			bitmap[i]=0;
	}
}

unsigned long FramePool::get_frame()
{
	unsigned long e_index,e_offset;
	unsigned long last_e_offset=frame_no%32;   //分两种情况，有余数的话才区分最后一行，没余数直接从头扫到尾
	for(e_index=0;e_index<=entry_no-2;e_index++)
		for(e_offset=0;e_offset<=31;e_offset++)
			if((bitmap[e_index]>>e_offset)==0)
			{
				bitmap[e_index]=(1<<e_offset) | bitmap[e_index];
				unsigned long frame_offset=e_index*32+e_offset;
				return frame_offset+startframe_no;
			}
	if(last_e_offset) //此时已经扫描到最后一个entry，有余数
	{	
		for(e_offset=0;e_offset<=last_e_offset-1;e_offset++)
			if((bitmap[e_index]>>e_offset)==0)
			{
				bitmap[e_index]=(1<<e_offset) | bitmap[e_index];
				unsigned long frame_offset=e_index*32+e_offset;
				return frame_offset+startframe_no;
			}
		Console::puts("No free frame available \n");
		return 0;
	}
	else //没余数
	{	
		for(e_offset=0;e_offset<=31;e_offset++)
			if((bitmap[e_index]>>e_offset)==0)
				{
					bitmap[e_index]=(1<<e_offset) | bitmap[e_index];
					unsigned long frame_offset=e_index*32+e_offset;
					return frame_offset+startframe_no;
				}
		Console::puts("No free frame available \n");
		return 0;
	}

}

void FramePool::mark_inaccessible(unsigned long _base_frame_no,unsigned long _nframes)
{
	if(_base_frame_no>=startframe_no && _base_frame_no+_nframes<=startframe_no+frame_no)
	{
		unsigned long start_entry=(_base_frame_no-startframe_no)/32;
		unsigned long start_offset=(_base_frame_no-startframe_no)%32;
		unsigned long counter=0;
		unsigned long offset=start_offset;
		unsigned long entry_number=start_entry;
		while(counter<=99)
		{	
			if(offset>31) //此时offset=32
			{
				entry_number++;
				offset=0;
			}
			bitmap[entry_number]=(1<<offset) | bitmap[entry_number];
			counter++;
		}
	}
	else
		Console::puts("frame not belong to this frame pool \n");

}

void FramePool::release_frame(unsigned long frame_index)
{
	;
}

/*void FramePool::frame_init()
{
	for (unsigned long i=0;i<=511;i++)
		indicator[i]=0;
	for (unsigned long i=512;i<=1024-1;i++)
		indicator[i]=1;
}*/

/*#include "frame_pool.H"
#include "console.H"

unsigned long FramePool::bitmap_addr[2];

FramePool::FramePool(unsigned long _base_frame_no,
             unsigned long _nframes,
			 unsigned long _info_frame_no)
{
	base_frame_no=_base_frame_no;
	nframes=_nframes;
	info_frame_no = _info_frame_no;      

	int i = 0;

	if (!info_frame_no)
	{
 		bitmap=(unsigned long *)0x200000;
		bitmap_page = 16;
	 	bitmap[0] = 1;
	    i ++;

	    bitmap_addr[0] = (unsigned long)bitmap;
 	}                                                
 	else
 	{
	 	bitmap=(unsigned long *)(info_frame_no * 4096);
	 	bitmap_page = 224;

	 	bitmap_addr[1] = (unsigned long)bitmap;
	}

	for ( ;i<bitmap_page;i++)
		bitmap[i] = 0;                          
}


unsigned long FramePool::get_frame()
{
	int page, offset;
	for(page = 0; page < bitmap_page; page ++)
	{
		if(bitmap[page] != 0xFFFFFFFF) //not full
			for(offset = 0; offset < 32; offset ++)
			{
				if (!(bitmap[page] >> offset) % 2)
				{
					bitmap[page] |= (1 << offset);
					return page * 32 + offset + base_frame_no;
				}
			}
	}
	//frame_pool is full, reuturn 0
	return 0;
}

void FramePool::mark_inaccessible(unsigned long _base_frame_no,
                          unsigned long _nframes)
{
	int page, offset;
	page = _base_frame_no / 32;
	offset = _base_frame_no % 32;

	for(int i = 0; i < _nframes; i ++)
	{
		bitmap[page] &= (1 << offset);

		if(offset == 32)
		{
			offset = 0;
			page ++;
		}
	}
}


void FramePool::release_frame(unsigned long _frame_no)
{ 
	unsigned long page_no = _frame_no / 4096;
	if(page_no < 512)
	{
		Console::puts("read-only area!");
		return;
	}
	else
	{
		int page, offset;
		unsigned long * bitmap;

		if(page_no < 1024) //kernel memory pool
		{
			page_no -= 512;
			bitmap = (unsigned long *)bitmap_addr[0];
		}
		else // process memory pool
		{
			page_no	-= 1024;
			bitmap = (unsigned long *)bitmap_addr[1];
		}

		page = page_no / 32;
		offset = page_no % 32;

		bitmap[page] &= ~(1 << offset);
	}
}*/