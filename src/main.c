/*
 * BSD 3-Clause License
 * 
 * Copyright (c) 2025, Ariz Kamizuki
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file launch.c
 * @brief Application Bundle Launcher
 * @version 1.0.0
 * @author Ariz Kamizuki
 * @date 2025
 * 
 * A robust application launcher that manages bundled applications with
 * proper library path handling, comprehensive error checking, and
 * logging capabilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>
#include <stdarg.h>

/* Application Configuration */
#define APP_NAME            "Application Launcher"
#define APP_VERSION         "1.0.0"
#define MAX_PATH_LENGTH     PATH_MAX
#define MAX_ENV_LENGTH      4096
#define MAX_BUFFER_SIZE     2048

/* Bundle Structure Definitions */
#define EXEC_PATH           "/exec/base"
#define LIB_PATH            "/library"
#define RES_PATH            "/resources"
#define METADATA_PATH       "/info.yaml"
#define ICON_PATH           "/icon.png"

/* Exit Codes */
#define EXIT_SUCCESS        0
#define EXIT_INVALID_ARGS   1
#define EXIT_BUNDLE_ERROR   2
#define EXIT_EXEC_ERROR     3
#define EXIT_SYSTEM_ERROR   4

/* Log Levels */
typedef enum {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG
} log_level_t;

/**
 * @brief Print formatted log message with timestamp
 * @param level Log level
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void log_message(log_level_t level, const char *format, ...) {
    time_t now;
    struct tm *timeinfo;
    char timestamp[64];
    const char *level_str;
    const char *level_icon;
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    switch (level) {
        case LOG_INFO:
            level_str = "INFO";
            level_icon = "ℹ️";
            break;
        case LOG_WARNING:
            level_str = "WARN";
            level_icon = "⚠️";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            level_icon = "❌";
            break;
        case LOG_DEBUG:
            level_str = "DEBUG";
            level_icon = "🔍";
            break;
        default:
            level_str = "UNKNOWN";
            level_icon = "❓";
    }
    
    fprintf(level == LOG_ERROR ? stderr : stdout, 
            "[%s] %s %s: ", timestamp, level_icon, level_str);
    
    va_list args;
    va_start(args, format);
    vfprintf(level == LOG_ERROR ? stderr : stdout, format, args);
    va_end(args);
    
    fprintf(level == LOG_ERROR ? stderr : stdout, "\n");
}

/**
 * @brief Check if file exists and is accessible
 * @param path File path to check
 * @return 1 if file exists, 0 otherwise
 */
