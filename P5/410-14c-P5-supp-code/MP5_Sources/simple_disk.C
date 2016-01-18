

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "simple_disk.H"
#include "interrupts.H"

/*--------------------------------------------------------------------------*/
/* STATIC MEMBER */
/*--------------------------------------------------------------------------*/

BOOLEAN SimpleDisk::write_lock = FALSE;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

SimpleDisk::SimpleDisk(DISK_ID _disk_id, unsigned int _size) {
	disk_id   = _disk_id;
	disk_size = _size;
}

/*--------------------------------------------------------------------------*/
/* DISK CONFIGURATION */
/*--------------------------------------------------------------------------*/

unsigned int SimpleDisk::size() {
	return disk_size;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void SimpleDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {

	outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
	outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
	outportb(0x1F3, (unsigned char)_block_no); 
                         /* send low 8 bits of block number */
	outportb(0x1F4, (unsigned char)(_block_no >> 8)); 
                         /* send next 8 bits of block number */
	outportb(0x1F5, (unsigned char)(_block_no >> 16)); 
                         /* send next 8 bits of block number */
	outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (disk_id << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

	outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);

}

BOOLEAN SimpleDisk::is_ready() {
	return (inportb(0x1F7) & 0x08);
}

void SimpleDisk::read(unsigned long _block_no, unsigned char * _buf) {
/* Reads 512 Bytes in the given block of the given disk drive and copies them 
   to the given buffer. No error check! */
	issue_operation(READ, _block_no);

	wait_until_ready();

	/* read data from port */
	int i;
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = inportw(0x1F0);
		_buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	}
}

void SimpleDisk::write(unsigned long _block_no, unsigned char * _buf) {
/* Writes 512 Bytes from the buffer to the given block on the given disk drive. */
	while (write_lock) {}   //wait other threads to finish writing
	write_lock = TRUE;   //set the lock
	issue_operation(WRITE, _block_no);
	
	wait_until_ready();

	/* write data to port */
	int i; 
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
		outportw(0x1F0, tmpw);
	}
	write_lock = FALSE;   //free the lock
}



/* DERIVED CLASS -- BLOCKING DISK */


extern Scheduler* SYSTEM_SCHEDULER;
Thread* BlockingDisk::block_queue[QUEUE_SIZE];
int BlockingDisk::head = 0;
int BlockingDisk::tail = 0;

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
	: SimpleDisk(_disk_id, _size) {}

void BlockingDisk::read(unsigned long _block_no, unsigned char* _buf) {
	// put current thread on blocking (device) queue
	block_queue[tail] = Thread::CurrentThread();
	tail = (tail + 1) % QUEUE_SIZE;
	
	
	issue_operation(READ, _block_no);
	
	// any time the thread gains CPU, it checks if the disk is ready, if it's not it will be "stuck" in the loop
	while (!is_ready()) {
		// if not, gives up CPU and wait
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
	}
	
	// if yes, do I/O operation;
	int i;
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = inportw(0x1F0);
		_buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	}
	// after the I/O operation is done
	// dequeue the thread off the block queue
	// dequeue the disk off the disk queue
	head = (head + 1) % QUEUE_SIZE;
}

void BlockingDisk::write(unsigned long _block_no, unsigned char* _buf) {
	while (write_lock) {}   //wait other threads to finish writing
	write_lock = TRUE;   //set the lock
	
	// put current thread on blocking (device) queue
	block_queue[tail] = Thread::CurrentThread();
	tail = (tail + 1) % QUEUE_SIZE;


	issue_operation(WRITE, _block_no);
	
	
	while (!is_ready()) {
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
	}
	
	// if yes, do I/O operation;
	int i; 
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
		outportw(0x1F0, tmpw);
	}
	// after the I/O operation is done
	// dequeue the thread off the block queue
	// dequeue the disk off the disk queue
	head = (head + 1) % QUEUE_SIZE;
	write_lock = FALSE;   //this statement is vital! I used to forget to put it!
}

void BlockingDisk::interrupt_handler(REGS* _regs) {
	// since we apply context switch in the interrupt handler,
	// if the scheduler is in critical section, then the interrupt is not allowed to happen because it will damage the scheduler state unpredictably.
	// So, we check whether the scheduler is in critical section first
	// if so, we simply return and let the blocking thread wait to do I/O operation when it gains CPU in next round.
	if (SYSTEM_SCHEDULER->lock) {
		return;
	}
	// whenever the disk is ready to do I/O operation, this interrupt is trigerred
	Thread* proper_thread = block_queue[head];
	// chances are that the the currently running thread and the thread for which the I/O operation 
	// has been prepared for are actually the same thread!
	// if this is the case, simply return and do nothing (the idea is enlightened by a programmer on stackoverflow)
	
	if (proper_thread == Thread::CurrentThread()) {
		
		return;
	}
	else {
		SYSTEM_SCHEDULER->preempt(proper_thread);
		return;
	}
}


