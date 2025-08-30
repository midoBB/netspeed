<!--
SPDX-FileCopyrightText: 2025 Mohamed Hamdi <haamdi@outlook.com>

SPDX-License-Identifier: MPL-2.0
-->

# netspeed

Network interface speed monitoring tool for Waybar that outputs JSON formatted bandwidth statistics.

## Build and Install
```bash
make
sudo make install
```

## Usage
```bash
netspeed [-t POLLING_INTERVAL] [INTERFACE...]
```

## Usage Examples
```bash
netspeed                    # Auto-detect interfaces
netspeed -t 2               # 2-second polling
netspeed eth0 wlan0         # Monitor specific interfaces
netspeed -t 5 enp3s0        # 5-second polling on specific interface
```

## Waybar Config
```json
"custom/netspeed": {
    "exec": "netspeed",
    "return-type": "json"
}
```

## Requirements
- GCC 14+ or Clang 18+ for C23 support