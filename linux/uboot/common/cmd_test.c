/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>

int do_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char * const *ap;
	int left, adv, expr, last_expr, neg, last_cmp;

	/* args? */
	if (argc < 3)
		return 1;

#if 0
	{
		printf("test:");
		left = 1;
		while (argv[left])
			printf(" %s", argv[left++]);
	}
#endif

	last_expr = 0;
	left = argc - 1; ap = argv + 1;
	if (left > 0 && strcmp(ap[0], "!") == 0) {
		neg = 1;
		ap++;
		left--;
	} else
		neg = 0;

	expr = -1;
	last_cmp = -1;
	last_expr = -1;
	while (left > 0) {

		if (strcmp(ap[0], "-o") == 0 || strcmp(ap[0], "-a") == 0)
			adv = 1;
		else if (strcmp(ap[0], "-z") == 0 || strcmp(ap[0], "-n") == 0)
			adv = 2;
		else
			adv = 3;

		if (left < adv) {
			expr = 1;
			break;
		}

		if (adv == 1) {
			if (strcmp(ap[0], "-o") == 0) {
				last_expr = expr;
				last_cmp = 0;
			} else if (strcmp(ap[0], "-a") == 0) {
				last_expr = expr;
				last_cmp = 1;
			} else {
				expr = 1;
				break;
			}
		}

		if (adv == 2) {
			if (strcmp(ap[0], "-z") == 0)
				expr = strlen(ap[1]) == 0 ? 1 : 0;
			else if (strcmp(ap[0], "-n") == 0)
				expr = strlen(ap[1]) == 0 ? 0 : 1;
			else {
				expr = 1;
				break;
			}

			if (last_cmp == 0)
				expr = last_expr || expr;
			else if (last_cmp == 1)
				expr = last_expr && expr;
			last_cmp = -1;
		}

		if (adv == 3) {
			if (strcmp(ap[1], "=") == 0)
				expr = strcmp(ap[0], ap[2]) == 0;
			else if (strcmp(ap[1], "!=") == 0)
				expr = strcmp(ap[0], ap[2]) != 0;
			else if (strcmp(ap[1], ">") == 0)
				expr = strcmp(ap[0], ap[2]) > 0;
			else if (strcmp(ap[1], "<") == 0)
				expr = strcmp(ap[0], ap[2]) < 0;
			else if (strcmp(ap[1], "-eq") == 0)
				expr = simple_strtol(ap[0], NULL, 10) == simple_strtol(ap[2], NULL, 10);
			else if (strcmp(ap[1], "-ne") == 0)
				expr = simple_strtol(ap[0], NULL, 10) != simple_strtol(ap[2], NULL, 10);
			else if (strcmp(ap[1], "-lt") == 0)
				expr = simple_strtol(ap[0], NULL, 10) < simple_strtol(ap[2], NULL, 10);
			else if (strcmp(ap[1], "-le") == 0)
				expr = simple_strtol(ap[0], NULL, 10) <= simple_strtol(ap[2], NULL, 10);
			else if (strcmp(ap[1], "-gt") == 0)
				expr = simple_strtol(ap[0], NULL, 10) > simple_strtol(ap[2], NULL, 10);
			else if (strcmp(ap[1], "-ge") == 0)
				expr = simple_strtol(ap[0], NULL, 10) >= simple_strtol(ap[2], NULL, 10);
			else {
				expr = 1;
				break;
			}

			if (last_cmp == 0)
				expr = last_expr || expr;
			else if (last_cmp == 1)
				expr = last_expr && expr;
			last_cmp = -1;
		}

		ap += adv; left -= adv;
	}

	if (neg)
		expr = !expr;

	expr = !expr;

	debug (": returns %d\n", expr);

	return expr;
}

U_BOOT_CMD(
	test,	CONFIG_SYS_MAXARGS,	1,	do_test,
	"minimal test like /bin/sh",
	"[args..]"
);

int do_false(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 1;
}

U_BOOT_CMD(
	false,	CONFIG_SYS_MAXARGS,	1,	do_false,
	"do nothing, unsuccessfully",
	NULL
);

int do_true(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}

U_BOOT_CMD(
	true,	CONFIG_SYS_MAXARGS,	1,	do_true,
	"do nothing, successfully",
	NULL
);

#ifdef CONFIG_VPU_TEST
#include <asm/arch/hw_test.h>
int do_vpu_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	if(argc > 1) {
		times = simple_strtoul(argv[1], NULL, 10);
	}
	vpu_sta_test(times);
	return 0;
}
U_BOOT_CMD(
	vpu_test,	CONFIG_SYS_MAXARGS,	1,	do_vpu_test,
	"vpu test",
	NULL
);

