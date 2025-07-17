#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include <lvgl.h>     // LVGL main header
#include <LittleFS.h> // Arduino LittleFS library

// Ensure C linkage for LVGL callbacks if compiling with C++
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes and registers the LVGL File System driver for Arduino ESP LittleFS.
 *
 * This function should be called after LittleFS.begin() and lv_init().
 * It sets up the necessary callbacks for LVGL to interact with the LittleFS filesystem
 * using the 'S' drive letter (as defined in lv_conf.h by LV_FS_ARDUINO_ESP_LITTLEFS_LETTER).
 */
void init_lvgl_littlefs_driver();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // LVGL_DRIVER_H
