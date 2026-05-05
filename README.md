# Embedded Linux QEMU ARM

Minimal embedded Linux system running on QEMU ARM emulator.

## Project Overview

This project builds a complete embedded Linux system from scratch for ARM architecture, running on QEMU without any physical hardware.

## Stack

- **Kernel** : Linux 5.15 (compiled from source) + Linux 6.1.44 (Buildroot)
- **Architecture** : ARM926EJ-S (ARMv5TEJ) — versatilepb
- **Userspace** : Busybox / Buildroot rootfs
- **Build system** : Buildroot 2023.11
- **Emulator** : QEMU 6.2
- **Driver** : Custom character device kernel module

## Project Structure

```
embedded-linux-qemu/
├── scripts/         → QEMU boot scripts
├── buildroot/       → Buildroot configuration
├── kernel_module/   → Custom kernel driver (chardev)
├── app/             → Userspace C application (step 4)
└── docs/            → Step by step guide
```

## Steps

- [x] **Step 1** : QEMU ARM boot — Linux 5.15 + busybox initramfs
- [x] **Step 2** : Buildroot — complete rootfs with network
- [x] **Step 3** : Kernel module — character device driver with mutex
- [ ] **Step 4** : Userspace app — communication with kernel module

## Quick Start

### Boot minimal system (Step 1)
```bash
./scripts/run_qemu.sh
```

### Boot Buildroot system (Step 2+)
```bash
./scripts/run_buildroot.sh
```

### Build kernel module (Step 3)
```bash
cd kernel_module && make
```

## Author

Moez Chagraoui — Embedded Systems Engineer
