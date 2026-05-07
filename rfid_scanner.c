// Minimal RFID scanner: prints antenna source + tag ID for each detection.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>

#include "CAENRFIDLib_Light.h"
#include "host.h"

#define ANTENNA_COUNT 2
#define MAX_ID_LENGTH 64

#define GREEN "\033[0;32m"
#define RESET "\033[0m"

volatile int running = 0;

static void hex_str(uint8_t *bytes, uint16_t len, char *out) {
    for (int i = 0; i < len; i++) sprintf(out + (i * 2), "%02X", bytes[i]);
    out[len * 2] = '\0';
}

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main(void) {
    CAENRFIDErrorCodes ec;
    CAENRFIDReader reader = {
        .connect       = _connect,
        .disconnect    = _disconnect,
        .tx            = _tx,
        .rx            = _rx,
        .clear_rx_data = _clear_rx_data,
        .enable_irqs   = _enable_irqs,
        .disable_irqs  = _disable_irqs
    };

    RS232_params port_params = {
        .com         = "/dev/ttyACM0",
        .baudrate    = 921600,
        .dataBits    = 8,
        .stopBits    = 1,
        .parity      = 0,
        .flowControl = 0,
    };

    const char *sources[ANTENNA_COUNT] = {"Source_0", "Source_1"};

    signal(SIGINT, handle_sigint);

    printf("[RFID] Connecting to %s...\n", port_params.com);
    ec = CAENRFID_Connect(&reader, CAENRFID_RS232, &port_params);
    if (ec != CAENRFID_StatusOK) {
        printf("[RFID] Connect failed (code %d)\n", ec);
        return -1;
    }

    CAENRFID_SetPower(&reader, 316);

    printf("[RFID] Scanning on %s and %s. Press Ctrl+C to stop.\n\n",
           sources[0], sources[1]);

    running = 1;
    while (running) {
        for (int a = 0; a < ANTENNA_COUNT && running; a++) {
            CAENRFIDTagList *tags = NULL, *node;
            uint16_t num_tags = 0;

            ec = CAENRFID_InventoryTag(&reader, (char *)sources[a],
                                       0, 0, 0, NULL, 0, 0,
                                       &tags, &num_tags);

            if (ec == CAENRFID_StatusOK) {
                node = tags;
                while (node != NULL) {
                    char epc[2 * MAX_ID_LENGTH + 1];
                    hex_str(node->Tag.ID, node->Tag.Length, epc);
                    printf(GREEN "[%s] %s" RESET "\n", sources[a], epc);

                    CAENRFIDTagList *next = node->Next;
                    free(node);
                    node = next;
                }
            }
        }
        usleep(50 * 1000);
    }

    CAENRFID_Disconnect(&reader);
    printf("\n[RFID] Disconnected.\n");
    return 0;
}
