#!/bin/bash
set -e

gcc \
  rfid_scanner.c \
  SRC/host.c SRC/CAENRFIDLib_Light.c SRC/IO_Light.c \
  -ISRC \
  -o rfid_scanner \
  -lpthread -lm \
  -Wall

chmod +x rfid_scanner
echo "Built rfid_scanner. Run with: ./rfid_scanner"
