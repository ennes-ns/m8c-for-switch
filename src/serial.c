// serial.c

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "serial.h"

#ifdef __SWITCH__

// Nintendo Switch-specific code (stub implementations)

int init_serial(const int verbose, const char *preferred_device) {
    if (verbose) {
        SDL_Log("Serial port functionality is not available on the Nintendo Switch.\n");
    }
    return 0; // Indicate success
}

int close_serial_port(void) {
    return 0; // Indicate success
}

int disconnect(void) {
    // Do nothing
    return 0; // Indicate success
}

int enable_and_reset_display(void) {
    // Do nothing
    return 0; // Indicate success
}

int reset_display(void) {
    // Do nothing
    return 0; // Indicate success
}

int send_msg_controller(uint8_t input) {
    // Do nothing
    return 0; // Indicate success
}

int send_msg_keyjazz(uint8_t note, uint8_t velocity) {
    // Do nothing
    return 0; // Indicate success
}

int serial_read(uint8_t *buffer, int len) {
    return 0; // Indicate no data read
}

int check_serial_port(void) {
    return 0; // Indicate serial port is not available
}

int list_devices(void) {
    // Do nothing
    return 0;
}

#else

// Code for other platforms (e.g., PC), using libserialport

#include <libserialport.h>

struct sp_port *m8_port = NULL;

// Helper function for error handling
static int check(enum sp_return result);

static int detect_m8_serial_device(const struct sp_port *port) {
    // Check the connection method - we want USB serial devices
    const enum sp_transport transport = sp_get_port_transport(port);

    if (transport == SP_TRANSPORT_USB) {
        // Get the USB vendor and product IDs.
        int usb_vid, usb_pid;
        sp_get_port_usb_vid_pid(port, &usb_vid, &usb_pid);

        if (usb_vid == 0x16C0 && usb_pid == 0x048A)
            return 1;
    }

    return 0;
}

int list_devices(void) {
    struct sp_port **port_list;
    const enum sp_return result = sp_list_ports(&port_list);

    if (result != SP_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "sp_list_ports() failed!\n");
        abort();
    }

    for (int i = 0; port_list[i] != NULL; i++) {
        const struct sp_port *port = port_list[i];

        if (detect_m8_serial_device(port)) {
            SDL_Log("Found M8 device: %s", sp_get_port_name(port));
        }
    }

    sp_free_port_list(port_list);
    return 0;
}

int check_serial_port(void) {

    int device_found = 0;

    struct sp_port **port_list;

    const enum sp_return result = sp_list_ports(&port_list);

    if (result != SP_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "sp_list_ports() failed!\n");
        abort();
    }

    for (int i = 0; port_list[i] != NULL; i++) {
        const struct sp_port *port = port_list[i];

        if (detect_m8_serial_device(port)) {
            if (strcmp(sp_get_port_name(port), sp_get_port_name(m8_port)) == 0)
                device_found = 1;
        }
    }

    sp_free_port_list(port_list);
    return device_found;
}

int init_serial(const int verbose, const char *preferred_device) {
    if (m8_port != NULL) {
        // Port is already initialized
        return 1;
    }

    struct sp_port **port_list;

    if (verbose)
        SDL_Log("Looking for USB serial devices.\n");

    enum sp_return result = sp_list_ports(&port_list);

    if (result != SP_OK) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "sp_list_ports() failed!\n");
        abort();
    }

    for (int i = 0; port_list[i] != NULL; i++) {
        const struct sp_port *port = port_list[i];

        if (detect_m8_serial_device(port)) {
            char *port_name = sp_get_port_name(port);
            SDL_Log("Found M8 in %s.\n", port_name);
            sp_copy_port(port, &m8_port);
            if (preferred_device != NULL && strcmp(preferred_device, port_name) == 0) {
                SDL_Log("Found preferred device, breaking");
                break;
            }
        }
    }

    sp_free_port_list(port_list);

    if (m8_port != NULL) {
        // Open the serial port and configure it
        SDL_Log("Opening port.");

        result = sp_open(m8_port, SP_MODE_READ_WRITE);
        if (check(result) != SP_OK)
            return 0;

        result = sp_set_baudrate(m8_port, 115200);
        if (check(result) != SP_OK)
            return 0;

        result = sp_set_bits(m8_port, 8);
        if (check(result) != SP_OK)
            return 0;

        result = sp_set_parity(m8_port, SP_PARITY_NONE);
        if (check(result) != SP_OK)
            return 0;

        result = sp_set_stopbits(m8_port, 1);
        if (check(result) != SP_OK)
            return 0;

        result = sp_set_flowcontrol(m8_port, SP_FLOWCONTROL_NONE);
        if (check(result) != SP_OK)
            return 0;
    } else {
        if (verbose) {
            SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "Cannot find a M8.\n");
        }
        return 0;
    }

    return 1;
}

static int check(const enum sp_return result) {

    char *error_message;

    switch (result) {
        case SP_ERR_ARG:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Invalid argument.\n");
            break;
        case SP_ERR_FAIL:
            error_message = sp_last_error_message();
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Failed: %s\n", error_message);
            sp_free_error_message(error_message);
            break;
        case SP_ERR_SUPP:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Not supported.\n");
            break;
        case SP_ERR_MEM:
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: Couldn't allocate memory.\n");
            break;
        case SP_OK:
        default:
            break;
    }
    return result;
}

int close_serial_port(void) {
    if (m8_port != NULL) {
        sp_close(m8_port);
        sp_free_port(m8_port);
        m8_port = NULL;
    }
    return 0;
}

int reset_display(void) {
    SDL_Log("Reset display\n");

    const char buf[1] = {'R'};
    int result = sp_blocking_write(m8_port, buf, 1, 5);
    if (result != 1) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Error resetting M8 display, code %d", result);
        return 0;
    }
    return 1;
}

int enable_and_reset_display(void) {

    SDL_Log("Enabling and resetting M8 display\n");

    const char buf[1] = {'E'};
    int result = sp_blocking_write(m8_port, buf, 1, 5);
    if (result != 1) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Error enabling M8 display, code %d", result);
        return 0;
    }

    result = reset_display();

    return result;
}

int disconnect(void) {

    SDL_Log("Disconnecting M8\n");

    const char buf[1] = {'D'};

    int result = sp_blocking_write(m8_port, buf, 1, 5);
    if (result != 1) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Error sending disconnect, code %d", result);
        result = 0;
    }
    close_serial_port();
    return result;
}

int serial_read(uint8_t *serial_buf, const int count) {
    return sp_nonblocking_read(m8_port, serial_buf, count);
}

int send_msg_controller(uint8_t input) {
    const char buf[2] = {'C', input};
    const size_t nbytes = 2;
    int result = sp_blocking_write(m8_port, buf, nbytes, 5);
    if (result != nbytes) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Error sending input, code %d", result);
        return -1;
    }
    return 1;
}

int send_msg_keyjazz(uint8_t note, uint8_t velocity) {
    if (velocity > 0x7F)
        velocity = 0x7F;
    const char buf[3] = {'K', note, velocity};
    const size_t nbytes = 3;
    int result = sp_blocking_write(m8_port, buf, nbytes, 5);
    if (result != nbytes) {
        SDL_LogError(SDL_LOG_CATEGORY_SYSTEM, "Error sending keyjazz, code %d", result);
        return -1;
    }

    return 1;
}

#endif // End of platform-specific code
