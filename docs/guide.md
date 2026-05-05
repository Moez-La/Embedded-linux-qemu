# Embedded Linux sur QEMU ARM — Guide Complet

**Auteur : Moez Chagraoui**  
**OS hôte : Ubuntu 22.04 LTS**  
**Cible : ARM926EJ-S (versatilepb) émulé sous QEMU**

---

# Étape 1 : Boot du kernel

**Résultat : Linux 5.15 qui boote sur ARM avec un shell busybox fonctionnel**

## Prérequis système

```bash
sudo apt update
sudo apt install -y \
  qemu-system-arm \
  qemu-utils \
  wget \
  gcc-arm-linux-gnueabi \
  make bc libssl-dev flex bison \
  debootstrap qemu-user-static \
  busybox-static
```

---

## 1. Créer la structure du projet

```bash
mkdir -p ~/embedded-linux-qemu/{scripts,buildroot,kernel_module,app}
cd ~/embedded-linux-qemu
```

Structure créée :
```
embedded-linux-qemu/
├── scripts/        → scripts QEMU et images
├── buildroot/      → configuration Buildroot (étape 2)
├── kernel_module/  → driver kernel custom (étape 3)
└── app/            → application C userspace (étape 4)
```

---

## 2. Compiler le kernel Linux 5.15 pour ARM

### Télécharger les sources
```bash
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.15.tar.xz -O /tmp/linux-5.15.tar.xz
cd /tmp
tar xf linux-5.15.tar.xz
cd linux-5.15
```

### Configurer pour la machine ARM Versatile PB
```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- versatile_defconfig
```

### Activer le support SCSI dans le kernel
```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- menuconfig
```

Dans le menu :
- Aller dans **Device Drivers** → **SCSI device support**
- Sélectionner **SCSI device support** → appuyer sur **Y**
- Sélectionner **SCSI disk support** → appuyer sur **Y**
- **Save** puis **Exit**

### Compiler le kernel et le Device Tree
```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- -j$(nproc) zImage
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- dtbs
```

> ⏳ La compilation du kernel prend 10-20 minutes selon la machine.

### Copier les fichiers dans le projet
```bash
cp /tmp/linux-5.15/arch/arm/boot/zImage ~/embedded-linux-qemu/scripts/kernel-arm
cp /tmp/linux-5.15/arch/arm/boot/dts/versatile-pb.dtb ~/embedded-linux-qemu/scripts/
```

---

## 3. Créer l'initramfs minimal (busybox ARM)

### Télécharger busybox pour ARM
```bash
mkdir -p /tmp/initramfs/{bin,sbin,etc,proc,sys,dev}
sudo wget https://busybox.net/downloads/binaries/1.31.0-defconfig-multiarch-musl/busybox-armv5l \
  -O /tmp/initramfs/bin/busybox
sudo chmod +x /tmp/initramfs/bin/busybox
```

### Créer les liens symboliques busybox
```bash
cd /tmp/initramfs/bin
sudo ln -s busybox sh
sudo ln -s busybox ls
sudo ln -s busybox mount
```

### Créer le script init
```bash
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
```

### Générer l'archive initramfs et la sauvegarder
```bash
cd /tmp/initramfs
find . | cpio -H newc -o | gzip > /tmp/initramfs.gz
cp /tmp/initramfs.gz ~/embedded-linux-qemu/scripts/
```

---

## 4. Créer le script de boot QEMU

```bash
cat > ~/embedded-linux-qemu/scripts/run_qemu.sh << 'EOF'
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
EOF
chmod +x ~/embedded-linux-qemu/scripts/run_qemu.sh
```

**Explication des paramètres QEMU :**
- `-machine versatilepb` → émule une carte ARM Versatile PB
- `-kernel` → kernel Linux compilé pour ARM
- `-dtb` → Device Tree Blob décrivant le matériel
- `-initrd` → système de fichiers initial en RAM
- `-m 256` → 256 Mo de RAM
- `-nographic` → sortie dans le terminal (pas de fenêtre graphique)
- `-append` → paramètres passés au kernel au boot
- `-no-reboot` → QEMU s'arrête au lieu de redémarrer

---

## 5. Lancer QEMU

```bash
cd ~/embedded-linux-qemu
./scripts/run_qemu.sh
```

> ✅ À partir de maintenant, à chaque redémarrage tu lances juste cette commande — tout est sauvegardé dans `scripts/`.

### Résultat attendu

Après le boot, tu dois voir :
```
===================================
 Embedded Linux ARM - QEMU Running!
===================================
/ #
```

### Installer toutes les commandes busybox
```bash
/bin/busybox --install -s /bin
```

### Vérifier le système
```bash
uname -a
# Résultat : Linux (none) 5.15.0 armv5tejl GNU/Linux

cat /proc/cpuinfo
# Résultat : ARM926EJ-S rev 5 (v5l)
```

### Quitter QEMU
Depuis un autre terminal :
```bash
kill $(pgrep qemu-system-arm)
```

---

## 6. Configurer GitHub

