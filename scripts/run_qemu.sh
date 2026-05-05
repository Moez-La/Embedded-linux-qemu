#!/bin/bash
qemu-system-arm \
  -machine versatilepb \
  -kernel scripts/kernel-arm \
  -dtb scripts/versatile-pb.dtb \
  -initrd scripts/initramfs.gz \
  -m 256 \
  -nographic \
  -append "root=/dev/ram0 rw console=ttyAMA0 init=/init" \
  -no-reboot
