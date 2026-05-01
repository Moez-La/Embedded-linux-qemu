# Embedded Linux sur QEMU ARM — Guide Complet

**Auteur : Moez Chagraoui**  
**OS hôte : Ubuntu 22.04 LTS**  
**Cible : ARM926EJ-S (versatilepb) émulé sous QEMU**

---

# Étape 1 : Boot du kernel

**Résultat : Linux 5.15 qui boote sur ARM avec un shell busybox fonctionnel**

## Prérequis système

\`\`\`bash
sudo apt update
sudo apt install -y \
  qemu-system-arm qemu-utils wget \
  gcc-arm-linux-gnueabi \
  make bc libssl-dev flex bison \
  debootstrap qemu-user-static \
  busybox-static
\`\`\`

## 1. Créer la structure du projet

\`\`\`bash
mkdir -p ~/embedded-linux-qemu/{scripts,buildroot,kernel_module,app}
cd ~/embedded-linux-qemu
\`\`\`

## 2. Compiler le kernel Linux 5.15 pour ARM

\`\`\`bash
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.tar.xz -O /tmp/linux-5.15.tar.xz
cd /tmp && tar xf linux-5.15.tar.xz && cd linux-5.15
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- versatile_defconfig
\`\`\`

Dans menuconfig activer SCSI :
\`\`\`bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
\`\`\`
- **Device Drivers** → **SCSI device support** → **Y**
- **SCSI disk support** → **Y**
- **Save** puis **Exit**

Compiler :
\`\`\`bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j$(nproc) zImage
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- dtbs
cp arch/arm/boot/zImage ~/embedded-linux-qemu/scripts/kernel-arm
cp arch/arm/boot/dts/versatile-pb.dtb ~/embedded-linux-qemu/scripts/
\`\`\`

## 3. Créer l'initramfs minimal

\`\`\`bash
mkdir -p /tmp/initramfs/{bin,sbin,etc,proc,sys,dev}
sudo wget https://busybox.net/downloads/binaries/1.31.0-defconfig-multiarch-musl/busybox-armv5l \
  -O /tmp/initramfs/bin/busybox
sudo chmod +x /tmp/initramfs/bin/busybox
cd /tmp/initramfs/bin && sudo ln -s busybox sh && sudo ln -s busybox ls && sudo ln -s busybox mount
\`\`\`

Créer le script init :
\`\`\`bash
cat > /tmp/initramfs/init << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
echo "==================================="
echo " Embedded Linux ARM - QEMU Running!"
echo "==================================="
exec /bin/sh
EOF
chmod +x /tmp/initramfs/init
\`\`\`

Générer l'archive :
\`\`\`bash
cd /tmp/initramfs
find . | cpio -H newc -o | gzip > /tmp/initramfs.gz
\`\`\`

## 4. Script de boot QEMU

\`\`\`bash
cat > ~/embedded-linux-qemu/scripts/run_qemu.sh << 'EOF'
#!/bin/bash
qemu-system-arm \
  -machine versatilepb \
  -kernel scripts/kernel-arm \
  -dtb scripts/versatile-pb.dtb \
  -initrd /tmp/initramfs.gz \
  -m 256 \
  -nographic \
  -append "root=/dev/ram0 rw console=ttyAMA0 init=/init" \
  -no-reboot
EOF
chmod +x ~/embedded-linux-qemu/scripts/run_qemu.sh
\`\`\`

## 5. Lancer et vérifier

\`\`\`bash
cd ~/embedded-linux-qemu
./scripts/run_qemu.sh
\`\`\`

Une fois le shell ouvert :
\`\`\`bash
/bin/busybox --install -s /bin
uname -a
# Linux (none) 5.15.0 armv5tejl GNU/Linux
cat /proc/cpuinfo
# ARM926EJ-S rev 5 (v5l)
\`\`\`

Pour quitter QEMU (depuis un autre terminal) :
\`\`\`bash
kill $(pgrep qemu-system-arm)
\`\`\`

## Résumé étape 1

| Élément | Détail |
|---|---|
| Kernel | Linux 5.15.0 compilé from scratch |
| Architecture | ARM926EJ-S (ARMv5TEJ) |
| Machine QEMU | versatilepb |
| Userspace | busybox-armv5l |
| Boot | initramfs en RAM |

---

# Étape 2 : Buildroot — (à venir)

