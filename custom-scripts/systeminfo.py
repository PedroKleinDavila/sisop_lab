#!/usr/bin/env python3

import json
import os
import re
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from datetime import datetime


def _read_text(path, default=""):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as file:
            return file.read().strip()
    except (OSError, IOError):
        return default


def _read_cpu_times():
    line = _read_text("/proc/stat").splitlines()[0]
    parts = line.split()

    if not parts or parts[0] != "cpu":
        return 0, 0

    values = [int(value) for value in parts[1:]]

    idle = values[3] if len(values) > 3 else 0
    iowait = values[4] if len(values) > 4 else 0

    idle_total = idle + iowait
    total = sum(values)

    return idle_total, total


def _hex_le_to_ipv4(value):
    if len(value) != 8:
        return None

    try:
        raw = bytes.fromhex(value)
    except ValueError:
        return None

    return ".".join(str(byte) for byte in reversed(raw))


def _ipv4_to_int(ip_address):
    try:
        parts = [int(part) for part in ip_address.split(".")]
    except ValueError:
        return None

    if len(parts) != 4 or any(part < 0 or part > 255 for part in parts):
        return None

    value = 0
    for part in parts:
        value = (value << 8) | part

    return value


def get_datetime():
    """
    Deriva a data/hora atual a partir de:
      - /proc/stat   -> btime (instante do boot em Unix timestamp)
      - /proc/uptime -> segundos desde o boot
    """
    boot_time = None

    for line in _read_text("/proc/stat").splitlines():
        if line.startswith("btime "):
            try:
                boot_time = int(line.split()[1])
            except (IndexError, ValueError):
                pass
            break

    if boot_time is None:
        return "unknown"

    current_timestamp = boot_time + get_uptime()
    return datetime.fromtimestamp(current_timestamp).strftime("%Y-%m-%d %H:%M:%S")


def get_uptime():
    content = _read_text("/proc/uptime")

    if not content:
        return 0

    try:
        return round(float(content.split()[0]), 2)
    except (IndexError, ValueError):
        return 0


def get_cpu_info():
    cpuinfo = _read_text("/proc/cpuinfo")

    model = "unknown"
    speed_mhz = 0.0
    cpu_fields = {}

    for line in cpuinfo.splitlines():
        if ":" not in line:
            continue

        key, value = [part.strip() for part in line.split(":", 1)]
        key_lower = key.lower()

        if key_lower not in cpu_fields and value:
            cpu_fields[key_lower] = value

        if speed_mhz == 0.0 and key_lower == "cpu mhz":
            try:
                speed_mhz = round(float(value), 2)
            except ValueError:
                pass

    for key in ("model name", "hardware", "processor"):
        value = cpu_fields.get(key)
        if value and not (key == "processor" and value.isdigit()):
            model = value
            break

    if speed_mhz == 0.0:
        for path in (
            "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
            "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq",
        ):
            value = _read_text(path)
            if value:
                try:
                    speed_mhz = round(float(value) / 1000.0, 2)
                    break
                except ValueError:
                    pass

    idle_before, total_before = _read_cpu_times()
    time.sleep(0.1)
    idle_after, total_after = _read_cpu_times()

    idle_delta = idle_after - idle_before
    total_delta = total_after - total_before

    usage_percent = 0.0
    if total_delta > 0:
        usage_percent = round(
            100.0 * (1.0 - (idle_delta / total_delta)),
            2,
        )

    return {
        "model": model,
        "speed_mhz": speed_mhz,
        "usage_percent": usage_percent,
    }


def get_memory_info():
    values = {}

    for line in _read_text("/proc/meminfo").splitlines():
        if ":" not in line:
            continue

        key, raw_value = line.split(":", 1)
        match = re.search(r"(\d+)", raw_value)

        if match:
            values[key] = int(match.group(1))

    total_kb = values.get("MemTotal", 0)

    if "MemAvailable" in values:
        used_kb = total_kb - values["MemAvailable"]
    else:
        used_kb = total_kb - (
            values.get("MemFree", 0)
            + values.get("Buffers", 0)
            + values.get("Cached", 0)
        )

    return {
        "total_mb": round(total_kb / 1024.0, 2),
        "used_mb": round(max(used_kb, 0) / 1024.0, 2),
    }


def get_os_version():
    return _read_text("/proc/version", "unknown")


def get_process_list():
    processes = []

    try:
        entries = os.listdir("/proc")
    except OSError:
        return processes

    for entry in entries:
        if not entry.isdigit():
            continue

        pid = int(entry)
        name = _read_text(f"/proc/{entry}/comm")

        if not name:
            continue

        processes.append({
            "pid": pid,
            "name": name,
        })

    processes.sort(key=lambda process: process["pid"])
    return processes


