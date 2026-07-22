/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GMSL_H_
#define GMSL_H_

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

#ifdef __cplusplus
}
#endif

#endif /* GMSL_H_ */
