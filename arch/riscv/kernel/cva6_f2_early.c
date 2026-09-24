// SPDX-License-Identifier: GPL-2.0-only
/*
 * AWS F2 / CVA6 early-boot (filled in setup_vm(), read from head.S).
 */
#include <linux/export.h>
#include <linux/types.h>

#ifdef CONFIG_CVA6_F2_NO_AMO
unsigned long f2_earlycon_uart_va;
u64 f2_satp_early_pgdir;
#endif
