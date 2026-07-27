/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <math.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/stm32_dcmipp.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(isp_ctrl, CONFIG_LOG_DEFAULT_LEVEL);

/* Only enable via KConfig */
#if CONFIG_ISP_AWB

/* Get the Pipe from the device tree */
#define AWB_PIPE            DT_REG_ADDR(DT_NODELABEL(pipe_main))

static DCMIPP_HandleTypeDef *isp_hdcmipp;

/* Frame counter to support intervals for updating the AWB */
static uint32_t frame_count;


/**
 * Applies the given gains to the ISP's Exposure block, which happens prior to
 * demosaicing.
 *
 * @param r_gain - Red scale factor (1.0 = Unity)
 * @param g_gain - Green scale factor (1.0 = Unity)
 * @param b_gain - Blue scale factor (1.0 = Unity)
 */
static void isp_awb_apply_gains(float r_gain, float g_gain, float b_gain)
{
	DCMIPP_ExposureConfTypeDef exp_cfg;

	/* The Exposure control is really a digital gain prior to running the demosaicing.
	 * It consists of a shift factor and multiplier.  To support a variety of input
	 * images, dynamically calculate the shift factor to get the most accurate
	 * representation of the gain.
	 * Per the API/Documentation:
	 * 	Shift Range = 0 - 7
	 *  Multiplier = 0 - 127
	 *
	 * Gain = Multiplier * 2**Shift / 128
	 * */
	exp_cfg.ShiftRed = CLAMP((uint8_t)(ceilf(log2f((r_gain * 128.0f)/127.0f))), 0, 7);
	exp_cfg.MultiplierRed = (uint8_t)(roundf((r_gain * 128.0f) / (float)(1 << exp_cfg.ShiftRed)));
	exp_cfg.ShiftGreen = CLAMP((uint8_t)(ceilf(log2f((g_gain * 128.0f)/127.0f))), 0, 7);
	exp_cfg.MultiplierGreen = (uint8_t)(roundf((g_gain * 128.0f) / (float)(1 << exp_cfg.ShiftGreen)));
	exp_cfg.ShiftBlue = CLAMP((uint8_t)(ceilf(log2f((b_gain * 128.0f)/127.0f))), 0, 7);
	exp_cfg.MultiplierBlue = (uint8_t)(roundf((b_gain * 128.0f) / (float)(1 << exp_cfg.ShiftBlue)));
	HAL_DCMIPP_PIPE_SetISPExposureConfig(isp_hdcmipp, AWB_PIPE, &exp_cfg);

	LOG_DBG("Exp: R=%u/%u G=%u/%u B=%u/%u",
		exp_cfg.ShiftRed, exp_cfg.MultiplierRed, exp_cfg.ShiftGreen, exp_cfg.MultiplierGreen,
		exp_cfg.ShiftBlue, exp_cfg.MultiplierBlue );
}

static void isp_awb_update(void)
{
	uint32_t acc_r, acc_g, acc_b;
	float gain_r, gain_b;

	/* Read accumulated statistics for each channel.
	 * We configured 3 stat extraction modules: 1=R, 2=G, 3=B
	 */
	if (HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter( isp_hdcmipp, AWB_PIPE,
			DCMIPP_STATEXT_MODULE1, &acc_r) != HAL_OK) {
		return;
	}

	if (HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter( isp_hdcmipp, AWB_PIPE,
			DCMIPP_STATEXT_MODULE2, &acc_g) != HAL_OK) {
		return;
	}

	if (HAL_DCMIPP_PIPE_GetISPAccumulatedStatisticsCounter( isp_hdcmipp, AWB_PIPE,
			DCMIPP_STATEXT_MODULE3, &acc_b) != HAL_OK) {
		return;
	}

	/* Can't do math if any values are 0 */
	if (acc_r == 0 || acc_g == 0 || acc_b == 0) {
		return;
	}

	/*
	 * Gray-world algorithm: adjust R and B gains so that their averages
	 * match the green channel average. Green channel gain stays at unity.
	 *
	 * Green is divided by 2 since Pre-Bayer there are 2x Green pixels than R and
	 * G.
	 */
	gain_r = (float)(acc_g >> 1) / (float)acc_r;
	gain_b = (float)(acc_g >> 1)/ (float)acc_b;

	isp_awb_apply_gains(gain_r, 1.0f, gain_b);

	LOG_DBG("AWB: R=%u G=%u B=%u -> mult_r=%u mult_b=%u",
		acc_r, acc_g, acc_b, (unsigned int)(gain_r * 100), (unsigned int)(gain_b * 100));
}

