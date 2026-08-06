/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GMSL_H_
#define GMSL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the GMSL Devices
 *
 * Initializes the GMSL interface. This uses the configuration parameters found
 * in gmsl_config.h and the device tree nodes to setup the devices
 *
 * @retval 0 on success. Error code otherwise.
 */
int gmsl_init(void);

/**
 * @brief Sets the i2c mux control
 *
 * The GMSL hardware platforms feature on-board I2C muxes controlled by physical
 * switches or test point signals. If declared in the device tree, this will
 * set the state of the associated GPIO.
 *
 * @param state - State to set the GPIO for i2c control ownership of this
 *  application
 *
 * @retval 0 on success. Error code otherwise.
 */
int gmsl_set_i2c_ctrl(bool state);

#ifdef __cplusplus
}
#endif

#endif /* GMSL_H_ */
