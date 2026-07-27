/*
 * Copyright (c) 2026 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/video.h>
#include <zephyr/video/controls.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cam_ctrl, CONFIG_LOG_DEFAULT_LEVEL);

#define CAM_ADJ_NUM_STEPS	5

struct camera_adj_event {
	struct k_work work;
	uint32_t ctrl_id;
};

static void camera_adj_handler(struct k_work *work);
static void button_input_cb(struct input_event *evt, void *user_data);

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);
static struct camera_adj_event cam_adj_evt = { .work = Z_WORK_INITIALIZER(camera_adj_handler) };
static const struct device* camera_dev = DEVICE_DT_GET(DT_CHOSEN(camera_sensor));

static void camera_adj_handler(struct k_work *work)
{
	struct camera_adj_event* cam_evt = CONTAINER_OF(work, struct camera_adj_event, work);
	struct video_ctrl_query query;
	struct video_control ctrl;
	int32_t span, step;

	query.dev = camera_dev;
	query.id = cam_evt->ctrl_id;
	if (video_query_ctrl(&query)) {
		LOG_ERR("Failed to query control");
		return;
	}

	span = query.range.max - query.range.min;
	step = MAX(1, span / CAM_ADJ_NUM_STEPS);

	ctrl.id = cam_evt->ctrl_id;
	if (video_get_ctrl(camera_dev, &ctrl)) {
		LOG_ERR("Failed to get control");
		return;
	}

	ctrl.val = ctrl.val + step;
	if (ctrl.val > query.range.max) {
		ctrl.val = query.range.min + (ctrl.val - query.range.max);
	}
	ctrl.val = ROUND_UP(ctrl.val, query.range.step);

	if (video_set_ctrl(camera_dev, &ctrl)) {
		LOG_ERR("Failed to set control");
		return;
	}
}

static void button_input_cb(struct input_event *evt, void *user_data)
{
	if (evt->sync == 0) {
		return;
	}

	if (evt->value) {
		switch (evt->code) {
			case INPUT_KEY_G:
				cam_adj_evt.ctrl_id = VIDEO_CID_ANALOGUE_GAIN;
				k_work_submit(&cam_adj_evt.work);
				break;
			case INPUT_KEY_E:
				cam_adj_evt.ctrl_id = VIDEO_CID_EXPOSURE;
				k_work_submit(&cam_adj_evt.work);
				break;
		}
	}
}
