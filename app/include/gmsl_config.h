/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GMSL_CONFIG_H_
#define GMSL_CONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct for building the initialization tables for GMSL
 */
struct gmsl_reg {
    /* Register Address */
    uint16_t reg_addr;

    /* Initialization Value */
    uint8_t data;
};

/*
 * The GMSL link bringup is done in the following order
 *  - deser_link_init
 *  - Delay: GMSL_LINK_INIT_DELAY_US
 *  - ser_xmit_init
 *  - ser_init
 *  - deser_init
 *  - deser_link_enable
 *  - ser_xmit_enable
 */

/** Delay time after running the deserialized link initialization */
#define GMSL_LINK_INIT_DLEAY_US 5

/** Register table for serializer transmit init phase */
static const struct gmsl_reg ser_max96717_xmit_init[] = {
    {0x0002, 0x03}, // DEV REG2: Disable Pipe Z
};

/** Register table for serializer device init phase */
static const struct gmsl_reg ser_max96717_init[] = {
    /* GPIO Configuration */
    {0x02C6, 0x00}, // MFP2: Receive from GMSL GPIO 0
    {0x02C4, 0x94}, // MFP2: Enable Rx over GMSL
    {0x02CD, 0x90}, // MFP5: Always high (IMX Module Enable)
    /* MIPI D-PHY Configuration */
    {0x0330, 0x00}, // MIPI_RX0: Port Configuration 1x4
    {0x0383, 0x80}, // MIPI_RX_EXT: Tunnel Mode Enabled
    {0x0331, 0x10}, // MIPI_RX1: Port B Lane Count: 2
    {0x0332, 0xE0}, // MIPI_RX2: PHY1 Lane Map D0=Lane2, D1=Lane3
    {0x0333, 0x04}, // MIPI_RX3: PHY2 Lane Map D0=Lane0, D1=Lane1
    {0x0334, 0x00}, // MIPI_RX4: PHY1 Polarity Normal
    {0x0335, 0x00}, // MIPI_RX5: PHY2 Polarity Normal
    /* Controller to Pipe Mapping */
    {0x0308, 0x64}, // FRONTTOP0: CLK_SELZ Port B, START_PORTB Enabled
    {0x0311, 0x30}, // FRONTTOP9: START_PORTBZ Start Video
    /* Pipe Configuration */
    {0x005B, 0x00}, // VIDEO_Z TX3: TX_STR_SEL Pipe Z = 0x0
};

/** Register table for serializer transmit enable phase */
static const struct gmsl_reg ser_max96717_xmit_enable[] = {
    {0x0002, 0x43}, // DEV REG2: Enable Pipe Z
};

/** Register table for deserializer link init phase */
static const struct gmsl_reg deser_max96724_link_init[] = {
    {0x040B, 0x00}, // BACKTOP12: CSI output disabled
    {0x0006, 0xF1}, // REG6: LINK_EN_A Enabled, B/C/D Disabled
    {0x0003, 0xFE}, // REG3: Remote CC Link A I2C Enabled, B/C/D Disabled
};

/** Register table for deserializer device init phase */
static const struct gmsl_reg deser_max96724_init[] = {
    /* GPIO Configuration */
    {0x0300, 0x83}, // GPIO0: Enable TX over GMSL
    {0x0310, 0x83}, // GPIO5: Enable TX over GMSL
    /* Video Pipe Routing */
    {0x00F0, 0x60}, // VIDEO_PIPE_SEL0: Pipe 0 PHY A, Input Pipe X
    {0x00F4, 0x01}, // VIDEO_PIPE_EN: Pipe 0 Enabled, Pipes 1-3 Disabled
    /* MIPI D-PHY Configuration */
    {0x08A0, 0x04}, // MIPI_PHY0: Port Configuration 2 (1x4)
    {0x098A, 0x50}, // MIPI_TX10: Port B Lane Count: 2
    {0x08A4, 0xE4}, // MIPI_PHY4: PHY2 D0=Lane0, D1=Lane1, PHY3 D0=Lane2, D1=Lane3
    {0x08A6, 0x00}, // MIPI_PHY6: PHY2/PHY3 Polarity Normal
    {0x0983, 0x07}, // MIPI_TX3: Controller 2 Auto Initial Deskew Disabled
    {0x0984, 0x01}, // MIPI_TX4: Controller 2 Periodic Deskew Disabled
    /* CSI Clock Configuration (~1000MHz per lane) */
    {0x1E00, 0xF4}, // PHY2 soft reset assert
    {0x1E00, 0xF4}, // PHY2 soft reset assert (repeated per errata)
    {0x041B, 0x2A}, // BACKTOP: CSI clock ~1000MHz per lane
    {0x1E00, 0xF5}, // PHY2 soft reset deassert
    {0x08A2, 0xC4}, // MIPI_PHY2: PHY0 and PHY1 in standby
    /* Tunnel Mode Configuration */
    {0x08CA, 0xE6}, // MIPI_CTRL_SEL: Pipe 0 Controller = 0x2
    {0x0939, 0x20}, // MIPI_TX57: Tunnel Destination Pipe 0 = 0x2
    {0x0936, 0x09}, // MIPI_TX54: Pipe 0 Tunnel Mode Enabled
    {0x0939, 0x60}, // MIPI_TX57: Pipe 0 Disable Auto Tunnel Detection
};

/** Register table for deserializer link enable phase */
static const struct gmsl_reg deser_max96724_link_enable[] = {
    {0x040B, 0x02}, // BACKTOP12: CSI output enabled
};

#ifdef __cplusplus
}
#endif

#endif /* GMSL_CONFIG_H_ */
