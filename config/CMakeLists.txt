zephyr_library()

zephyr_library_sources(src/sensor_rotation.c)
zephyr_library_sources(src/behavior_angle_step.c)

zephyr_include_directories(include)
