#ifndef _PDL_ENGINE_H_
#define _PDL_ENGINE_H_

typedef int (*pdl_cmd_handler)(struct pdl_packet *pkt, void *arg); 
struct pdl_cmd { 
        pdl_cmd_handler handler; 
}; 
 
int pdl_cmd_register(int cmd_type, pdl_cmd_handler handler); 
int pdl_handler(void *arg);
int pdl_handle_connect(u32 timeout);
#endif
