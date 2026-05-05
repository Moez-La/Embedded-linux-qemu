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

Connexion : `buildroot login: root` — pas de mot de passe, appuie sur **Entrée**.

### Vérifier le système
```bash
uname -a
# Linux buildroot 6.1.44 armv5tejl GNU/Linux

df -h
# /dev/root  55.1M  3.8M  48.2M  7%  /
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

# Étape 3 : Driver kernel

**Résultat : Module kernel character device avec buffer et mutex, testé sur QEMU ARM**

## Qu'est-ce qu'un character device driver ?

Un character device driver est un module kernel qui expose un fichier `/dev/xxx` permettant à une application userspace de communiquer directement avec le kernel via `open`, `read`, `write`. C'est l'architecture de base de tous les drivers Linux embarqués (capteurs, périphériques série, GPIO, etc.).

---

## 1. Créer le fichier source du driver

```bash
cd ~/embedded-linux-qemu/kernel_module
```

```bash
cat > chardev.c << 'EOF'
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "mon_device"
#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Moez Chagraoui");
MODULE_DESCRIPTION("Character device driver with mutex");

static int major;
static char buffer[BUFFER_SIZE];
static int buffer_len = 0;
static DEFINE_MUTEX(dev_mutex);

static int dev_open(struct inode *inode, struct file *file)
{
    pr_info("%s: device opened\n", DEVICE_NAME);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    pr_info("%s: device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *offset)
{
    int bytes_read;
    mutex_lock(&dev_mutex);
    if (*offset >= buffer_len) {
        mutex_unlock(&dev_mutex);
        return 0;
    }
    bytes_read = min((size_t)(buffer_len - *offset), count);
    if (copy_to_user(user_buf, buffer + *offset, bytes_read)) {
        mutex_unlock(&dev_mutex);
        return -EFAULT;
    }
    *offset += bytes_read;
    pr_info("%s: %d bytes read\n", DEVICE_NAME, bytes_read);
    mutex_unlock(&dev_mutex);
    return bytes_read;
}

static ssize_t dev_write(struct file *file, const char __user *user_buf,
                         size_t count, loff_t *offset)
{
    mutex_lock(&dev_mutex);
    if (count > BUFFER_SIZE) {
        mutex_unlock(&dev_mutex);
        return -EINVAL;
    }
    if (copy_from_user(buffer, user_buf, count)) {
        mutex_unlock(&dev_mutex);
        return -EFAULT;
    }
    buffer_len = count;
    pr_info("%s: %zu bytes written\n", DEVICE_NAME, count);
    mutex_unlock(&dev_mutex);
    return count;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

static int __init chardev_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        pr_err("%s: failed to register device\n", DEVICE_NAME);
        return major;
    }
    pr_info("%s: registered with major number %d\n", DEVICE_NAME, major);
    pr_info("Run: mknod /dev/%s c %d 0\n", DEVICE_NAME, major);
    return 0;
}

static void __exit chardev_exit(void)
{
    unregister_chrdev(major, DEVICE_NAME);
    pr_info("%s: unregistered\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);
EOF
```

---

## 2. Créer le Makefile

```bash
cat > Makefile << 'EOF'
KDIR := /tmp/buildroot-2023.11/output/build/linux-6.1.44
CROSS_COMPILE := /tmp/buildroot-2023.11/output/host/bin/arm-buildroot-linux-gnueabi-
ARCH := arm

obj-m += chardev.o

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
EOF
```

> ⚠️ Utiliser le compilateur Buildroot (`arm-buildroot-linux-gnueabi-`) et non celui d'Ubuntu (`arm-linux-gnueabi-`) — sinon erreur de flags incompatibles.

---

## 3. Compiler le module

```bash
make
```

Résultat attendu :
```
CC [M]  chardev.o
MODPOST Module.symvers
LD [M]  chardev.ko
```

Vérifier :
```bash
file chardev.ko
# chardev.ko: ELF 32-bit LSB relocatable, ARM
```

---

## 4. Intégrer le module dans le rootfs Buildroot

```bash
sudo mkdir -p /mnt/buildroot
sudo mount -o loop ~/embedded-linux-qemu/buildroot/rootfs.ext2 /mnt/buildroot
sudo cp ~/embedded-linux-qemu/kernel_module/chardev.ko /mnt/buildroot/root/
sudo umount /mnt/buildroot
```

---

## 5. Tester dans QEMU

### Lancer Buildroot
```bash
cd ~/embedded-linux-qemu
./scripts/run_buildroot.sh
```

Se connecter avec `root` (pas de mot de passe).

### Charger le module
```bash
insmod /root/chardev.ko
```

Résultat dans dmesg :
```
mon_device: registered with major number 251
Run: mknod /dev/mon_device c 251 0
```

### Créer le device et tester
```bash
mknod /dev/mon_device c 251 0
echo "Bonjour depuis userspace!" > /dev/mon_device
cat /dev/mon_device
```

Résultat attendu :
```
mon_device: device opened
mon_device: 26 bytes written
mon_device: device closed
mon_device: device opened
mon_device: 26 bytes read
Bonjour depuis userspace!
mon_device: device closed
```

### Vérifier les logs kernel
```bash
dmesg | grep mon_device
```

---

## Résumé de l'étape 3

| Élément | Détail |
|---|---|
| Type de driver | Character device |
| Buffer | 1024 bytes |
| Synchronisation | Mutex |
| Compilateur | arm-buildroot-linux-gnueabi-gcc 12.3.0 |
| Kernel cible | Linux 6.1.44 (Buildroot) |
| Test | echo + cat via /dev/mon_device |

---


# Étape 4 : Applications C userspace

**Résultat : Deux applications C cross-compilées pour ARM communiquant avec le driver kernel**

Deux applications distinctes :
- **basic_app** : lecture/écriture simple sur `/dev/mon_device` avec mesure de latence
- **benchmark** : application multi-thread producteur/consommateur avec simulation de capteurs et analyse statistique

---

## 1. Créer basic_app.c

```bash
cd ~/embedded-linux-qemu/app
```

```bash
cat > basic_app.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define DEVICE "/dev/mon_device"
#define BUFFER_SIZE 1024
#define NB_ITERATIONS 5

long get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

int main(void)
{
    int fd;
    char write_buf[BUFFER_SIZE];
    char read_buf[BUFFER_SIZE];
    long t_start, t_end, latency;
    int i;

    printf("=== Embedded Linux ARM - Basic Read/Write App ===\n\n");

    for (i = 0; i < NB_ITERATIONS; i++) {
        snprintf(write_buf, BUFFER_SIZE, "Message %d depuis userspace", i + 1);

        fd = open(DEVICE, O_RDWR);
        if (fd < 0) { perror("open"); return EXIT_FAILURE; }

        t_start = get_time_ns();
        if (write(fd, write_buf, strlen(write_buf)) < 0) {
            perror("write"); close(fd); return EXIT_FAILURE;
        }
        t_end = get_time_ns();
        latency = t_end - t_start;
        printf("[%d] WRITE : \"%s\"\n", i + 1, write_buf);
        printf("     Latence write : %ld ns\n", latency);
        close(fd);

        fd = open(DEVICE, O_RDWR);
        if (fd < 0) { perror("open"); return EXIT_FAILURE; }

        memset(read_buf, 0, BUFFER_SIZE);
        t_start = get_time_ns();
        if (read(fd, read_buf, BUFFER_SIZE) < 0) {
            perror("read"); close(fd); return EXIT_FAILURE;
        }
        t_end = get_time_ns();
        latency = t_end - t_start;
        printf("     READ  : \"%s\"\n", read_buf);
        printf("     Latence read  : %ld ns\n\n", latency);
        close(fd);

        sleep(1);
    }

    printf("=== Application terminee ===\n");
    return EXIT_SUCCESS;
}
EOF
```

---

## 2. Créer benchmark.c

```bash
cat > benchmark.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define DEVICE          "/dev/mon_device"
#define BUFFER_SIZE     1024
#define NB_ITERATIONS   100
#define LATENCY_THRESHOLD_US 1000

typedef struct {
    long min_ns;
    long max_ns;
    long total_ns;
    int  count;
    int  violations;
} Stats;

typedef struct {
    char     message[BUFFER_SIZE];
    int      ready;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_produced;
    pthread_cond_t  cond_consumed;
    Stats    write_stats;
    Stats    read_stats;
    int      done;
} SharedData;

static long get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

static void update_stats(Stats *s, long latency_ns)
{
    if (s->count == 0 || latency_ns < s->min_ns) s->min_ns = latency_ns;
    if (latency_ns > s->max_ns) s->max_ns = latency_ns;
    s->total_ns += latency_ns;
    s->count++;
    if (latency_ns > LATENCY_THRESHOLD_US * 1000)
        s->violations++;
}

static void print_stats(const char *label, const Stats *s)
{
    long avg = s->count ? s->total_ns / s->count : 0;
    printf("  %-10s | min: %6ld us | max: %6ld us | avg: %6ld us | violations: %d/%d\n",
           label, s->min_ns/1000, s->max_ns/1000, avg/1000,
           s->violations, s->count);
}

static void *producer(void *arg)
{
    SharedData *data = (SharedData *)arg;
    int fd;
    long t_start, t_end;
    int i;

    for (i = 0; i < NB_ITERATIONS; i++) {
        pthread_mutex_lock(&data->mutex);
        while (data->ready)
            pthread_cond_wait(&data->cond_consumed, &data->mutex);

        snprintf(data->message, BUFFER_SIZE,
                 "[%03d] SENSOR_TEMP=%.1f SENSOR_PRESS=%.1f SENSOR_FLOW=%.1f",
                 i + 1,
                 20.0 + (rand() % 600) / 10.0,
                 1.0  + (rand() % 50)  / 10.0,
                 0.5  + (rand() % 100) / 10.0);
        pthread_mutex_unlock(&data->mutex);

        fd = open(DEVICE, O_WRONLY);
        if (fd < 0) { perror("producer open"); break; }

        t_start = get_time_ns();
        write(fd, data->message, strlen(data->message));
        t_end = get_time_ns();
        close(fd);

        update_stats(&data->write_stats, t_end - t_start);

        pthread_mutex_lock(&data->mutex);
        data->ready = 1;
        pthread_cond_signal(&data->cond_produced);
        pthread_mutex_unlock(&data->mutex);

        usleep(10000);
    }

    pthread_mutex_lock(&data->mutex);
    data->done = 1;
    pthread_cond_signal(&data->cond_produced);
    pthread_mutex_unlock(&data->mutex);
    return NULL;
}

static void *consumer(void *arg)
{
    SharedData *data = (SharedData *)arg;
    int fd;
    char buf[BUFFER_SIZE];
    long t_start, t_end;

    while (1) {
        pthread_mutex_lock(&data->mutex);
        while (!data->ready && !data->done)
            pthread_cond_wait(&data->cond_produced, &data->mutex);

        if (!data->ready && data->done) {
            pthread_mutex_unlock(&data->mutex);
            break;
        }
        pthread_mutex_unlock(&data->mutex);

        fd = open(DEVICE, O_RDONLY);
        if (fd < 0) { perror("consumer open"); break; }

        memset(buf, 0, BUFFER_SIZE);
        t_start = get_time_ns();
        read(fd, buf, BUFFER_SIZE);
        t_end = get_time_ns();
        close(fd);

        update_stats(&data->read_stats, t_end - t_start);
        printf("  READ : %s\n", buf);

        pthread_mutex_lock(&data->mutex);
        data->ready = 0;
        pthread_cond_signal(&data->cond_consumed);
        pthread_mutex_unlock(&data->mutex);
    }
    return NULL;
}

int main(void)
{
    SharedData data;
    pthread_t prod_thread, cons_thread;

    memset(&data, 0, sizeof(data));
    pthread_mutex_init(&data.mutex, NULL);
    pthread_cond_init(&data.cond_produced, NULL);
    pthread_cond_init(&data.cond_consumed, NULL);
    srand(42);

    printf("\n");
    printf("=================================================\n");
    printf("   Embedded Linux ARM - Kernel Driver Benchmark  \n");
    printf("   Device : %s\n", DEVICE);
    printf("   Iterations : %d\n", NB_ITERATIONS);
    printf("   Latency threshold : %d us\n", LATENCY_THRESHOLD_US);
    printf("=================================================\n\n");

    pthread_create(&prod_thread, NULL, producer, &data);
    pthread_create(&cons_thread, NULL, consumer, &data);

    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);

    printf("\n=== Rapport Final ===\n");
    print_stats("WRITE", &data.write_stats);
    print_stats("READ",  &data.read_stats);
    printf("\n  Total iterations : %d\n", NB_ITERATIONS);
    printf("  Threshold        : %d us\n", LATENCY_THRESHOLD_US);
    printf("\n=== Benchmark termine ===\n\n");

    pthread_mutex_destroy(&data.mutex);
    pthread_cond_destroy(&data.cond_produced);
    pthread_cond_destroy(&data.cond_consumed);
    return EXIT_SUCCESS;
}
EOF
```

---

## 3. Créer le Makefile

```bash
cat > Makefile << 'EOF'
CC := /tmp/buildroot-2023.11/output/host/bin/arm-buildroot-linux-gnueabi-gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread

all: basic_app benchmark

basic_app: basic_app.c
	$(CC) $(CFLAGS) -o basic_app basic_app.c

benchmark: benchmark.c
	$(CC) $(CFLAGS) -o benchmark benchmark.c $(LDFLAGS)

clean:
	rm -f basic_app benchmark
EOF
make
```

> ⚠️ Utiliser le compilateur Buildroot (`arm-buildroot-linux-gnueabi-`) — sinon erreur de flags incompatibles.

---

## 4. Intégrer dans le rootfs et tester

```bash
sudo mount -o loop ~/embedded-linux-qemu/buildroot/rootfs.ext2 /mnt/buildroot
sudo cp ~/embedded-linux-qemu/app/basic_app /mnt/buildroot/root/
sudo cp ~/embedded-linux-qemu/app/benchmark /mnt/buildroot/root/
sudo umount /mnt/buildroot
cd ~/embedded-linux-qemu
./scripts/run_buildroot.sh
```

Se connecter avec `root`, puis charger le driver :

```bash
insmod /root/chardev.ko
mknod /dev/mon_device c 251 0
```

### Tester basic_app

```bash
/root/basic_app
```

Résultat attendu :
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
...
=== Application terminee ===
```

### Tester benchmark

```bash
/root/benchmark
```

Résultat attendu :
```
=================================================
   Embedded Linux ARM - Kernel Driver Benchmark
   Device : /dev/mon_device
   Iterations : 100
   Latency threshold : 1000 us
=================================================

  READ : [001] SENSOR_TEMP=56.6 SENSOR_PRESS=5.0 SENSOR_FLOW=8.6
  READ : [002] SENSOR_TEMP=64.1 SENSOR_PRESS=2.2 SENSOR_FLOW=6.3
  ...
  READ : [100] SENSOR_TEMP=59.2 SENSOR_PRESS=1.9 SENSOR_FLOW=9.5

=== Rapport Final ===
  WRITE      | min:  389 us | max: 1001 us | avg:  459 us | violations: 1/100
  READ       | min:  346 us | max:  845 us | avg:  422 us | violations: 0/100

  Total iterations : 100
  Threshold        : 1000 us
=== Benchmark termine ===
```

---

## Résumé de l'étape 4

| Élément | Détail |
|---|---|
| basic_app | Read/write simple avec mesure de latence |
| benchmark | Producteur/consommateur multi-thread |
| Threads | 2 threads POSIX synchronisés avec mutex + cond |
| Données | Capteurs simulés (TEMP/PRESS/FLOW) |
| Itérations | 100 cycles write + read |
| Write latence moyenne | 459 µs |
| Read latence moyenne | 422 µs |
| Violations | 1/100 write, 0/100 read |
