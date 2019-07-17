#automated jailhouse init

#!/bin/bash

modprobe jailhouse #insert jailhouse.ko module
jailhouse enable ultra96.cell
insmod uio_ivshmem.ko
jailhouse cell create ultra96-linux-demo.cell
jailhouse cell load non-root /home/root/erika_inmate.bin
jailhouse cell start non-root
#echo 64 > /proc/sys/vm/nr_hugepages

