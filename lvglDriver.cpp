#include "lvglDriver.h" // Include our custom header
#include <Arduino.h>    // For Serial.println and other Arduino functions
#include <string.h>     // For strncpy

// Define the static lv_fs_drv_t instance that will hold the driver callbacks.
// This needs to be static or global so its memory persists after init_lvgl_littlefs_driver returns.
static lv_fs_drv_t fs_drv;

// --- LVGL File System Callbacks ---

/**
 * @brief Callback function to open a file for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param path Path to the file (without the drive letter).
 * @param mode Mode to open the file (read, write, or both).
 * @return A pointer to a File object (Arduino LittleFS) on success, or NULL on failure.
 */
static void * fs_open_cb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    LV_UNUSED(drv); // Mark parameter as unused to suppress warnings

    // Dynamically allocate a File object to hold the LittleFS file handle.
    // This memory must be freed in fs_close_cb.
    File * file = new File(); 
    if (file == NULL) {
        Serial.println("Error: Failed to allocate memory for File object.");
        return NULL;
    }

    // Prepend '/' to the path for LittleFS compatibility
    char fullPath[LV_FS_MAX_PATH_LEN]; // Use LV_FS_MAX_PATH_LEN from lv_conf.h
    snprintf(fullPath, sizeof(fullPath), "/%s", path);

    // Determine the LittleFS open mode based on LVGL's mode flags.
    // Note: LittleFS "r+" requires the file to exist. "w+" creates/truncates.
    // "a+" appends/creates. Adjust as needed for specific file access patterns.
    if (mode == LV_FS_MODE_WR) {
        *file = LittleFS.open(fullPath, "w"); // Open for writing, creates if not exists, truncates if exists
    } else if (mode == LV_FS_MODE_RD) {
        *file = LittleFS.open(fullPath, "r"); // Open for reading
    } else if ((mode & LV_FS_MODE_WR) && (mode & LV_FS_MODE_RD)) {
        *file = LittleFS.open(fullPath, "w+"); // Open for read/write, creates if not exists, truncates if exists
    } else {
        Serial.printf("Error: Unsupported LVGL file mode: %d\n", mode);
        delete file;
        return NULL;
    }

    // Check if the file was opened successfully.
    if (!file->operator bool()) {
        Serial.printf("Error: Failed to open file '%s' (full path: '%s') with mode %d\n", path, fullPath, mode);
        delete file; // Free memory if open failed
        return NULL;
    }
    return (void *)file; // Return pointer to the File object
}

/**
 * @brief Callback function to close an opened file for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param file_p Pointer to the File object to close.
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_close_cb(lv_fs_drv_t * drv, void * file_p) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * file = (File *)file_p;
    if (file) {
        file->close();
        delete file; // Free the dynamically allocated File object
    }
    return LV_FS_RES_OK;
}

/**
 * @brief Callback function to read data from an opened file for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param file_p Pointer to the File object to read from.
 * @param buf Pointer to the buffer to store the read data.
 * @param btr Bytes To Read.
 * @param br Pointer to store the actual Bytes Read.
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_read_cb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * file = (File *)file_p;
    if (!file || !file->operator bool()) {
        *br = 0;
        return LV_FS_RES_DENIED; // File not open or invalid
    }
    *br = file->readBytes((char *)buf, btr);
    return (*br > 0 || btr == 0) ? LV_FS_RES_OK : LV_FS_RES_DENIED; // DENIED can indicate EOF or error
}

/**
 * @brief Callback function to write data to an opened file for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param file_p Pointer to the File object to write to.
 * @param buf Pointer to the buffer containing data to write.
 * @param btw Bytes To Write.
 * @param bw Pointer to store the actual Bytes Written.
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_write_cb(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * file = (File *)file_p;
    if (!file || !file->operator bool()) {
        *bw = 0;
        return LV_FS_RES_DENIED; // File not open or invalid
    }
    *bw = file->write((const uint8_t *)buf, btw);
    return (*bw == btw) ? LV_FS_RES_OK : LV_FS_RES_DENIED;
}

/**
 * @brief Callback function to set the read/write pointer position in an opened file for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param file_p Pointer to the File object.
 * @param pos The new position in bytes from the origin.
 * @param whence Origin for the seek operation (LV_FS_SEEK_SET, LV_FS_SEEK_CUR, LV_FS_SEEK_END).
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_seek_cb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * file = (File *)file_p;
    if (!file || !file->operator bool()) {
        return LV_FS_RES_DENIED; // File not open or invalid
    }

    SeekMode littlefs_whence;
    switch (whence) {
        case LV_FS_SEEK_SET: littlefs_whence = SeekSet; break;
        case LV_FS_SEEK_CUR: littlefs_whence = SeekCur; break;
        case LV_FS_SEEK_END: littlefs_whence = SeekEnd; break;
        default: return LV_FS_RES_INV_PARAM; // Invalid seek origin
    }
    return file->seek(pos, littlefs_whence) ? LV_FS_RES_OK : LV_FS_RES_DENIED;
}

/**
 * @brief Callback function to get the current read/write pointer position in an opened file for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param file_p Pointer to the File object.
 * @param pos_p Pointer to store the current position.
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_tell_cb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * file = (File *)file_p;
    if (!file || !file->operator bool()) {
        return LV_FS_RES_DENIED; // File not open or invalid
    }
    *pos_p = file->position();
    return LV_FS_RES_OK;
}

/**
 * @brief Callback function to open a directory for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param path Path to the directory.
 * @return A pointer to a File object (representing a directory) on success, or NULL on failure.
 */
