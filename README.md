# MeMS Memory Management system
Implement a custom memory management system (MeMS) using the C programming language. MeMS should utilize the system calls mmap and munmap for memory allocation and deallocation, respectively. The system must satisfy the following constraints and requirements outlined below:

## Guidelines :
* MeMS can solely use the system calls mmap and munmap for memory management. The use of any other memory management library functions such as malloc, calloc, free, and realloc are STRICTLY PROHIBITED.
* MeMS should request memory from the OS using mmap in multiples of the system's PAGE_SIZE, which can be determined using the command getconf PAGE_SIZE. For most Linux distributions, the PAGE_SIZE is 4096 bytes (4 KB); however, it might differ for other systems.
* MeMS should deallocate memory only through munmap and deallocation should only occur in multiples of PAGE_SIZE.
* As the value of PAGE_SIZE can differ from system to system hence use the macro “PAGE_SIZE” provided in the template wherever you need the value of PAGE_SIZE in your code so that this size can be modified if required for evaluation purposes.
* The user program must use the functions provided by MeMS for memory allocation and deallocation. It is not allowed to use any other memory management library functions, including malloc, calloc, free, realloc, mmap, and munmap.
* Although MeMS requests memory from the OS in multiples of PAGE_SIZE, it only allocates that much amount of memory to the user program as requested by the user program. MeMS maintains a free list data structure to keep track of the heap memory which MeMS has requested from the OS.
* This free list keeps track of two items:
- memory allocated to each user program. We will call this memory as PROCESS in the free list (details below).
- Memory which has not been allocated to any user program. We will call this memory as a HOLE in the free list (details below).

## Free List Structure
We are using a free list data structure , it consists of MainChainNode(s) which defined as follows
```c
typedef struct MainChainNode {
    void* start_virtual_address; // Start of the memory segment
    size_t total_size; // Total size of the memory segment
    SubChainNode* sub_chain_head; // Head of the sub-chain
    struct MainChainNode* next; //Next MainChainNode
    struct MainChainNode* prev; //Prev MainChainNode
} MainChainNode;
```

Each Main Node consists of SubChainNode(s) which is defined as follows :
```c
typedef struct SubChainNode {
    size_t size;
    int type; // 0 for HOLE, 1 for PROCESS
    void* virtual_address;
    void* physical_address;
    struct SubChainNode* next;
    struct SubChainNode* prev;
} SubChainNode;
```

## Approach
For the Defining the list we have made a Global **head** and to keep track of virtual address we have defined a global pointer.

```c
MainChainNode* head;
void* next_virtual_address;
```

### void mems_init()
In this function we are initialising the **head** and **next_virtual_address**

### void mems_finish()
In this function we are going to each Main Node & it's every Sub Node , and deleting by Unmapping them from the memory.

This Function resets the whole System.

### void* mems_malloc(size_t size)
Allocates memory of the specified size by reusing a segment from the free list if a sufficiently large segment is available. 
Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

there are few cases which has to implemented separately
1. if no node has been implemented , then we first have to give some space to the head and then initialise the needful sub nodes
2. if some nodes are created before hand , we need to first find enough space for the given size. If found just allocate that much memory to the process and to the hole (if needed). Otherwise create separate main node for the new allocations.

Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)

### void mems_print_stats()
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.

we are going to each and every node , thus taking in account of all the processes, holes , used memory , unused memory, number of pages etc.

Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT

###  void* mems_get(void* ptr)
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
here, we are traversing over the freelist data structure and checking if the MeMS virtual address of the node match given address. if so , we returns the MeMS physical address of the desired node.

Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).

### void mems_free(void* ptr)
this function free up the memory pointed by our virtual_address and add it to the free list.
If that particular address is not found we're printing "ADDRESS NOT FOUND!!"

here we traversing over list the free list searching a particular MeMS virtual address , if we got so, we convert that node from process to hole.
we are also taking care that , if we found two adjacent holes. we should club or merge them.

Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing

## Running of program 
We have created a make file 
```bash
all: clean example 

example: example.c mems.h
	gcc -o example example.c

clean:
	rm -rf example
```

To run the program just use these commands
```bash
make
./example
```




