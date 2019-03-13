#ifndef _BOOT_MODE_H_
#define _BOOT_MODE_H_

void normal_mode(void);
void recovery_mode(void);
void update_mode(void);
void charge_mode(void);
void dloader_mode(void);
void fastboot_mode(void);
void alarm_mode(void);
void calibration_detect(int key);
void engtest_mode(void);
void sleep_mode(void);
int is_bat_low(void);
int alarm_flag_check(void);

#define RECOVERY_MODE   0x77665502
#define FASTBOOT_MODE   0x77665500
#define NORMAL_MODE   0x77665503
#define ALARM_MODE   0x77665504
#define SLEEP_MODE   0x77665505

#define BOOT_NORAML 0xf1
#define BOOT_FASTBOOT 0xf2
#define BOOT_RECOVERY 0xf3
#define BOOT_CALIBRATE 0xf4
#define BOOT_DLOADER 0xf5
#define BOOT_CHARGE 0xf6
#define BOOT_UPDATE 0xf7

#define KEY_FASTBOOT 0x2f
#define KEY_RECOVERY 0x3f
#define KEY_ENGTEST 0x4f
#define KEY_UPDATE 0x5f

#define BACKLIGHT_ON 1
#define BACKLIGHT_OFF 0

extern unsigned int check_key_boot(uint32_t key);
extern void vlx_nand_boot(char * kernel_pname, char * cmdline, int backlight_set);
#endif
