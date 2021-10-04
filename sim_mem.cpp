#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <array>
#include <string.h>
#include "sim_mem.hh"
using namespace::std;
char main_memory[MEMORY_SIZE]; 
bool fullMem = false;   // boolean for unallocated main memory
int startIndex=0;       // starting index for each segment of main memory
int physical_address=0; // address in main memory
int addedToBss=-1, addedToData=-1, addedToHeapStack=-1;    //number of pages added to each segment
int addedPages=-1;         // num of pages added to the unallocated segment main memory
int currPageNum=-1;        // number of currently requested page

sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size, 
            int bss_size, int heap_stack_size, int num_of_pages, int page_size){
    // initialize sizes
    this->text_size= text_size;
    this->data_size= data_size;
    this->bss_size= bss_size;
    this->heap_stack_size= heap_stack_size;
    this->num_of_pages= num_of_pages;
    this->page_size= page_size;
    // set main memory to 0
    for(int i=0; i<MEMORY_SIZE; i++){
        main_memory[i]='0';
    }
    // initialize page table and all its fields
    this->page_table= new page_descriptor[num_of_pages];
    if(this->page_table == NULL){
        perror("page table alloc failed\n");
        exit(1);
    }
    for(int i=0; i< num_of_pages; i++){
        page_table[i].frame = -1;
        page_table[i].D= 0;
        if(i%2==0){     //initialize page table read/write permissions
            page_table[i].P= 1;
        } else{
            page_table[i].P= 0; 
        }
        page_table[i].V= 0;
    }
    // open executable file for reading
    program_fd= open(exe_file_name, O_RDONLY);
    if(program_fd<0){
        perror("exe open failed\n");
        exit(1);
    }
    // open swapfile fot read and write, create it if it doesn't exist
    swapfile_fd= open(swap_file_name, O_RDWR | O_CREAT);
    if(swapfile_fd<0){
        perror("swapfile open failed\n");
        exit(1);
    }
    // initialize swapfile with zeros
    int buff[page_size * num_of_pages]={0};
    if(write(swapfile_fd, &buff, page_size*num_of_pages) < 0){
        perror("initilizing swapfile to 0 failed\n");
        exit(1);
    }
    startIndex= bss_size+ heap_stack_size+ data_size+ text_size;
    maxFrames= (MEMORY_SIZE - startIndex)/page_size;
    data_frames= data_size/page_size;
    pageAtFrame= new int[maxFrames];  
    page= new char[page_size]; 
    inData= new int [data_frames];   
    for(int i=0; i<data_frames; i++){
        inData[i]= -1;
    }
    bss_frames= bss_size/page_size;
    heap_stack_frames= heap_stack_size/page_size;
}

void sim_mem::swap(int frameToEmpty){
    int swappedPageNum= pageAtFrame[frameToEmpty];
    // free space in main memory by copying page to swapfile 
    if(pwrite(swapfile_fd, main_memory+ physical_address, page_size, page_size*swappedPageNum) < 0){
        perror("writing to swapfile failed\n");
        exit(1);
    }
    page_table[swappedPageNum].V=0;
    page_table[swappedPageNum].frame=-1;
    page_table[swappedPageNum].D=1;
}

void sim_mem::addToPageTable(int added){
    page_table[currPageNum].frame= added; // current frame is equal to number of pages added
    int physaddr= physical_address;
    strncpy(main_memory+physical_address, page, page_size);
    physical_address= physaddr;
    page_table[currPageNum].V=1;
    pageAtFrame[added]=currPageNum;
}

void sim_mem::getFromExe(int added){
    if(pread(program_fd, page, page_size, page_size*currPageNum) < 0){
        perror("read failed\n");
        return;
    }
    if(fullMem){
        swap(added);
    }
    addToPageTable(added);
}

char sim_mem::load(int address){
    if(address < 0 || address > page_size*num_of_pages){
        perror("Invalid address\n");
        return '\0';
    }

    int offset;
    currPageNum= address / page_size;
    offset= address % page_size;
    startIndex= bss_size+ heap_stack_size+ data_size+ text_size;

    if(page_table[currPageNum].V == 1){ //page is in main memory
        physical_address= startIndex + (page_size* page_table[currPageNum].frame);
        return main_memory[physical_address+ offset]; 
    }

    else{   // page not in main memory
        if(addedPages+1 < maxFrames)
            addedPages++; 
        else if( addedPages+1 == maxFrames){
            addedPages=0;
            fullMem=true;
        }

        if( page_table[currPageNum].P == 0){    // read only. page is in executable (0- text)
            physical_address= startIndex + (page_size*addedPages);
            getFromExe(addedPages);
            return main_memory[physical_address+offset];
        } 

        else{   // P = 1    read and write
            if( page_table[currPageNum].D ==1){ // dirty page bring from swap
                if(pread(swapfile_fd, page, page_size, page_size*currPageNum) < 0){
                    addedPages--;
                    perror("read failed\n");
                    return '\0';
                }
                if(fullMem){
                    swap(addedPages);
                }
                physical_address= startIndex + (page_size*addedPages);
                addToPageTable(addedPages);
                return main_memory[physical_address+offset];
            }
            else{   // D = 0
                // check data or bss/stack/heap
                if(currPageNum>=0 && currPageNum<data_frames ) {  //1- data 
                    addedPages--;
                    for(int i=0; i< data_frames; i++){
                        if(inData[i]== currPageNum ){
                            physical_address= startIndex + (page_size*page_table[currPageNum].frame);
                            return main_memory[physical_address+offset];
                        }
                    }
                    if(addedToData < data_frames){
                        addedToData++;
                        inData[addedToData]=currPageNum;
                        startIndex= 0; //data start
                        physical_address= startIndex + (page_size*addedToData);
                        getFromExe(addedToData);
                        return main_memory[physical_address+offset];
                    }else{
                        perror("data overflow\n");
                        return '\0';
                    }
                }
                else { // 2-bss, 3-heap and stack
                    addedPages--;
                    perror("no allocated memory\n");
                    return '\0';
                }
            }
        }
    }
}

