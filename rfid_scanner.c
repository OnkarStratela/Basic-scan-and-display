// Minimal RFID scanner: prints antenna source + tag ID for each detection.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>

#include "CAENRFIDLib_Light.h"
#include "host.h"

#define ANTENNA_COUNT 2
#define MAX_ID_LENGTH 64
#define MAX_TAGS 1024  // Max unique tags remembered per antenna

// ANSI color codes
#define GREEN "\033[0;32m"
#define RESET "\033[0m"

volatile int running = 0;
volatile int scanRate = 25;

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
        .com         = "/dev/ttyACM0",   // Or /dev/ttyUSB0 depending on your setup
        .baudrate    = 921600,           // Common CAEN baudrate
        .dataBits    = 8,
        .stopBits    = 1,
        .parity      = 0,
        .flowControl = 0,
    };

    const char *sources[ANTENNA_COUNT] = {"Source_0", "Source_1"};
    int power = 316;  // Start with 140mW (10% on CAEN)

    signal(SIGINT, handle_sigint);

    printf("[RFID] Connecting to CAEN reader on %s at %d baud...\n",
           port_params.com, port_params.baudrate);
    ec = CAENRFID_Connect(&reader, CAENRFID_RS232, &port_params);
    if (ec != CAENRFID_StatusOK) {
        printf("[RFID] Connect failed (code %d). Check:\n", ec);
        printf("  - Is reader connected to %s?\n", port_params.com);
        printf("  - Try different baudrate (115200, 460800, 921600)\n");
        printf("  - Check USB permissions: sudo usermod -a -G dialout $USER\n");
        printf("  - Try: sudo chmod 666 %s\n", port_params.com);
        return -1;
    }

    CAENRFID_SetPower(&reader, power);
    printf("[RFID] Power set to %d mW\n", power);

    printf("[RFID] Scanning on %s and %s (scan rate: %dms). Press Ctrl+C to stop.\n\n",
           sources[0], sources[1], scanRate);

    // Per-antenna persistent dedupe: a given EPC is reported at most once
    // per antenna for the lifetime of this run. Restart the program to reset.
    static char seen[ANTENNA_COUNT][MAX_TAGS][2 * MAX_ID_LENGTH + 1];
    int seen_count[ANTENNA_COUNT] = {0};

    running = 1;
    while (running) {
        for (int a = 0; a < ANTENNA_COUNT && running; a++) {
            CAENRFIDTagList *tags = NULL, *node;
            uint16_t num_tags = 0;

            // Request RSSI alongside the EPC so Tag.RSSI is populated.
            ec = CAENRFID_InventoryTag(&reader, (char *)sources[a],
                                       0, 0, 0, NULL, 0, RSSI,
                                       &tags, &num_tags);

            if (ec == CAENRFID_StatusOK) {
                node = tags;
                while (node != NULL) {
                    char epc[2 * MAX_ID_LENGTH + 1];
                    hex_str(node->Tag.ID, node->Tag.Length, epc);

                    bool already_seen = false;
                    for (int i = 0; i < seen_count[a]; i++) {
                        if (strcmp(seen[a][i], epc) == 0) {
                            already_seen = true;
                            break;
                        }
                    }

                    if (!already_seen) {
                        printf(GREEN "[%s] %s (RSSI: %d dBm)" RESET "\n",
                               sources[a], epc, node->Tag.RSSI);
                        if (seen_count[a] < MAX_TAGS) {
                            strcpy(seen[a][seen_count[a]++], epc);
                        }
                    }

                    CAENRFIDTagList *next = node->Next;
                    free(node);
                    node = next;
                }
            }
        }
        usleep(scanRate * 1000);
    }

    CAENRFID_Disconnect(&reader);
    printf("\n[RFID] Disconnected.\n");
    return 0;
}
