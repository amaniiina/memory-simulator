#include <string.h>
#include <stdio.h>
#include "sim_mem.cpp"
int main(){
    char val;
    sim_mem mem_sm("exec_file", "swap_file" ,25, 50, 25,25, 25, 5);
    mem_sm.store( 80,'X');
    mem_sm.store( 70,'E');
    mem_sm.store( 50,'T');
    mem_sm.store( 60,'V');
    mem_sm.store( 50,'w');
    mem_sm.store( 0,'Q');
    val = mem_sm.load ( 0);
    val = mem_sm.load ( 5);
    printf("VAL IS : %c\n", val);
    val = mem_sm.load ( 15);
    printf("VAL IS : %c\n", val);
    val = mem_sm.load ( 20);
    val = mem_sm.load ( 45);
    val = mem_sm.load ( 85);
    val = mem_sm.load ( 75);
    val = mem_sm.load ( 66);
    mem_sm.print_memory();
    mem_sm.print_swap();
    mem_sm.print_page_table();

    return 0;
}
