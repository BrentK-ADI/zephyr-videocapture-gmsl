/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "gmsl.h"
#include "gmsl_config.h"

LOG_MODULE_REGISTER(gmsl, CONFIG_LOG_DEFAULT_LEVEL);

/* Verify the device tree nodes are as expected for this setup */
BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(gmsl_ser)),
	     "GMSL Serializer not configured correctly in devicetree");

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_NODELABEL(gmsl_ser), adi_max96717),
	     "gmsl_ser must be compatible with adi,max96717");

BUILD_ASSERT(DT_NODE_EXISTS(DT_NODELABEL(gmsl_deser)),
	     "GMSL Deserializer not configured correctly in devicetree");

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_NODELABEL(gmsl_deser), adi_max96724),
	     "gmsl_deser must be compatible with adi,max96724");

/* I2C Bus assignments for GMSL pulled from the device tree */
static const struct i2c_dt_spec ser_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(gmsl_ser));
static const struct i2c_dt_spec deser_i2c = I2C_DT_SPEC_GET(DT_NODELABEL(gmsl_deser));

/**
 * @brief Helper for reading a GMSL register value
 *
 * Performs a register read on specific I2C spec of the given register address.
 *
 * @param spec - I2C spec to read from
 * @param reg - 16-bit Register read
 * @param val - Storage location for received data
 *
 * @retval 0 on success, error code otherwise
 */
static int gmsl_reg_read(const struct i2c_dt_spec *spec, uint16_t reg, uint8_t *val)
{
	uint8_t addr_buf[2] = { reg >> 8, reg & 0xFF };

	return i2c_write_read_dt(spec, addr_buf, sizeof(addr_buf), val, 1);
}

/**
 * @brief Helper for writing a GMSL register value
 *
 * Performs a register write on specific I2C spec of the given register address.
 *
 * @param spec - I2C spec to write to
 * @param reg - 16-bit Register to write
 * @param val - Value to write
 *
 * @retval 0 on success, error code otherwise
 */
static int gmsl_reg_write(const struct i2c_dt_spec *spec, uint16_t reg, uint8_t val)
{
	uint8_t buf[3] = { reg >> 8, reg & 0xFF, val };

	return i2c_write_dt(spec, buf, sizeof(buf));
}

/**
 * @brief Helper for writing a block of initialization registers
 *
 * Performs register writes of the initialization registers provided by the
 * given table.
 *
 * @param spec - I2C spec to read from
 * @param reg_tabl - Table for register/data pairs to write
 * @param reg_count - Size of the register table
 *
 * @retval 0 on success, error code otherwise
 */
static int gmsl_load_reg_table(const struct i2c_dt_spec *spec,
							   const struct gmsl_reg* reg_table,
							   size_t reg_count)
{
	int i;
	int ret;
	for (i = 0; i < reg_count; i++) {
		ret = gmsl_reg_write(spec, reg_table[i].reg_addr, reg_table[i].data);
		if (ret) {
			return ret;
		}
	}

	return 0;
}


int gmsl_init(void)
{
	int ret;

	if (!i2c_is_ready_dt(&ser_i2c)) {
		LOG_ERR("Serializer I2C bus not ready");
		return -ENODEV;
	}

	if (!i2c_is_ready_dt(&deser_i2c)) {
		LOG_ERR("Deserializer I2C bus not ready");
		return -ENODEV;
	}

	ret = gmsl_load_reg_table(&deser_i2c, deser_max96724_link_init, ARRAY_SIZE(deser_max96724_link_init));
	if (ret) {
		return ret;
	}
	LOG_INF("Deserializer Link Init Complete");

	/* Link init delay */
	k_usleep(GMSL_LINK_INIT_DLEAY_US);

	ret = gmsl_load_reg_table(&ser_i2c, ser_max96717_xmit_init, ARRAY_SIZE(ser_max96717_xmit_init));
	if (ret) {
		return ret;
	}
	LOG_INF("Serializer Transmit Init Complete");

	ret = gmsl_load_reg_table(&ser_i2c, ser_max96717_init, ARRAY_SIZE(ser_max96717_init));
	if (ret) {
		return ret;
	}
	LOG_INF("Serializer Init Complete");

	ret = gmsl_load_reg_table(&deser_i2c, deser_max96724_init, ARRAY_SIZE(deser_max96724_init));
	if (ret) {
		return ret;
	}
	LOG_INF("Deserializer Init Complete");

	ret = gmsl_load_reg_table(&deser_i2c, deser_max96724_link_enable, ARRAY_SIZE(deser_max96724_link_enable));
	if (ret) {
		return ret;
	}
	LOG_INF("Deserializer Link Enable Complete");

	ret = gmsl_load_reg_table(&ser_i2c, ser_max96717_xmit_enable, ARRAY_SIZE(ser_max96717_xmit_enable));
	if (ret) {
		return ret;
	}
	LOG_INF("Serializer Transmit Enable Complete");

	LOG_INF("GMSL init complete");
	return 0;
}
