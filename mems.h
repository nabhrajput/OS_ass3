/*
All the main functions with respect to the MeMS are inplemented here
read the function discription for more details

NOTE: DO NOT CHANGE THE NAME OR SIGNATURE OF FUNCTIONS ALREADY PROVIDED
you are only allowed to implement the functions 
you can also make additional helper functions a you wish

REFER DOCUMENTATION FOR MORE DETAILS ON FUNSTIONS AND THEIR FUNCTIONALITY
*/
// add other headers as required
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include <sys/mman.h>


/*
Use this macro where ever you need PAGE_SIZE.
As PAGESIZE can differ system to system we should have flexibility to modify this 
macro to make the output of all system same and conduct a fair evaluation. 
*/
#define PAGE_SIZE 4096

typedef struct SubChainNode {
    size_t size;
    int type; // 0 for HOLE, 1 for PROCESS
    void* virtual_address;
    struct SubChainNode* next;
    struct SubChainNode* prev;
} SubChainNode;

// Structure for the main chain nodes
typedef struct MainChainNode {
    void* start_virtual_address; // Start of the memory segment
    size_t total_size; // Total size of the memory segment
    SubChainNode* sub_chain_head; // Head of the sub-chain
    struct MainChainNode* next;
    struct MainChainNode* prev;
} MainChainNode;

MainChainNode* head;
void* next_virtual_address;

/*
Initializes all the required parameters for the MeMS system. The main parameters to be initialized are:
1. the head of the free list i.e. the pointer that points to the head of the free list
2. the starting MeMS virtual address from which the heap in our MeMS virtual address space will start.
3. any other global variable that you want for the MeMS implementation can be initialized here.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_init() {
    head = mmap(NULL,sizeof(MainChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    head ->next = NULL;
    head -> prev = NULL;
    head -> start_virtual_address = 0;
    head -> total_size = 0;
    head -> sub_chain_head = NULL;
    next_virtual_address = 0;
}



/*
This function will be called at the end of the MeMS system and its main job is to unmap the 
allocated memory using the munmap system call.
Input Parameter: Nothing
Returns: Nothing
*/
void mems_finish() {
    MainChainNode* main_node = head;
    MainChainNode* temp_main;

    while (main_node != NULL) {
        SubChainNode* sub_node = main_node->sub_chain_head;
        SubChainNode* temp_sub;

        while (sub_node != NULL) {
            // Unmap the memory segment and free the SubChainNode
            temp_sub = sub_node;
            sub_node = sub_node->next;
            munmap(temp_sub->virtual_address, temp_sub->size);
            free(temp_sub);
        }

        // Free the MainChainNode
        temp_main = main_node;
        main_node = main_node->next;
        free(temp_main);
    }

    head = NULL; // Reset the MeMS system
}