def get_disks():
    disks = []

    try:
        devices = os.listdir("/sys/block")
    except OSError:
        return disks

    for device in sorted(devices):
        if device.startswith(("loop", "ram", "zram")):
            continue

        size_sectors = _read_text(f"/sys/block/{device}/size")

        try:
            sectors = int(size_sectors)
        except ValueError:
            continue

        size_mb = (sectors * 512) / (1024 * 1024)

        disks.append({
            "device": f"/dev/{device}",
            "size_mb": round(size_mb, 2),
        })

    return disks


def get_usb_devices():
    usb_devices = []
    base_path = "/sys/bus/usb/devices"

    try:
        devices = os.listdir(base_path)
    except OSError:
        return usb_devices

    for port in sorted(devices):
        if "-" not in port or ":" in port:
            continue

        device_path = os.path.join(base_path, port)

        manufacturer = _read_text(os.path.join(device_path, "manufacturer"))
        product = _read_text(os.path.join(device_path, "product"))

        description_parts = [
            value for value in (manufacturer, product) if value
        ]

        if not description_parts:
            continue

        usb_devices.append({
            "port": port,
            "description": " ".join(description_parts),
        })

    return usb_devices


def _get_local_ipv4_addresses():
    """
    Extrai endereços IPv4 locais do FIB do kernel em /proc/net/fib_trie.
    """
    addresses = []
    previous_ip = None

    for line in _read_text("/proc/net/fib_trie").splitlines():
        ip_match = re.search(r"\b(\d{1,3}(?:\.\d{1,3}){3})\b", line)

        if ip_match:
            previous_ip = ip_match.group(1)
            continue

        if previous_ip and "/32 host LOCAL" in line:
            if previous_ip not in addresses:
                addresses.append(previous_ip)
            previous_ip = None

    return addresses


def _get_ipv4_routes():
    routes = []
    lines = _read_text("/proc/net/route").splitlines()

    for line in lines[1:]:
        fields = line.split()

        if len(fields) < 8:
            continue

        interface = fields[0]
        destination = _hex_le_to_ipv4(fields[1])
        mask = _hex_le_to_ipv4(fields[7])

        if not destination or not mask:
            continue

        destination_int = _ipv4_to_int(destination)
        mask_int = _ipv4_to_int(mask)

        if destination_int is None or mask_int is None:
            continue

        routes.append({
            "interface": interface,
            "destination": destination_int,
            "mask": mask_int,
        })

    return routes


def get_network_adapters():
    adapters = []

    try:
        interfaces = sorted(os.listdir("/sys/class/net"))
    except OSError:
        return adapters

    local_addresses = _get_local_ipv4_addresses()
    routes = _get_ipv4_routes()

    for interface in interfaces:
        ip_address = ""

        if interface == "lo" and "127.0.0.1" in local_addresses:
            ip_address = "127.0.0.1"
        else:
            matching_routes = [
                route
                for route in routes
                if route["interface"] == interface and route["mask"] != 0
            ]

            matching_routes.sort(
                key=lambda route: bin(route["mask"]).count("1"),
                reverse=True,
            )

            for local_ip in local_addresses:
                if local_ip.startswith("127."):
                    continue

                local_ip_int = _ipv4_to_int(local_ip)
                if local_ip_int is None:
                    continue

                for route in matching_routes:
                    if (
                        local_ip_int & route["mask"]
                        == route["destination"] & route["mask"]
                    ):
                        ip_address = local_ip
                        break

                if ip_address:
                    break

        adapters.append({
            "interface": interface,
            "ip_address": ip_address,
        })

    return adapters

class StatusHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/status":
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
            return

        response = {
            "datetime": get_datetime(),
            "uptime_seconds": get_uptime(),
            "cpu": get_cpu_info(),
            "memory": get_memory_info(),
            "os_version": get_os_version(),
            "processes": get_process_list(),
            "disks": get_disks(),
            "usb_devices": get_usb_devices(),
            "network_adapters": get_network_adapters(),
        }

        data = json.dumps(response, indent=2).encode("utf-8")

        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, format, *args):
        print(
            "%s - - [%s] %s"
            % (
                self.client_address[0],
                self.log_date_time_string(),
                format % args,
            )
        )


def run_server(port=8080):
    print(f"Servidor disponível em http://0.0.0.0:{port}/status")
    server = HTTPServer(("0.0.0.0", port), StatusHandler)

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    run_server()