#ifdef CONFIG_VPU_MD5_TEST
int do_vpu_md5_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	if(argc > 1) {
		times = simple_strtoul(argv[1], NULL, 10);
	}
	vpu_md5_test(times);
	return 0;
}
U_BOOT_CMD(
	vpu_md5_test,	CONFIG_SYS_MAXARGS,	1,	do_vpu_md5_test,
	"vpu md5 test",
	NULL
);
#endif //CONFIG_VPU_MD5_TEST
#endif //CONFIG_VPU_TEST

#ifdef CONFIG_CPU_TEST
#include <asm/arch/hw_test.h>
int do_cpu_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	if(argc > 1) {
		times = simple_strtoul(argv[1], NULL, 10);
	}
	cpu_pll_test(times);
	return 0;
}
U_BOOT_CMD(
	cpu_test,	CONFIG_SYS_MAXARGS,	1,	do_cpu_test,
	"cpu test",
	NULL
);
#endif //CONFIG_VPU_TEST

#ifdef CONFIG_DDR_TEST
#include <asm/arch/hw_test.h>
int do_ddr_mem_copy_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	unsigned int src_addr = 0x80000000,des_addr = 0x83000000,n = 0x10;

	if(argc > 1) {
		times = simple_strtoul(argv[1], NULL, 10);
	}
	if(argc > 2) {
		src_addr = simple_strtoul(argv[2],NULL,16);
	}
	if(argc > 3) {
		des_addr = simple_strtoul(argv[3],NULL,16);
	}
	if(argc > 4) {
		n = simple_strtoul(argv[4],NULL,16);
	}
	ddr_mem_copy_test(times,src_addr,des_addr,n);
	return 0;
}

U_BOOT_CMD(
	ddr_mem_cp,	5,	1,	do_ddr_mem_copy_test,
	"ddr memory copy test",
	NULL
);
#endif //CONFIG_DDR_TEST

#ifdef CONFIG_TIMER_TEST
#include <asm/arch/hw_test.h>
int do_timer_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	unsigned int timer_id = 0;

	if(argc > 1) {
		timer_id = simple_strtoul(argv[1], NULL, 10);
	}
	if(argc > 2) {
		times = simple_strtoul(argv[2],NULL,16);
	}
	tim_test(timer_id,times);
	return 0;
}

U_BOOT_CMD(
	timer_test,	5,	1,	do_timer_test,
	"timer_test timer_id(0~2) times",
	NULL
);
#endif /* CONFIG_TIMER_TEST */

#ifdef CONFIG_UART_TEST
#include <asm/arch/hw_test.h>
int do_uart_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	unsigned int uart_id = 2;// uart3 as default test port

	if(argc > 1) {
		uart_id = simple_strtoul(argv[1], NULL, 10);
	}
	if(argc > 2) {
		times = simple_strtoul(argv[2],NULL,16);
	}
	uart_test(uart_id,times);
	return 0;
}

U_BOOT_CMD(
	uart_test,	5,	1,	do_uart_test,
	"uart_test uart_id(0~2) times",
	NULL
);
#endif /* CONFIG_UART_TEST */

#ifdef CONFIG_GIC_TEST
#include <asm/arch/hw_test.h>
int do_gic_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;

	if(argc > 1) {
		times = simple_strtoul(argv[1], NULL, 10);
	}
	gic_test(times);
	return 0;
}

U_BOOT_CMD(
	gic_test,	5,	1,	do_gic_test,
	"gic_test times",
	NULL
);
#endif /* CONFIG_GIC_TEST */

#ifdef CONFIG_CACHE_TEST
#include <asm/arch/hw_test.h>
int do_cache_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;

	if(argc > 1) {
		times = simple_strtoul(argv[1], NULL, 10);
	}
	cpu_cache_test(times);
	return 0;
}

U_BOOT_CMD(
	cache_test,	5,	1,	do_cache_test,
	"cpu L1/L2 D-cache I-cache test",
	NULL
);
#endif /* CONFIG_CACHE_TEST */

#ifdef CONFIG_MIPI_LOOP_TEST
#include<asm/arch/hw_test.h>
int do_mipi_loop_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;

	if (argc > 1)
		times = simple_strtoull(argv[1], NULL, 10);
	if (times < 1) {
		printf("Invalid parameter\n");
		return 1;
	}
	test_dsi_csi_loop(times);

	return 0;
}

U_BOOT_CMD(
	mipi_loop,	5,	5,	do_mipi_loop_test,
	"mipi loop test",
	NULL
);
#endif /* CONFIG_MIPI_LOOP_TEST */

#ifdef CONFIG_I2C_TEST
#include<asm/arch/hw_test.h>
int do_i2c_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int times = 1;
	int id = 2; /* I2C id number:0,1,2,I2C 2 is default device */

	if(argc > 1)
		times = simple_strtoull(argv[1], NULL, 10);
	if(argc > 2)
		id = simple_strtoul(argv[2],NULL,10);
	test_i2c(id,times);
	return 0;
}

U_BOOT_CMD(
	i2c_test,	5,	5,	do_i2c_test,
	"i2c test",
	NULL
);
#endif /* CONFIG_I2C_TET */
