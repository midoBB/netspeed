/*
 * SPDX-FileCopyrightText: 2025 Mohamed Hamdi <haamdi@outlook.com>
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include <getopt.h>
#include <regex.h>
#include <stdbit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_INTERFACES 32
#define MAX_LINE_LEN 256
#define MAX_NAME_LEN 16
#define DECIMAL_BASE 1000

typedef struct {
  char name[MAX_NAME_LEN];
  unsigned long long rx_bytes;
  unsigned long long tx_bytes;
} interface_t;

static unsigned int polling_interval = 1;
static char **interface_filter = NULL;
static int filter_count = 0;

void usage(const char *progname) {
  fprintf(stderr, "Usage: %s [-t POLLING_INTERVAL] [INTERFACE...]\n", progname);
  exit(1);
}

bool interface_exists(const char *name) {
  char path[64];
  struct stat st;
  snprintf(path, sizeof(path), "/sys/class/net/%s", name);
  return stat(path, &st) == 0;
}

bool is_valid_interface(const char *name) {
  static regex_t regex;
  static bool regex_compiled = false;

  if (!regex_compiled) {
    if (regcomp(&regex, "^(eth|wlan|enp|wlp)", REG_EXTENDED) != 0) {
      return false;
    }
    regex_compiled = true;
  }

  return regexec(&regex, name, 0, NULL, 0) == 0;
}

void format_human_readable(unsigned long long bytes, char output[static 16]) {
  static const char *const units[] = {"B", "K", "M", "G", "T", "P"};
  static const size_t num_units = sizeof(units) / sizeof(units[0]);

  if (bytes < DECIMAL_BASE) {
    snprintf(output, 16, "%llu%s", bytes, units[0]);
    return;
  }

  double value = (double)bytes;
  size_t unit_idx = 0;

  while (value >= DECIMAL_BASE && unit_idx < num_units - 1) {
    value /= 1000.0;
    unit_idx++;
  }

  snprintf(output, 16, "%.1f%s", value, units[unit_idx]);
}

void output_error(const char *text, const char *tooltip) {
  printf("{\"text\": \"⚠ %s\", \"tooltip\": \"%s\", \"class\": \"error\"}\n",
         text, tooltip);
  fflush(stdout);
}

void output_json(unsigned long long rx_rate, unsigned long long tx_rate) {
  char rx_str[16], tx_str[16];
  format_human_readable(rx_rate, rx_str);
  format_human_readable(tx_rate, tx_str);

  printf("{\"text\": \"%4s  %4s \"}\n",
         rx_str, tx_str);
  fflush(stdout);
}

int read_interfaces(interface_t interfaces[static MAX_INTERFACES]) {
  FILE *fp = fopen("/proc/net/dev", "r");
  if (!fp) {
    output_error("Error", "Cannot open /proc/net/dev");
    return -1;
  }

  char line[MAX_LINE_LEN];
  int count = 0;

  // Skip first two header lines
  fgets(line, sizeof(line), fp);
  fgets(line, sizeof(line), fp);

  while (fgets(line, sizeof(line), fp) && count < MAX_INTERFACES) {
    char *colon = strchr(line, ':');
    if (!colon)
      continue;

    *colon = '\0';
    char *name = line;

    // Strip leading whitespace from interface name
    while (*name == ' ')
      name++;

    // Skip if we have a filter list and this interface isn't in it
    if (filter_count > 0) {
      bool found = false;
      for (int i = 0; i < filter_count; i++) {
        if (strcmp(name, interface_filter[i]) == 0) {
          found = true;
          break;
        }
      }
      if (!found)
        continue;
    } else {
      // Auto-filter: only include standard network interfaces
      if (!is_valid_interface(name))
        continue;
    }

    // Parse the data after the colon
    char *data = colon + 1;
    unsigned long long rx_bytes, tx_bytes;
    unsigned long long dummy;

    if (sscanf(data,
               "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu "
               "%llu %llu %llu %llu",
               &rx_bytes, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
               &dummy, &tx_bytes, &dummy, &dummy, &dummy, &dummy, &dummy,
               &dummy, &dummy) == 16) {

      // C23 compound literal assignment
      interfaces[count] =
          (interface_t){.rx_bytes = rx_bytes, .tx_bytes = tx_bytes};
      snprintf(interfaces[count].name, MAX_NAME_LEN, "%.15s", name);
      count++;
    }
  }

  fclose(fp);
  return count;
}

void calculate_and_output_rates(const interface_t prev[static MAX_INTERFACES],
                                const interface_t curr[static MAX_INTERFACES],
                                int count) {
  unsigned long long total_rx_rate = 0, total_tx_rate = 0;

  for (int i = 0; i < count; i++) {
    // Find matching interface in previous data
    for (int j = 0; j < count; j++) {
      if (strcmp(curr[i].name, prev[j].name) == 0) {
        unsigned long long rx_rate = (curr[i].rx_bytes - prev[j].rx_bytes) /
                                      (unsigned long long)polling_interval;
        unsigned long long tx_rate = (curr[i].tx_bytes - prev[j].tx_bytes) /
                                     (unsigned long long)polling_interval;

        total_rx_rate += rx_rate;
        total_tx_rate += tx_rate;
        break;
      }
    }
  }

  output_json(total_rx_rate, total_tx_rate);
}

int main(int argc, char *argv[]) {
  int opt;

  // Parse command line arguments
  while ((opt = getopt(argc, argv, "t:")) != -1) {
    switch (opt) {
    case 't':
      polling_interval = atoi(optarg);
      if (polling_interval < 1) {
        output_error(optarg, "Invalid polling interval");
        return 1;
      }
      break;
    default:
      usage(argv[0]);
    }
  }

  // Store interface filter if provided
  if (optind < argc) {
    filter_count = argc - optind;
    interface_filter = &argv[optind];

    // Validate interface names
    for (int i = 0; i < filter_count; i++) {
      if (!interface_exists(interface_filter[i])) {
        output_error(interface_filter[i], "Interface does not exist");
        return 1;
      }
    }
  }

  interface_t prev_interfaces[MAX_INTERFACES] = {};
  interface_t curr_interfaces[MAX_INTERFACES] = {};
  int prev_count = 0;

  // Initial read
  prev_count = read_interfaces(prev_interfaces);
  if (prev_count <= 0) {
    return 1;
  }

  while (true) {
    sleep(polling_interval);

    int curr_count = read_interfaces(curr_interfaces);
    if (curr_count <= 0) {
      continue;
    }

    if (prev_count > 0) {
      calculate_and_output_rates(prev_interfaces, curr_interfaces, curr_count);
    }

    // Copy current to previous
    memcpy(prev_interfaces, curr_interfaces,
           sizeof(interface_t) * (size_t)curr_count);
    prev_count = curr_count;
  }

  return 0;
}