/*
Allocates memory of the specified size by reusing a segment from the free list if 
a sufficiently large segment is available. 

Else, uses the mmap system call to allocate more memory on the heap and updates 
the free list accordingly.

Note that while mapping using mmap do not forget to reuse the unused space from mapping
by adding it to the free list.
Parameter: The size of the memory the user program wants
Returns: MeMS Virtual address (that is created by MeMS)
*/ 
void* mems_malloc(size_t size){
    size_t mandatory = 0;
    while(mandatory < size){
        mandatory += PAGE_SIZE;
    }

    size_t need = size;
    size_t extra = mandatory - need;

    if(head->sub_chain_head == NULL){ //if memory is empty 
        printf("Adding first node \n");
        if(extra != 0){ 
            SubChainNode* first = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            first -> next = NULL;
            first -> prev = NULL;
            first -> size = need;
            first -> virtual_address = next_virtual_address; //virtual address is a pointer , it will store address of the current 
            first -> type = 1; // Process

            next_virtual_address = next_virtual_address + need;


            SubChainNode* second = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            second -> next =  NULL;
            second -> prev = first;
            second -> size = extra;
            second -> virtual_address = next_virtual_address;
            second -> type = 0;

            next_virtual_address = next_virtual_address + extra;
            first -> next = second;
            second->prev = first;

            head ->sub_chain_head = first;
            head -> total_size += mandatory;
            
            return first->virtual_address;
        }
        else{ //extra = 0
            SubChainNode* first = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            first -> next = NULL;
            first -> prev = NULL;
            first -> size = need;
            first -> virtual_address = next_virtual_address; //virtual address is a pointer , it will store address of the current 
            first -> type = 1; // Process

            next_virtual_address = next_virtual_address + need;

            head ->sub_chain_head = first;
            head -> total_size += mandatory;
            
            return first->virtual_address;
        }
    }

    else{ //if there is already some memory present
        /* If enough memory present , give enough memory and update free list
        */
        MainChainNode* where_to_make_new = head;
        MainChainNode* main_temp = head;
        while(main_temp != NULL){
            SubChainNode* sub_temp = main_temp->sub_chain_head;
            while(sub_temp != NULL){
                if((sub_temp->type == 0) && (sub_temp->size <= need)){
                    printf("Node Found \n");
                    //you found enough free space
                    if(extra != 0){
                        //memory actually needed 
                        SubChainNode* new_occupied = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                        new_occupied->next = NULL;
                        new_occupied->prev = NULL;
                        new_occupied->size = need;
                        new_occupied->type = 1;
                        new_occupied->virtual_address = sub_temp->virtual_address;

                        //extra memory that is not used
                        SubChainNode* new_free = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                        new_free -> next = NULL;
                        new_free -> prev = NULL;
                        new_free -> size = (sub_temp->size) - (new_occupied->size);
                        new_free -> type = 0;
                        new_free -> virtual_address = (sub_temp->virtual_address) + (new_occupied->size); 

                        new_occupied ->next = new_free;
                        new_free ->prev = new_occupied;

                        new_occupied ->prev = sub_temp -> prev;
                        new_free ->next = sub_temp ->next;

                        sub_temp ->prev ->next = new_occupied;
                        if(sub_temp -> next != NULL){
                            sub_temp ->next->prev = new_free;
                        }

                        munmap(sub_temp,sub_temp->size);

                        return new_occupied->virtual_address;
                    }
                    else{ //extra = 0
                        SubChainNode* new_occupied = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                        new_occupied->next = NULL;
                        new_occupied->prev = NULL;
                        new_occupied->size = need;
                        new_occupied->type = 1;
                        new_occupied->virtual_address = sub_temp->virtual_address;

                        new_occupied ->next = sub_temp->next;
                        new_occupied ->prev = sub_temp ->prev;
                        if(sub_temp -> next != NULL){
                            sub_temp ->next->prev = new_occupied;
                        } 
                        sub_temp->prev->next = new_occupied;

                        munmap(sub_temp,sub_temp->size);

                        return new_occupied->virtual_address;

                    }
                }
                sub_temp = sub_temp->next;
            }
            where_to_make_new = main_temp;
            main_temp = main_temp->next;
        } 

        // it will be executed when no free space found
        // new node should be created at the end

        printf("Adding new Main node \n ");
        MainChainNode* new_main = mmap(NULL,sizeof(MainChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        new_main->next = NULL;
        new_main->prev = where_to_make_new;
        where_to_make_new ->next = new_main;
        new_main ->start_virtual_address = next_virtual_address;
        new_main->total_size = mandatory;
        

        //reached the last node
        if(extra != 0){ 
            SubChainNode* first = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            first -> next = NULL;
            first -> prev = NULL;
            first -> size = need;
            first -> virtual_address = next_virtual_address; //virtual address is a pointer , it will store address of the current 
            first -> type = 1; // Process

            next_virtual_address = next_virtual_address + need;


            SubChainNode* second = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            second -> next =  NULL;
            second -> prev = first;
            second -> size = extra;
            second -> virtual_address = next_virtual_address;
            second -> type = 0;

            next_virtual_address = next_virtual_address + extra;
            first -> next = second;
            second ->prev = first;

            new_main->sub_chain_head = first;
            first->prev = NULL;

            
            return first->virtual_address;
        }

        else{ //extra = 0
            SubChainNode* first = mmap(NULL,sizeof(SubChainNode), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            first -> next = NULL;
            first -> prev = NULL;
            first -> size = need;
            first -> virtual_address = next_virtual_address; //virtual address is a pointer , it will store address of the current 
            first -> type = 1; // Process

            next_virtual_address = next_virtual_address + need;
            new_main->sub_chain_head = first;;
            
            return first->virtual_address;
        }
    }
}


/*
this function print the stats of the MeMS system like
1. How many pages are utilised by using the mems_malloc
2. how much memory is unused i.e. the memory that is in freelist and is not used.
3. It also prints details about each node in the main chain and each segment (PROCESS or HOLE) in the sub-chain.
Parameter: Nothing
Returns: Nothing but should print the necessary information on STDOUT
*/
void mems_print_stats() {
    MainChainNode* main_node = head;
    size_t total_mapped_pages = 0;
    size_t total_unused_memory = 0;

    while (main_node != NULL) {
        SubChainNode* sub_node = main_node->sub_chain_head;

        while (sub_node != NULL) {
            // Print information about the sub-chain node
            printf("Virtual Address: %p, Size: %zu, Type: %s\n", sub_node->virtual_address, sub_node->size, sub_node->type == 0 ? "HOLE" : "PROCESS");

            // Calculate total unused memory (HOLE)
            if (sub_node->type == 0) {
                total_unused_memory += sub_node->size;
            }

            sub_node = sub_node->next;
        }

        total_mapped_pages += main_node->total_size / PAGE_SIZE;

        main_node = main_node->next;
    }

    // Print total mapped pages and unused memory
    printf("Total Mapped Pages: %zu\n", total_mapped_pages);
    printf("Total Unused Memory: %zu bytes\n", total_unused_memory);
}



/*
Returns the MeMS physical address mapped to ptr ( ptr is MeMS virtual address).
Parameter: MeMS Virtual address (that is created by MeMS)
Returns: MeMS physical address mapped to the passed ptr (MeMS virtual address).
*/
void *mems_get(void* v_ptr){
    MainChainNode* temp_main = head;
    printf("Something's happening in Mainchain");
    while(temp_main != NULL){
        SubChainNode* temp_sub = head->sub_chain_head;
        printf("Something's happening in Subchain");
        while(temp_sub != NULL){
            if(temp_sub->virtual_address == v_ptr){
                printf("Something's happening");
                printf("Virtual Address %p found at %p\n",temp_sub->virtual_address,temp_sub);
                return temp_sub;
            }
            temp_sub = temp_sub->next;
        }
        temp_main=temp_main->next;
    }
    printf("Address not found !!");
    return NULL;
}


/*
this function free up the memory pointed by our virtual_address and add it to the free list
Parameter: MeMS Virtual address (that is created by MeMS) 
Returns: nothing
*/
void mems_free(void* ptr) {
    MainChainNode* main_node = head;
    SubChainNode* sub_node;

    // Find the sub-chain node associated with the given MeMS virtual address (ptr)
    while (main_node != NULL) {
        sub_node = main_node->sub_chain_head;
        while (sub_node != NULL) {
            if (sub_node->virtual_address == ptr) {
                // Mark the segment as HOLE
                sub_node->type = 0;

                // Try to club this HOLE with adjacent HOLEs
                SubChainNode* prev_hole = sub_node->prev;
                SubChainNode* next_hole = sub_node->next;

                // Club with the previous HOLE if it exists and is not PROCESS
                if (prev_hole != NULL && prev_hole->type == 0) {
                    prev_hole->size += sub_node->size;
                    prev_hole->next = next_hole;
                    if (next_hole != NULL) {
                        next_hole->prev = prev_hole;
                    }
                    munmap(sub_node, sub_node->size);
                }

                // Club with the next HOLE if it exists and is not PROCESS
                if (next_hole != NULL && next_hole->type == 0) {
                    sub_node->size += next_hole->size;
                    sub_node->next = next_hole->next;
                    if (next_hole->next != NULL) {
                        next_hole->next->prev = sub_node;
                    }
                    munmap(next_hole, next_hole->size);
                }

                return; // Done with freeing and clubbing
            }
            sub_node = sub_node->next;
        }
        main_node = main_node->next;
    }
}
