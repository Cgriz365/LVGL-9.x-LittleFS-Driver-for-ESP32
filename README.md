LVGL LittleFS Driver for Arduino ESP32

This repository provides a custom file system driver for LVGL (Light and Versatile Graphics Library) to integrate with the LittleFS filesystem on ESP32 microcontrollers using the Arduino framework. This allows your LVGL applications to read from and write to the ESP32's flash memory via LittleFS.

Features

    LVGL File System Abstraction: Seamlessly integrates LittleFS with LVGL's lv_fs_drv_t interface.

    Read/Write Operations: Supports standard file operations like opening, closing, reading, writing, seeking, and telling.

    Directory Operations: Includes support for opening, reading, and closing directories, allowing for file browsing.

    Arduino ESP32 Compatibility: Designed specifically for the Arduino framework on ESP32 devices.

Prerequisites

Before using this driver, ensure you have the following installed and configured:

    Arduino IDE (or PlatformIO with Arduino framework for ESP32).

    ESP32 Board Package installed in your Arduino IDE.

    LittleFS for ESP32 Library: This is usually included with the ESP32 board package, but ensure it's available.

    LVGL Library: Install the LVGL library (version 9.x or newer is recommended and tested with this driver) via the Arduino Library Manager or manually.

Installation

    If using platformIO include "board_build.filesystem = littlefs" in your platform.ini file

    Download the Driver Files:

        Save the lvglDriver.h content into a file named lvglDriver.h.

        Save the lvglDriver.cpp content into a file named lvglDriver.cpp.

        Place both files in your Arduino project's sketch folder (e.g., MyProject/lvglDriver.h and MyProject/lvglDriver.cpp). If you prefer, you can put them in a subfolder like MyProject/src/LVGL_LittleFS/.

    Configure lv_conf.h:
    Ensure your lv_conf.h (which should be accessible by your LVGL library setup) has the following definitions enabled:

    #define LV_USE_FS_ARDUINO_ESP_LITTLEFS 1
    #if LV_USE_FS_ARDUINO_ESP_LITTLEFS
        #define LV_FS_ARDUINO_ESP_LITTLEFS_LETTER 'S' /**< Set an upper-case driver-identifier letter for this driver (e.g. 'A'). */
        #define LV_FS_ARDUINO_ESP_LITTLEFS_PATH ""    /**< Set the working directory. File/directory paths will be appended to it. */
    #endif

    The LV_FS_ARDUINO_ESP_LITTLEFS_LETTER defines the drive letter LVGL will use (e.g., 'S' for LittleFS).

Usage

To use the LVGL LittleFS driver in your Arduino sketch (.ino file):

    Include necessary headers:

    #include <LittleFS.h>   // For LittleFS.begin()
    #include <lvgl.h>       // For LVGL core functions
    #include "lvglDriver.h" // For init_lvgl_littlefs_driver()

    Initialize LittleFS:
    Call LittleFS.begin() early in your setup() function. This mounts the LittleFS filesystem.

    void setup() {
        Serial.begin(115200);
        if (!LittleFS.begin()) {
            Serial.println("FATAL: An Error has occurred while mounting LittleFS. Halting.");
            while (true); // Or handle error appropriately
        }
        Serial.println("LittleFS mounted successfully.");
        // ... rest of setup
    }

    Initialize LVGL:
    Call lv_init() after LittleFS is initialized.

    void setup() {
        // ... LittleFS.begin() ...
        lv_init();
        // ... rest of setup
    }

    Initialize and Register the LVGL LittleFS Driver:
    Call init_lvgl_littlefs_driver() after lv_init(). This function, provided by lvglDriver.cpp, sets up LVGL's internal file system driver with the necessary callbacks to interact with LittleFS.

    void setup() {
        // ... LittleFS.begin() ...
        // ... lv_init() ...
        init_lvgl_littlefs_driver();
        // ... rest of setup
    }

    Perform File Operations with LVGL API:
    Once the driver is registered, you can use standard LVGL file system functions (e.g., lv_fs_open, lv_fs_read, lv_fs_write, lv_fs_close, lv_fs_dir_open, etc.) with paths prefixed by your chosen drive letter (e.g., "S:my_file.txt").

    // Example: Writing to a file
    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, "S:my_data.txt", LV_FS_MODE_WR);
    if (res == LV_FS_RES_OK) {
        lv_fs_write(&f, "Hello LittleFS!", 15, NULL);
        lv_fs_close(&f);
    }

    // Example: Reading from a file
    char buffer[50];
    uint32_t bytes_read;
    res = lv_fs_open(&f, "S:my_data.txt", LV_FS_MODE_RD);
    if (res == LV_FS_RES_OK) {
        lv_fs_read(&f, buffer, sizeof(buffer) - 1, &bytes_read);
        buffer[bytes_read] = '\0';
        lv_fs_close(&f);
        Serial.printf("Read: %s\n", buffer);
    }

Example

The lvgl_littlefs_usage_example Arduino sketch (your main .ino file) demonstrates the full usage, including:

    Initialization of LittleFS and LVGL.

    Registration of this custom LVGL LittleFS driver.

    Writing data to a file (S:test_file.txt).

    Reading data from the same file and verifying its content.

    Testing file seeking and telling operations.

    Testing opening a non-existent file (expected to fail with LV_FS_RES_NOT_EX).

    Listing the contents of the root directory (S:/).

Remember to replace the display and input driver stubs in the example with your actual hardware-specific implementations for LVGL to render on your screen and respond to touch.
Troubleshooting

    open(): <filename> does not start with /: This error indicates that the path passed to the underlying LittleFS open() function is missing the leading /. The lvglDriver.cpp file is designed to automatically prepend this, so ensure you are using the latest version of lvglDriver.cpp and perform a clean build of your project.

    unknown driver letter: This means LVGL has not successfully registered the 'S' drive letter with the LittleFS driver. Ensure init_lvgl_littlefs_driver() is called after lv_init().

    Compilation Errors related to LV_FS_MAX_PATH: In LVGL 9.x, LV_FS_MAX_PATH was renamed to LV_FS_MAX_PATH_LEN. Ensure your lvglDriver.cpp uses LV_FS_MAX_PATH_LEN.

    Compilation Errors related to fs_dir_read_cb signature: LVGL 9.x changed the dir_read_cb signature to include a len parameter. Ensure your fs_dir_read_cb in lvglDriver.cpp matches the updated signature: static lv_fs_res_t fs_dir_read_cb(lv_fs_drv_t * drv, void * rddir_p, char * fn, uint32_t len).

If you encounter persistent issues, double-check your lv_conf.h settings, ensure all libraries are correctly installed, and perform a clean build.
