# Minimal RFID Scanner

Scans both antennas (`Source_0`, `Source_1`) on a CAEN RS232 RFID reader and
prints the antenna source and tag ID for every detection.

## Build

```bash
chmod +x compile.sh
./compile.sh
```

## Run

```bash
./rfid_scanner
```

## Output

```
[Source_0] E20000172211010418905449
[Source_1] E2000017221101041890ABCD
```

## Notes

- Reader port: `/dev/ttyACM0` (edit `rfid_scanner.c` if different).
- If permission denied: `sudo chmod 666 /dev/ttyACM0`.
