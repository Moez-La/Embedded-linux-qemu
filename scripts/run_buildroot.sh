#!/bin/bash
cd "$(dirname "$0")/../buildroot"
qemu-system-arm \
  -M versatilepb \
  -kernel zImage \
  -dtb versatile-pb.dtb \
  -drive file=rootfs.ext2,if=scsi,format=raw \
  -append "rootwait root=/dev/sda console=ttyAMA0,115200" \
  -net nic,model=rtl8139 -net user \
  -nographic
