/****************************************************************************
 * boards/arm/ra8p/ek-ra8p1/src/ek-ra8p1.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_RA8P_EK_RA8P1_SRC_EK_RA8P1_H
#define __BOARDS_ARM_RA8P_EK_RA8P1_SRC_EK_RA8P1_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void ra8p1_pin_initialize(void);
int ra8p1_ospi0_cs1_nor_initialize(void);
int ra8p1_sdram_initialize(void);
int ra8p1_extmem_initialize(void);

#endif /* __BOARDS_ARM_RA8P_EK_RA8P1_SRC_EK_RA8P1_H */
