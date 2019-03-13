#ifndef _PDL2_COMMAND_H_
#define _PDL2_COMMAND_H_

int pdl_command_init(void);
int sys_connect(struct pdl_packet *packet, void *arg);
int data_start(struct pdl_packet *packet, void *arg);
int data_midst(struct pdl_packet *packet, void *arg);
int data_end(struct pdl_packet *packet, void *arg);
int data_exec(struct pdl_packet *packet, void *arg);
int read_partition(struct pdl_packet *packet, void *arg);
int read_partition_table(struct pdl_packet *packet, void *arg);
int format_flash(struct pdl_packet *packet, void *arg);
int read_image_attr(struct pdl_packet *packet, void *arg);
int erase_partition(struct pdl_packet *packet, void *arg);
int get_pdl_version(struct pdl_packet *packet, void *arg);
int reset_machine(struct pdl_packet *packet, void *arg);
int poweroff_machine(struct pdl_packet *packet, void *arg);
int set_pdl_dbg(struct pdl_packet *packet, void *arg);
int check_partition_table(struct pdl_packet *packet, void *arg);
int recv_image_list(struct pdl_packet *packet, void *arg);
int get_pdl_security(struct pdl_packet *packet, void *arg);
int hw_test(struct pdl_packet *packet, void *arg);
int get_pdl_log(struct pdl_packet *packet, void *arg);
int download_finish(struct pdl_packet *packet, void *arg);

#endif
