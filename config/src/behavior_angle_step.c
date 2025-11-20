#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <drivers/sensor_rotation.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct sensor_rotation_step_config {
  const struct device *sensor_dev;
  int step_deg;
  int min_deg;
  int max_deg;
  bool wrap;
};

static int sensor_rotation_step_pressed(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event,
                                        uint32_t param1, uint32_t param2) {
  const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
  if (!dev) {
    return -ENODEV;
  }

  const struct sensor_rotation_step_config *cfg = dev->config;
  return sensor_rotation_step_angle(cfg->sensor_dev, cfg->step_deg,
                                    cfg->min_deg, cfg->max_deg, cfg->wrap);
}

static int
sensor_rotation_step_released(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event,
                              uint32_t param1, uint32_t param2) {
  return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api sensor_rotation_step_behavior_api = {
    .binding_pressed = sensor_rotation_step_pressed,
    .binding_released = sensor_rotation_step_released,
};

#define SENSOR_ROTATION_BEHAVIOR(inst)                                         \
  static const struct sensor_rotation_step_config                              \
      sensor_rotation_step_config_##inst = {                                   \
          .sensor_dev = DEVICE_DT_GET(DT_INST_PROP(inst, sensor_rotation)),    \
          .step_deg = DT_INST_PROP(inst, step_deg),                            \
          .min_deg = DT_INST_PROP_OR(inst, min_angle, 0),                      \
          .max_deg = DT_INST_PROP_OR(inst, max_angle, 315),                    \
          .wrap = DT_INST_PROP_OR(inst, wrap_around, false),                   \
  };                                                                           \
  BEHAVIOR_DT_INST_DEFINE(inst, sensor_rotation_step_behavior_api, NULL,       \
                          &sensor_rotation_step_config_##inst, NULL)

DT_INST_FOREACH_STATUS_OKAY(SENSOR_ROTATION_BEHAVIOR);
