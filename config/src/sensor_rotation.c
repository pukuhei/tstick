#define DT_DRV_COMPAT zmk_input_processor_sensor_rotation

#include "drivers/input_processor.h"
#include <drivers/sensor_rotation.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct sensor_rotation_config {
  int rotation_angle;
};

struct sensor_rotation_data {
  int32_t x;
  int32_t y;
  int16_t sin_val;
  int16_t cos_val;
  int angle;
};

static const int16_t sin_table[] = {
    0,   87,  174, 259, 342, 423, 500, 574, 643,  707,
    766, 819, 866, 906, 940, 966, 985, 996, 1000,
};

static const int16_t cos_table[] = {
    1000, 996, 985, 966, 940, 906, 866, 819, 766, 707,
    643,  574, 500, 423, 342, 259, 174, 87,  0,
};

static void lookup_sin_cos(int angle, int16_t *sin_val, int16_t *cos_val) {
  angle = angle % 360;
  if (angle < 0) {
    angle += 360;
  }

  int index = (angle % 90) / 5;
  int quadrant = angle / 90;

  int16_t sin_base = sin_table[index];
  int16_t cos_base = cos_table[index];

  switch (quadrant) {
  case 0:
    *sin_val = sin_base;
    *cos_val = cos_base;
    break;
  case 1:
    *sin_val = cos_base;
    *cos_val = -sin_base;
    break;
  case 2:
    *sin_val = -sin_base;
    *cos_val = -cos_base;
    break;
  case 3:
    *sin_val = -cos_base;
    *cos_val = sin_base;
    break;
  }
}

static void rotate_xy(int32_t *x, int32_t *y, int16_t sin_val,
                      int16_t cos_val) {
  int32_t new_x = (*x * cos_val - *y * sin_val) / 1000;
  int32_t new_y = (*x * sin_val + *y * cos_val) / 1000;

  *x = new_x;
  *y = new_y;
}

static int normalize_angle(int angle) {
  if (angle == 360) {
    return 0;
  }

  angle %= 360;
  if (angle < 0) {
    angle += 360;
  }

  return angle;
}

int sensor_rotation_get_angle(const struct device *dev) {
  struct sensor_rotation_data *data = dev->data;
  return data->angle;
}

int sensor_rotation_set_angle(const struct device *dev, int angle) {
  struct sensor_rotation_data *data = dev->data;

  angle = normalize_angle(angle);
  data->angle = angle;
  lookup_sin_cos(angle, &data->sin_val, &data->cos_val);

  LOG_DBG("Sensor rotation angle updated to %d", angle);
  return 0;
}

int sensor_rotation_step_angle(const struct device *dev, int step_deg,
                               int min_deg, int max_deg, bool wrap) {
  struct sensor_rotation_data *data = dev->data;

  min_deg = normalize_angle(min_deg);
  max_deg = normalize_angle(max_deg);

  int angle = data->angle + step_deg;

  if (step_deg > 0) {
    if (angle > max_deg) {
      angle = wrap ? min_deg : max_deg;
    }
  } else if (step_deg < 0) {
    if (angle < min_deg) {
      angle = wrap ? max_deg : min_deg;
    }
  }

  return sensor_rotation_set_angle(dev, angle);
}

static int sensor_rotation_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {
  struct sensor_rotation_data *data = dev->data;

  switch (event->type) {
  case INPUT_EV_REL:
    if (event->code == INPUT_REL_X) {
      int32_t temp_x = event->value;
      int32_t temp_y = data->y;
      rotate_xy(&temp_x, &temp_y, data->sin_val, data->cos_val);
      data->x = event->value;
      event->value = temp_x;
      LOG_DBG("X value: %d, rotate %d : %d, %d : sin %d, cos %d", event->value,
              data->angle, data->x, data->y, data->sin_val, data->cos_val);
    } else if (event->code == INPUT_REL_Y) {
      int32_t temp_x = data->x;
      int32_t temp_y = event->value;
      rotate_xy(&temp_x, &temp_y, data->sin_val, data->cos_val);
      data->y = event->value;
      event->value = temp_y;
      LOG_DBG("Y value: %d, rotate %d : %d, %d : sin %d, cos %d", event->value,
              data->angle, data->x, data->y, data->sin_val, data->cos_val);
    }
    break;
  default:
    return ZMK_INPUT_PROC_CONTINUE;
  }

  return 0;
}

static int sensor_rotation_init(const struct device *dev) {
  struct sensor_rotation_data *data = dev->data;
  const struct sensor_rotation_config *cfg = dev->config;

  data->x = 0;
  data->y = 0;

  return sensor_rotation_set_angle(dev, cfg->rotation_angle);
}

static struct zmk_input_processor_driver_api sensor_rotation_driver_api = {
    .handle_event = sensor_rotation_handle_event,
};

#define SENSOR_ROTATION_INST(n)                                                \
  static struct sensor_rotation_data sensor_rotation_data_##n = {};            \
  static const struct sensor_rotation_config sensor_rotation_config_##n = {    \
      .rotation_angle = DT_INST_PROP(n, rotation_angle),                       \
  };                                                                           \
  DEVICE_DT_INST_DEFINE(                                                       \
      n, sensor_rotation_init, NULL, &sensor_rotation_data_##n,                \
      &sensor_rotation_config_##n, POST_KERNEL,                                \
      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &sensor_rotation_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_ROTATION_INST)