### Créer le .gitignore
```bash
cat > ~/embedded-linux-qemu/.gitignore << 'EOF'
scripts/rootfs.img
scripts/kernel-arm
scripts/versatile-pb.dtb
scripts/initramfs.gz
buildroot/rootfs.ext2
buildroot/zImage
buildroot/versatile-pb.dtb
EOF
```

### Premier commit
```bash
cd ~/embedded-linux-qemu
git init
git remote add origin git@github.com:TON_USERNAME/Embedded-linux-qemu.git
git config pull.rebase false
git pull origin main --allow-unrelated-histories
git add .
git commit -m "feat(step1): QEMU ARM boot - Linux 5.15 kernel + busybox initramfs"
git push -u origin main
```

---

## Résumé de l'étape 1

| Élément | Détail |
|---|---|
| Kernel | Linux 5.15.0 compilé from scratch |
| Architecture | ARM926EJ-S (ARMv5TEJ) |
| Machine QEMU | versatilepb |
| Userspace | busybox-armv5l |
| Boot | initramfs en RAM |
| Compilateur | arm-linux-gnueabi-gcc 11.4.0 |

---

# Étape 2 : Buildroot

**Résultat : Système Linux complet avec rootfs ext2, réseau DHCP et login**

## Qu'est-ce que Buildroot ?

Buildroot est un outil qui génère automatiquement un système Linux embarqué complet — kernel, rootfs, bibliothèques, applications — avec une simple configuration. C'est la référence en industrie pour construire des images embarquées propres et reproductibles.

---

## 1. Télécharger et extraire Buildroot

```bash
wget https://buildroot.org/downloads/buildroot-2023.11.tar.gz -O /tmp/buildroot.tar.gz
cd /tmp
tar xf buildroot.tar.gz
cd buildroot-2023.11
```

---

## 2. Installer les dépendances

```bash
sudo apt install -y cpio rsync bc
```

---

## 3. Configurer Buildroot pour QEMU ARM Versatile

```bash
PATH=$(echo $PATH | tr ':' '\n' | grep -v '^\.$' | tr '\n' ':' | sed 's/:$//') \
make qemu_arm_versatile_defconfig
```

> La commande `PATH=...` corrige une erreur liée au répertoire courant dans le PATH.

Cette commande charge la configuration officielle Buildroot pour exactement notre machine QEMU ARM Versatile PB.

---

## 4. Compiler Buildroot

```bash
PATH=$(echo $PATH | tr ':' '\n' | grep -v '^\.$' | tr '\n' ':' | sed 's/:$//') \
make -j$(nproc)
```

> ⏳ Cette commande prend **30-60 minutes** — Buildroot télécharge et compile toute la toolchain, le kernel 6.1 et le rootfs.

### Vérifier les images générées
```bash
ls /tmp/buildroot-2023.11/output/images/
```

Résultat attendu :
```
rootfs.ext2  start-qemu.sh  versatile-pb.dtb  zImage
```

---

## 5. Lancer le système Buildroot

```bash
cd /tmp/buildroot-2023.11/output/images/
./start-qemu.sh --serial-only
```

### Connexion
```
buildroot login: root
```
Pas de mot de passe — appuie sur **Entrée**.

### Résultat attendu
```
Welcome to Buildroot
buildroot login: root
#
```

### Vérifier le système
```bash
uname -a
# Linux buildroot 6.1.44 armv5tejl GNU/Linux

df -h
# /dev/root  55.1M  3.8M  48.2M  7%  /

cat /proc/cpuinfo
# ARM926EJ-S rev 5 (v5l)
```

---

## 6. Sauvegarder dans le projet

```bash
cp /tmp/buildroot-2023.11/output/images/rootfs.ext2 ~/embedded-linux-qemu/buildroot/
cp /tmp/buildroot-2023.11/output/images/zImage ~/embedded-linux-qemu/buildroot/
cp /tmp/buildroot-2023.11/output/images/versatile-pb.dtb ~/embedded-linux-qemu/buildroot/
cp /tmp/buildroot-2023.11/output/images/start-qemu.sh ~/embedded-linux-qemu/buildroot/
```

### Créer le script de boot Buildroot
```bash
cat > ~/embedded-linux-qemu/scripts/run_buildroot.sh << 'EOF'
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
EOF
chmod +x ~/embedded-linux-qemu/scripts/run_buildroot.sh
```

### Commit sur GitHub
```bash
cd ~/embedded-linux-qemu
git add .
git commit -m "feat(step2): Buildroot - Linux 6.1.44 ARM rootfs

- Configured Buildroot for qemu_arm_versatile
- Generated complete rootfs.ext2 (55MB)
- Network working with DHCP
- Login: root (no password)"
git push
```

---

## Résumé de l'étape 2

| Élément | Détail |
|---|---|
| Kernel | Linux 6.1.44 (compilé par Buildroot) |
| Architecture | ARM926EJ-S (ARMv5TEJ) |
| Rootfs | ext2 — 55MB complet |
| Réseau | DHCP — IP 10.0.2.15 |
| Login | root (pas de mot de passe) |
| Build system | Buildroot 2023.11 |

---

# Étape 3 : Driver kernel — (à venir)
