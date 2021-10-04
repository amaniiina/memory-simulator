#define MEMORY_SIZE 150
extern char main_memory[MEMORY_SIZE];

typedef struct page_descriptor {
    unsigned int V;     // valid
    unsigned int D;     // dirty
    unsigned int P;     // permission
    unsigned int frame; // the number of a frame if in case it is page-mapped
} page_descriptor;

class sim_mem {

    int swapfile_fd;    // swap file fd
    int program_fd;     // executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int num_of_pages;
    int page_size;
    page_descriptor *page_table; // pointer to page table

    char *page;         // array for 
    int maxFrames;      // max pages in main memory
    int *pageAtFrame;        // keep track of free frames where index= frame and value= page in that frame
    int bss_frames;     //max frames to fit in bss segment
    int heap_stack_frames;  //max frames to fit in heap/stack segment
    int data_frames;    //max frames to fit in data segment
    void swap(int);     // swap pages from main memory when full
    void addToPageTable(int); // update page table
    void getFromExe(int); // get page from executable file and swap if nessecary
    int *inData;        // physical addresses in data
    //void fullMemCheck(int);


public:

    sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size, 
            int bss_size, int heap_stack_size, int num_of_pages, int page_size);
    ~sim_mem();
    char load(int address);
    void store(int address, char value);
    void print_memory();
    void print_swap ();
    void print_page_table();

};

