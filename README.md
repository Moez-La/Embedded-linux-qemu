# Embedded Linux QEMU ARM

Minimal embedded Linux system running on QEMU ARM emulator.

## Project Overview

This project builds a complete embedded Linux system from scratch for ARM architecture, running on QEMU without any physical hardware.

## Stack

- **Kernel** : Linux 5.15 (compiled from source)
- **Architecture** : ARM926EJ-S (ARMv5TEJ) — versatilepb
- **Userspace** : Busybox
- **Build system** : Buildroot (step 2)
- **Emulator** : QEMU 6.2

## Project Structure

\`\`\`
embedded-linux-qemu/
├── scripts/         → QEMU boot scripts
├── buildroot/       → Buildroot configuration (step 2)
├── kernel_module/   → Custom kernel driver (step 3)
├── app/             → Userspace C application (step 4)
└── docs/            → Step by step guide
\`\`\`

## Steps

- [x] **Step 1** : QEMU ARM boot — Linux 5.15 + busybox initramfs
- [ ] **Step 2** : Buildroot — minimal rootfs
- [ ] **Step 3** : Kernel module — custom character device driver
- [ ] **Step 4** : Userspace app — communication with kernel module

## Quick Start

\`\`\`bash
./scripts/run_qemu.sh
\`\`\`

## Author

Moez Chagraoui — Embedded Systems Engineer
