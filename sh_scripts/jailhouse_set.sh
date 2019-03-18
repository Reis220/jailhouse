#automated jailhouse init

#!/bin/bash

modprobe jailhouse #insert jailhouse.ko module
jailhouse enable ultra96.cell
jailhouse cell create ultra96-linux-demo.cell
jailhouse cell load non-root-ultra96 erika-inmate.bin
jailhouse cell start non-root-ultra96 #start erika os
