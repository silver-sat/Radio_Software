/**
* @file stats.h
* @author Tom Conrad (tom@silversat.org)
* @brief Library providing structure to handle code execution stats
* @version 1.0.1
* @date 2024-10-31

stats.h - Library providing structure to handle code execution stats
Created by Tom Conrad, October 31, 2024.
Released into the public domain.

*/

#ifndef STATS_H
#define STATS_H

struct Stats
{
    // process timers
    unsigned long max_loop_time{0};
    unsigned long max_interface_handler_execution_time{0};
    unsigned long max_transmit_handler_execution_time{0};
    unsigned long max_data_processor_execution_time{0};
    unsigned long max_receive_handler_execution_time{0};

    // debug variable
    int max_buffer_load_s0{0};
    int max_buffer_load_s1{0};
    int max_databuffer_load{0};
    int max_commandbuffer_load{0};
    int max_txbuffer_load{0};

    // memory tracking
    int free_mem_minimum{32000};
};

#endif