static void * fs_dir_open_cb(lv_fs_drv_t * drv, const char * path) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * dir = new File(); // Dynamically allocate File object for directory
    if (dir == NULL) {
        Serial.println("Error: Failed to allocate memory for Directory object.");
        return NULL;
    }
    // Prepend '/' to the path for LittleFS compatibility
    char fullPath[LV_FS_MAX_PATH_LEN]; // Use LV_FS_MAX_PATH_LEN from lv_conf.h
    snprintf(fullPath, sizeof(fullPath), "/%s", path);

    *dir = LittleFS.open(fullPath);
    if (!dir->isDirectory()) {
        Serial.printf("Error: Failed to open directory '%s' (full path: '%s') or it's not a directory.\n", path, fullPath);
        delete dir;
        return NULL;
    }
    return (void *)dir;
}

/**
 * @brief Callback function to read the next entry from an opened directory for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param rddir_p Pointer to the File object representing the opened directory.
 * @param fn Buffer to store the filename of the next entry.
 * @param len Maximum length of the filename buffer (LV_FS_MAX_PATH_LEN).
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_dir_read_cb(lv_fs_drv_t * drv, void * rddir_p, char * fn, uint32_t len) { // Added 'len' parameter
    LV_UNUSED(drv); // Mark parameter as unused
    File * dir = (File *)rddir_p;
    if (!dir || !dir->operator bool() || !dir->isDirectory()) {
        fn[0] = '\0'; // Indicate end of directory or error
        return LV_FS_RES_DENIED;
    }

    File entry = dir->openNextFile();
    if (!entry) {
        fn[0] = '\0'; // No more files
        return LV_FS_RES_OK;
    }
    // Copy filename, ensuring null-termination and buffer bounds using 'len'
    strncpy(fn, entry.name(), len - 1); // Use 'len' here instead of LV_FS_MAX_PATH
    fn[len - 1] = '\0'; // Ensure null-termination within the provided buffer size
    entry.close(); // Close the individual file/directory entry, not the directory handle
    return LV_FS_RES_OK;
}

/**
 * @brief Callback function to close an opened directory for LVGL.
 * @param drv Pointer to the LVGL file system driver.
 * @param rddir_p Pointer to the File object representing the opened directory.
 * @return LV_FS_RES_OK on success, or an LV_FS_RES_t error code on failure.
 */
static lv_fs_res_t fs_dir_close_cb(lv_fs_drv_t * drv, void * rddir_p) {
    LV_UNUSED(drv); // Mark parameter as unused
    File * dir = (File *)rddir_p;
    if (dir) {
        dir->close();
        delete dir; // Free the dynamically allocated File object
    }
    return LV_FS_RES_OK;
}

/**
 * @brief Initializes and registers the LVGL File System driver for Arduino ESP LittleFS.
 * This function should be called after LittleFS.begin() and lv_init().
 */
void init_lvgl_littlefs_driver() {
    // Initialize the LVGL file system driver structure with default values.
    lv_fs_drv_init(&fs_drv);

    // Assign the drive letter defined in lv_conf.h
    fs_drv.letter = LV_FS_ARDUINO_ESP_LITTLEFS_LETTER;

    // Assign the callback functions for file operations.
    fs_drv.open_cb = fs_open_cb;
    fs_drv.close_cb = fs_close_cb;
    fs_drv.read_cb = fs_read_cb;
    fs_drv.write_cb = fs_write_cb;
    fs_drv.seek_cb = fs_seek_cb;
    fs_drv.tell_cb = fs_tell_cb;

    // Assign the callback functions for directory operations (optional, but recommended).
    fs_drv.dir_open_cb = fs_dir_open_cb;
    fs_drv.dir_read_cb = fs_dir_read_cb; // This assignment now matches the new signature
    fs_drv.dir_close_cb = fs_dir_close_cb;

    // Register the initialized driver with LVGL.
    lv_fs_drv_register(&fs_drv);

    Serial.printf("LVGL LittleFS driver registered with letter '%c'.\n", LV_FS_ARDUINO_ESP_LITTLEFS_LETTER);
}