void sim_mem::store(int address, char value){
    if(address < 0 || address > page_size*num_of_pages){
        perror("Invalid address\n");
        return;
    }
    int offset;
    currPageNum= address / page_size;
    offset= address % page_size;

    if(page_table[currPageNum].V == 1){ //page is in main memory
        for(int i=0; i< data_frames; i++){
            if(inData[i]== currPageNum ){
                startIndex=0;
                physical_address= startIndex + (page_size*page_table[currPageNum].frame);
                main_memory[physical_address+offset]= value;
                return;
            }
        }
            startIndex= bss_size+ heap_stack_size+ data_size+ text_size;
            physical_address= (startIndex + (page_size* page_table[currPageNum].frame));
            main_memory[physical_address+ offset]= value; 
    }

    else{   // page not in main memory
        if(addedPages+1 < maxFrames)
            addedPages++; 
        else if( addedPages+1 == maxFrames){
            addedPages=-1;
            fullMem=true;
        }

        if( page_table[currPageNum].P == 0){    // read only. page is in executable (text)
            addedPages--;
            perror("No write permissions\n");
            return;
        } 
        else{   // P = 1    read and write
            if( page_table[currPageNum].D ==1){ // dirty page bring from swap
                if(pread(swapfile_fd, page, page_size, page_size*currPageNum) < 0){
                    addedPages--;
                    perror("read failed\n");
                    return;
                }
                if(fullMem){
                    swap(addedPages);
                }
                addToPageTable(addedPages);
                main_memory[physical_address+offset]=value;
            }
            else{   // D = 0
                // check data or bss/stack/heap
                addedPages--;
                if(currPageNum>=0 && currPageNum<data_frames){ // in data
                    for(int i=0; i< data_frames; i++){
                        if(inData[i]== currPageNum ){
                            startIndex=0;
                            physical_address= startIndex + (page_size*page_table[currPageNum].frame);
                            main_memory[physical_address+offset]= value;
                            return;
                        }
                    }
                    if(addedToData < data_frames){
                        addedToData++;
                        inData[addedToData]=currPageNum;
                        startIndex= 0;
                        physical_address= startIndex + (page_size* addedToData);
                        getFromExe(addedToData);
                        main_memory[physical_address+offset]= value;
                    }else{
                        perror("data overflow\n");
                        return;
                    }
                }
                // else bss, heap and stack -> allocate new page
                else if(value == NULL && currPageNum>data_frames && currPageNum<(data_frames+bss_frames)){ // in bss
                    startIndex= data_size; // bss start
                    if(addedToBss < bss_frames){
                        addedToBss++;
                        physical_address= startIndex + (page_size* addedToBss);
                        for(int i=physical_address; i<page_size; i++){
                            main_memory[i]= '0';
                        }
                    }else{
                        perror("bss overflow\n");
                        return;
                    }
                }
                else if(currPageNum>data_frames+bss_frames && currPageNum<(data_frames+bss_frames+heap_stack_frames)){ // in heap stack
                    startIndex= bss_size + data_size; //heap stack start
                    if(addedToHeapStack < heap_stack_frames){
                        addedToHeapStack++;
                        physical_address= startIndex + (page_size* addedToHeapStack);
                        for(int i=physical_address; i<page_size; i++){
                            main_memory[i]= '0';
                        }
                    }else{
                        perror("heap/stack overflow\n");
                        return;
                    }
                }else{
                    startIndex= bss_size+ heap_stack_size+ data_size+ text_size;
                    physical_address= startIndex + (page_size*addedPages);
                    getFromExe(addedPages);
                    main_memory[physical_address+offset]= value;
                }
            }
        }
    }
}

sim_mem::~sim_mem (){
//close files and free or delete memory
    close(swapfile_fd);
    close(program_fd);
     delete[] page_table;
     delete[] page;
     delete[] inData;
}

void sim_mem::print_memory() { 
    int i; 
    printf("\n Physical memory\n"); 
    for(i = 0; i < MEMORY_SIZE; i++) { 
        printf("[%c]\n", main_memory[i]); 
    } 
}

void sim_mem::print_swap() { 
    char* str = (char*) malloc(this->page_size *sizeof(char));
    int i; 
    printf("\n Swap memory\n"); 
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file 
    while(read(swapfile_fd, str, this->page_size) == this->page_size) { 
        for(i = 0; i < page_size; i++) { 
            printf("%d - [%c]\t", i, str[i]); 
        } 
        printf("\n"); 
    } 
}

void sim_mem::print_page_table() { 
    int i; 
    printf("\n page table \n"); 
    printf("Valid\t Dirty\t Permission \t Frame\n"); 
    for(i = 0; i < num_of_pages; i++) { 
        printf("[%d]\t[%d]\t[%d]\t[%d]\n", page_table[i].V, page_table[i].D, page_table[i].P, page_table[i].frame); 
    } 
}