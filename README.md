# Embedded Linux QEMU ARM

[![Build](https://github.com/Moez-La/Embedded-linux-qemu/actions/workflows/build.yml/badge.svg)](https://github.com/Moez-La/Embedded-linux-qemu/actions/workflows/build.yml)

Minimal embedded Linux system built from scratch for ARM architecture, running on QEMU without any physical hardware. This project covers the full embedded Linux stack: kernel compilation, rootfs generation with Buildroot, custom kernel driver, and userspace applications with real-time latency benchmarking.

---

## Project Overview

This project demonstrates the complete embedded Linux development workflow:

- Cross-compiling a Linux kernel for ARM from source
- Building a minimal root filesystem with Buildroot
- Writing a custom Linux kernel module (character device driver)
- Developing userspace applications that communicate with the kernel driver
- Measuring and analyzing kernel ↔ userspace communication latency

All of this runs on an emulated ARM926EJ-S processor using QEMU — no physical hardware required.

---

## Stack

| Component | Details |
|---|---|
| **Emulator** | QEMU 6.2 — machine versatilepb |
| **Architecture** | ARM926EJ-S (ARMv5TEJ) |
| **Kernel (Step 1)** | Linux 5.15 — compiled from source |
| **Kernel (Step 2+)** | Linux 6.1.44 — compiled by Buildroot |
| **Build system** | Buildroot 2023.11 |
| **Userspace (Step 1)** | Busybox (armv5l) |
| **Userspace (Step 2+)** | Buildroot rootfs (ext2, 55MB) |
| **Driver** | Custom character device — `chardev.ko` |
| **Compiler** | arm-buildroot-linux-gnueabi-gcc 12.3.0 |
| **Host OS** | Ubuntu 22.04 LTS |

---

## Project Structure

```
embedded-linux-qemu/
├── scripts/
│   ├── run_qemu.sh          → Boot minimal system (Step 1)
│   ├── run_buildroot.sh     → Boot Buildroot system (Step 2+)
│   ├── kernel-arm           → Linux 5.15 zImage (not in git)
│   ├── versatile-pb.dtb     → Device Tree Blob (not in git)
│   └── initramfs.gz         → Busybox initramfs (not in git)
├── buildroot/
│   ├── rootfs.ext2          → Buildroot rootfs image (not in git)
│   ├── zImage               → Buildroot kernel (not in git)
│   ├── versatile-pb.dtb     → Buildroot DTB (not in git)
│   └── start-qemu.sh        → Buildroot boot script
├── kernel_module/
│   ├── chardev.c            → Character device driver source
│   └── Makefile             → Cross-compilation Makefile
├── app/
│   ├── basic_app.c          → Simple read/write app
│   ├── benchmark.c          → Multi-thread producer/consumer benchmark
│   └── Makefile             → Cross-compilation Makefile
└── docs/
    └── guide.md             → Complete step-by-step guide
```

---

## Steps

- [x] **Step 1** — QEMU ARM boot : Linux 5.15 kernel compiled from source + busybox initramfs
- [x] **Step 2** — Buildroot : complete rootfs with network (DHCP), login, systemd
- [x] **Step 3** — Kernel module : custom character device driver with mutex
- [x] **Step 4** — Userspace apps : basic read/write + multi-thread benchmark with sensor simulation

---

## Quick Start

### Step 1 — Boot minimal system
```bash
./scripts/run_qemu.sh
```
Then inside QEMU:
```bash
/bin/busybox --install -s /bin
uname -a
# Linux (none) 5.15.0 armv5tejl GNU/Linux
```

### Step 2 — Boot Buildroot system
```bash
./scripts/run_buildroot.sh
# Login: root (no password)
```

### Step 3 — Build and load kernel module
```bash
# Build
cd kernel_module && make

# Load in QEMU (after mounting into rootfs)
insmod /root/chardev.ko
mknod /dev/mon_device c 251 0
```

### Step 4 — Build and run userspace apps
```bash
# Build both apps
cd app && make

# Run basic app in QEMU
/root/basic_app

# Run benchmark in QEMU
/root/benchmark
```

---

## Kernel Module — chardev

The kernel module implements a Linux character device driver with the following features:

- `open` / `release` — device lifecycle management
- `read` / `write` — data transfer between kernel and userspace
- Internal buffer of 1024 bytes
- Mutex protection for thread-safe concurrent access
- Kernel logging via `pr_info` / `dmesg`

```
# Load the module
insmod /root/chardev.ko
# mon_device: registered with major number 251

# Create the device node
mknod /dev/mon_device c 251 0

# Test
echo "hello kernel" > /dev/mon_device
cat /dev/mon_device
# hello kernel
```

---

## Results

### Basic App — Read/Write on /dev/mon_device

Simple read/write cycles between userspace and kernel driver with latency measurement using `clock_gettime(CLOCK_MONOTONIC)`.

```
=== Embedded Linux ARM - Basic Read/Write App ===

[1] WRITE : "Message 1 depuis userspace"
     Latence write : 829000 ns
     READ  : "Message 1 depuis userspace"
     Latence read  : 903000 ns

[2] WRITE : "Message 2 depuis userspace"
     Latence write : 319000 ns
     READ  : "Message 2 depuis userspace"
     Latence read  : 277000 ns

[3] WRITE : "Message 3 depuis userspace"
     Latence write : 287000 ns
     READ  : "Message 3 depuis userspace"
     Latence read  : 256000 ns

[4] WRITE : "Message 4 depuis userspace"
     Latence write : 259000 ns
     READ  : "Message 4 depuis userspace"
     Latence read  : 241000 ns

[5] WRITE : "Message 5 depuis userspace"
     Latence write : 305000 ns
     READ  : "Message 5 depuis userspace"
     Latence read  : 310000 ns

=== Application terminee ===
```

### Benchmark App — Producer/Consumer Multi-thread

Two POSIX threads communicating via `/dev/mon_device` with simulated sensor data (temperature, pressure, flow rate). 100 iterations with latency threshold at 1000 µs. Thread synchronization via `pthread_mutex` and `pthread_cond`.

```
=================================================
   Embedded Linux ARM - Kernel Driver Benchmark
   Device : /dev/mon_device
   Iterations : 100
   Latency threshold : 1000 us
=================================================

  READ : [001] SENSOR_TEMP=56.6 SENSOR_PRESS=5.0 SENSOR_FLOW=8.6
  READ : [002] SENSOR_TEMP=64.1 SENSOR_PRESS=2.2 SENSOR_FLOW=6.3
  READ : [003] SENSOR_TEMP=62.1 SENSOR_PRESS=5.0 SENSOR_FLOW=4.0
  ...
  READ : [100] SENSOR_TEMP=59.2 SENSOR_PRESS=1.9 SENSOR_FLOW=9.5

=== Rapport Final ===
  WRITE      | min:  389 us | max: 1001 us | avg:  459 us | violations: 1/100
  READ       | min:  346 us | max:  845 us | avg:  422 us | violations: 0/100

  Total iterations : 100
  Threshold        : 1000 us
=== Benchmark termine ===
```

**Key observations:**
- Average write latency : **459 µs** on ARM926EJ-S emulated
- Average read latency : **422 µs**
- Only **1 timing violation** out of 100 write operations
- **Zero violations** on read — mutex protection working correctly
- Producer/consumer synchronization via `pthread_cond` — zero data races

---

## Documentation

Full step-by-step guide available in [`docs/guide.md`](docs/guide.md) — covers every command from scratch including kernel compilation, Buildroot setup, driver development and testing.

---

## Author

**Moez Chagraoui** — Embedded Systems Engineer  
Double degree : INP-ENSEEIHT Toulouse (ACISE) + ENIT Tunis  
[LinkedIn](https://linkedin.com/in/moez-chagraoui) • [GitHub](https://github.com/Moez-La)