/* Weak function provided by the dcmipp driver */
int stm32_dcmipp_isp_init(DCMIPP_HandleTypeDef *hdcmipp, const struct device *source)
{
	DCMIPP_StatisticExtractionConfTypeDef stat_cfg;
	HAL_StatusTypeDef status;

	isp_hdcmipp = hdcmipp;
	frame_count = 0;

	/* Configure statistics extraction: Module 1 = Red average */
	stat_cfg.Mode = DCMIPP_STAT_EXT_MODE_AVERAGE;
	stat_cfg.Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_R;
	stat_cfg.Bins = DCMIPP_STAT_EXT_AVER_MODE_ALL_PIXELS;

	status = HAL_DCMIPP_PIPE_SetISPStatisticExtractionConfig(
				hdcmipp, AWB_PIPE, DCMIPP_STATEXT_MODULE1, &stat_cfg);
	if (status != HAL_OK) {
		LOG_ERR("Failed to configure stat extraction module 1 (Red)");
		return -EIO;
	}

	/* Module 2 = Green average */
	stat_cfg.Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_G;

	status = HAL_DCMIPP_PIPE_SetISPStatisticExtractionConfig(
				hdcmipp, AWB_PIPE, DCMIPP_STATEXT_MODULE2, &stat_cfg);
	if (status != HAL_OK) {
		LOG_ERR("Failed to configure stat extraction module 2 (Green)");
		return -EIO;
	}

	/* Module 3 = Blue average */
	stat_cfg.Source = DCMIPP_STAT_EXT_SOURCE_PRE_BLKLVL_B;

	status = HAL_DCMIPP_PIPE_SetISPStatisticExtractionConfig(
				hdcmipp, AWB_PIPE, DCMIPP_STATEXT_MODULE3, &stat_cfg);
	if (status != HAL_OK) {
		LOG_ERR("Failed to configure stat extraction module 3 (Blue)");
		return -EIO;
	}

	/* Enable all three stat extraction modules */
	HAL_DCMIPP_PIPE_EnableISPStatisticExtraction(hdcmipp, AWB_PIPE, DCMIPP_STATEXT_MODULE1);
	HAL_DCMIPP_PIPE_EnableISPStatisticExtraction(hdcmipp, AWB_PIPE, DCMIPP_STATEXT_MODULE2);
	HAL_DCMIPP_PIPE_EnableISPStatisticExtraction(hdcmipp, AWB_PIPE, DCMIPP_STATEXT_MODULE3);

	/* Enable the ISP exposure block with unity gains */
	isp_awb_apply_gains(1.0f, 1.0f, 1.0f);
	status = HAL_DCMIPP_PIPE_EnableISPExposure(hdcmipp, AWB_PIPE);
	if (status != HAL_OK) {
		LOG_ERR("Failed to enable ISP exposure");
		return -EIO;
	}

	LOG_INF("ISP AWB initialized");
	return 0;
}

/* Weak function provided by the dcmipp driver */
void stm32_dcmipp_isp_vsync_update(DCMIPP_HandleTypeDef *hdcmipp, uint32_t Pipe)
{
	if (Pipe != AWB_PIPE) {
		return;
	}

	frame_count++;
	if (frame_count >= CONFIG_ISP_AWB_INTERVAL) {
		frame_count = 0;
		isp_awb_update();
	}
}

int stm32_dcmipp_isp_start(void)
{
	frame_count = 0;
	return 0;
}
#endif
