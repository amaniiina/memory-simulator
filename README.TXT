
Program description:
    This program is a memory management simulator, with a main_memory array of size MEMORY_SIZE. The main memory is divided into segments: data, bss, heap/stack,
    text, and free (unallocated) memory, each segment has a specific number of pages based on it's size that is determined in the main. Pages are also mapped
    in page table with valid and permission bits and each pages' frame. The two main functions are load and store they both add the page corresponding to a given
    address to the appropriate segment in main memory and update the page table accordingly, the load function returns the value at the given address while the store
    function stores the given value at the given address. 


Program files:
sim_mem.hh - header file 
sim_mem.cpp - source code
main.cpp - program tester


Compile and running: 
In Linux terminal:
    To compile: g++ main.cpp -o sim-mem
    To run: ./sim-mem
In Visual Studio Code:
    To compile: Ctrl + shift + b
    To run: Ctrl + f5


Input: 
sim-mem - executable file and swap file.

Output:
sim-mem - print memory segments according to the main.

