#ifndef _PDL1_COMMAND_H_
#define _PDL1_COMMAND_H_

int sys_connect(struct pdl_packet *packet, void *arg);
int data_start(struct pdl_packet *packet, void *arg);
int data_midst(struct pdl_packet *packet, void *arg);
int data_end(struct pdl_packet *packet, void *arg);
int data_exec(struct pdl_packet *packet, void *arg);
int get_pdl_version(struct pdl_packet *packet, void *arg);
int reset_machine(struct pdl_packet *packet, void *arg);
int get_pdl_security(struct pdl_packet *packet, void *arg);

#endif