int file_exists(const char *path) {
    if (!path || strlen(path) == 0) {
        return 0;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
}

/**
 * @brief Check if directory exists and is accessible
 * @param path Directory path to check
 * @return 1 if directory exists, 0 otherwise
 */
int directory_exists(const char *path) {
    if (!path || strlen(path) == 0) {
        return 0;
    }
    
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    
    return S_ISDIR(st.st_mode);
}

/**
 * @brief Validate bundle structure
 * @param bundle_path Path to application bundle
 * @return EXIT_SUCCESS on success, error code on failure
 */
int validate_bundle(const char *bundle_path) {
    char full_path[MAX_PATH_LENGTH];
    
    if (!directory_exists(bundle_path)) {
        log_message(LOG_ERROR, "Bundle directory not found: %s", bundle_path);
        return EXIT_BUNDLE_ERROR;
    }
    
    // Check for required executable
    snprintf(full_path, sizeof(full_path), "%s%s", bundle_path, EXEC_PATH);
    if (!file_exists(full_path)) {
        log_message(LOG_ERROR, "Required executable not found: %s", full_path);
        return EXIT_BUNDLE_ERROR;
    }
    
    // Check if executable is actually executable
    if (access(full_path, X_OK) != 0) {
        log_message(LOG_ERROR, "Executable lacks execute permissions: %s", full_path);
        return EXIT_BUNDLE_ERROR;
    }
    
    log_message(LOG_INFO, "Bundle validation successful: %s", bundle_path);
    return EXIT_SUCCESS;
}

/**
 * @brief Configure library path environment
 * @param bundle_path Path to application bundle
 * @return EXIT_SUCCESS on success, error code on failure
 */
int configure_library_path(const char *bundle_path) {
    char lib_full_path[MAX_PATH_LENGTH];
    char new_ld_path[MAX_ENV_LENGTH];
    const char *current_ld_path;
    
    snprintf(lib_full_path, sizeof(lib_full_path), "%s%s", bundle_path, LIB_PATH);
    
    // Check if library directory exists
    if (!directory_exists(lib_full_path)) {
        log_message(LOG_WARNING, "Library directory not found: %s", lib_full_path);
        return EXIT_SUCCESS; // Not critical, continue execution
    }
    
    current_ld_path = getenv("LD_LIBRARY_PATH");
    
    if (current_ld_path && strlen(current_ld_path) > 0) {
        if (strlen(lib_full_path) + strlen(current_ld_path) + 2 > MAX_ENV_LENGTH) {
            log_message(LOG_ERROR, "LD_LIBRARY_PATH would exceed maximum length");
            return EXIT_SYSTEM_ERROR;
        }
        snprintf(new_ld_path, sizeof(new_ld_path), "%s:%s", lib_full_path, current_ld_path);
    } else {
        snprintf(new_ld_path, sizeof(new_ld_path), "%s", lib_full_path);
    }
    
    if (setenv("LD_LIBRARY_PATH", new_ld_path, 1) != 0) {
        log_message(LOG_ERROR, "Failed to set LD_LIBRARY_PATH: %s", strerror(errno));
        return EXIT_SYSTEM_ERROR;
    }
    
    log_message(LOG_INFO, "Library path configured: %s", lib_full_path);
    log_message(LOG_DEBUG, "Full LD_LIBRARY_PATH: %s", new_ld_path);
    
    return EXIT_SUCCESS;
}

/**
 * @brief Inspect optional bundle components
 * @param bundle_path Path to application bundle
 */
void inspect_optional_components(const char *bundle_path) {
    char component_path[MAX_PATH_LENGTH];
    
    // Check metadata file
    snprintf(component_path, sizeof(component_path), "%s%s", bundle_path, METADATA_PATH);
    if (file_exists(component_path)) {
        log_message(LOG_INFO, "Metadata file found: %s", component_path);
    } else {
        log_message(LOG_DEBUG, "Metadata file not present");
    }
    
    // Check icon file
    snprintf(component_path, sizeof(component_path), "%s%s", bundle_path, ICON_PATH);
    if (file_exists(component_path)) {
        log_message(LOG_INFO, "Icon file found: %s", component_path);
    } else {
        log_message(LOG_DEBUG, "Icon file not present");
    }
    
    // Check resources directory
    snprintf(component_path, sizeof(component_path), "%s%s", bundle_path, RES_PATH);
    if (directory_exists(component_path)) {
        log_message(LOG_INFO, "Resources directory found: %s", component_path);
    } else {
        log_message(LOG_DEBUG, "Resources directory not present");
    }
}

/**
 * @brief Launch the application
 * @param bundle_path Path to application bundle
 * @return EXIT_SUCCESS on success, error code on failure
 */
int launch_application(const char *bundle_path) {
    char exec_path[MAX_PATH_LENGTH];
    int result;
    
    // Validate bundle structure
    result = validate_bundle(bundle_path);
    if (result != EXIT_SUCCESS) {
        return result;
    }
    
    // Configure environment
    result = configure_library_path(bundle_path);
    if (result != EXIT_SUCCESS) {
        return result;
    }
    
    // Inspect optional components
    inspect_optional_components(bundle_path);
    
    // Prepare execution
    snprintf(exec_path, sizeof(exec_path), "%s%s", bundle_path, EXEC_PATH);
    
    log_message(LOG_INFO, "Launching application: %s", exec_path);
    log_message(LOG_DEBUG, "Working directory: %s", getcwd(NULL, 0));
    
    // Execute application
    char *const argv[] = { exec_path, NULL };
    char *const envp[] = { NULL }; // Use current environment
    
    execv(exec_path, argv);
    
    // If we reach here, execv failed
    log_message(LOG_ERROR, "Failed to execute application: %s", strerror(errno));
    return EXIT_EXEC_ERROR;
}

/**
 * @brief Print application usage information
 * @param program_name Name of the program
 */
void print_usage(const char *program_name) {
    printf("%s v%s\n", APP_NAME, APP_VERSION);
    printf("A professional application bundle launcher for Linux systems.\n\n");
    printf("Usage: %s <bundle_path>\n\n", program_name);
    printf("Arguments:\n");
    printf("  bundle_path    Path to the application bundle directory\n\n");
    printf("Expected Bundle Structure:\n");
    printf("  bundle_path/\n");
    printf("  ├── exec/base          (Required executable)\n");
    printf("  ├── library/           (Optional shared libraries)\n");
    printf("  ├── resources/         (Optional resource files)\n");
    printf("  ├── info.yaml          (Optional metadata)\n");
    printf("  └── icon.png           (Optional application icon)\n\n");
    printf("Exit Codes:\n");
    printf("  0  Success\n");
    printf("  1  Invalid arguments\n");
    printf("  2  Bundle validation error\n");
    printf("  3  Application execution error\n");
    printf("  4  System error\n");
}

/**
 * @brief Main entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc != 2) {
        if (argc > 2) {
            log_message(LOG_ERROR, "Too many arguments provided");
        } else {
            log_message(LOG_ERROR, "Missing required bundle path argument");
        }
        print_usage(argv[0]);
        return EXIT_INVALID_ARGS;
    }
    
    const char *bundle_path = argv[1];
    
    // Validate bundle path length
    if (strlen(bundle_path) >= MAX_PATH_LENGTH) {
        log_message(LOG_ERROR, "Bundle path too long (max %d characters)", MAX_PATH_LENGTH - 1);
        return EXIT_INVALID_ARGS;
    }
    
    // Initialize logging
    log_message(LOG_INFO, "Starting %s v%s", APP_NAME, APP_VERSION);
    log_message(LOG_INFO, "Target bundle: %s", bundle_path);
    
    // Launch application
    int result = launch_application(bundle_path);
    
    // This should never be reached if execv succeeds
    log_message(LOG_ERROR, "Application launcher terminated unexpectedly");
    return result;
}