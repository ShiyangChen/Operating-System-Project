#include "scheduler.H"
#include "console.H"
#include "utils.H"
#include "thread.H"
Scheduler::Scheduler()
{
	front=0;
	rear=0;
	tag=0;
}
void Scheduler::add(Thread * _thread)
{
	if((tag==1) && (front==rear))
	{
		Console::puts("Queue is full\n");
		for(;;);
	}
	else
	{
		queue[rear]=_thread;
		rear=(rear+1)%M;
		tag=1;
		spin_wait("add a thread to ready queue\n",5000000);
	}
}

void Scheduler::resume(Thread * _thread)
{
	if((tag==1) && (front==rear))
	{
		Console::puts("Queue is full\n");
		for(;;);
	}
	else
	{
		queue[rear]=_thread;
		rear=(rear+1)%M;
		tag=1;
		spin_wait("resume a thread onto ready queue\n",5000000);
	}
}

void Scheduler::yield()
{
	if((tag==0) && (front==rear))
	{
		Console::puts("Queue is empty\n");
		//for(;;);
	}
	else
	{
		Thread * nexthread=queue[front];
		front=(front+1)%M;
		tag=0;
		spin_wait("A new thread taken out\n",5000000);
		Thread::dispatch_to(nexthread);
	}
}

void Scheduler::terminate(Thread * _thread)
{
	int found = 0;
	
	delete _thread;
}