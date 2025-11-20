#pragma once

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

int sensor_rotation_set_angle(const struct device *dev, int angle);
int sensor_rotation_get_angle(const struct device *dev);
int sensor_rotation_step_angle(const struct device *dev, int step_deg,
                               int min_deg, int max_deg, bool wrap);

#ifdef __cplusplus
}
#endif
