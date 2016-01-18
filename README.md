# Operating-System-Projects
This project consists of five machine problems(MP). 
MP1: 
An easy machine problem just to test development environment. A simple “kernel” is provided to print a welcome text and goes into an infinite loop. Just modify the text on the welcome message to print out your name.

MP2: 
Built a demand-paging based virtual memory system. Set up the paging system and the page table infrastructure for a single address space and extend it to multiple processes, and therefore multiple address spaces. Implemented a frame manager, which manages and allocates frames to address spaces.

MP3: 
extended the page table management in MP2 to support very large numbers and sizes of address spaces. For this, I placed the page
table into virtual memory. This will slightly complicate the page table management and the design of the page fault handlers. Implemented a simple memory allocator and hook it up to the new and delete operators of C++.

MP4:
Built a round-robin fashion multi-thread scheduler which support low-level context switch and interrupt handler.

MP5:
Built a non-blocking disk driver on a mirrored-disk model and built on it an inode fashion filesystem which supports add, delete, overwrite, append of a file.
