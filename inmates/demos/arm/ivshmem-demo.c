/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (c) ARM Limited, 2014
 * Copyright (c) Siemens AG, 2014-2017
 *
 * Authors:
 *  Jean-Philippe Brucker <jean-philippe.brucker@arm.com>
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

//#include <mach.h>
#include <inmate.h>

#define VENDORID	0x1af4
#define DEVICEID	0x1110

#define IVSHMEM_CFG_SHMEM_PTR	0x40
#define IVSHMEM_CFG_SHMEM_SZ	0x48

#define JAILHOUSE_SHMEM_PROTO_UNDEFINED	0x0000

//#define IVSHMEM_IRQ 55
#define IVSHMEM_IRQ 140

#define MAX_NDEV	4


#define GICD_BASE_ADDR 0xf9010000
#define IVSHMEM_MEM_ADDR 0x7bf00000 //address of /dev/uio1 map[1] in root cell
#define IVSHMEM_PCI_REG_ADDR 0xfc100000 /*address of /dev/uio1 map[0] in root cell where exist the PCI registers:    
												    IntrMask = 0,
												    IntrStatus = 4,
												    IVPosition = 8,
												    Doorbell = 12,
												    IVLiveList = 16*/
#define PCI_CFG_BASE	0xfc000000

static char str[64] = "Hello from bare-metal ivshmem-demo inmate!!!  ";
static int irq_counter;

struct ivshmem_dev_data {
	u16 bdf;
	u32 *registers;
	void *shmem;
	//u32 *msix_table;
	u64 shmemsz;
	u64 bar2sz;
};

struct ivshmem_dev_data *d = NULL;
static struct ivshmem_dev_data devs[MAX_NDEV];

static u64 pci_cfg_read64(u16 bdf, unsigned int addr)
{
	u64 bar;

	bar = ((u64)pci_read_config(bdf, addr + 4, 4) << 32) |
	      pci_read_config(bdf, addr, 4);
	      
	/*bar = ((u64)arm_pci_read_config(bdf, addr + 4, 4) << 32) |
	      arm_pci_read_config(bdf, addr, 4);*/
	      
	return bar;
}

static void pci_cfg_write64(u16 bdf, unsigned int addr, u64 val)
{
	pci_write_config(bdf, addr + 4, (u32)(val >> 32), 4);
	pci_write_config(bdf, addr, (u32)val, 4);
	/*arm_pci_write_config(bdf, addr + 4, (u32)(val >> 32), 4);
	arm_pci_write_config(bdf, addr, (u32)val, 4);*/
}

static u64 get_bar_sz(u16 bdf, u8 barn)
{
	u64 bar, tmp;
	u64 barsz;

	bar = pci_cfg_read64(bdf, PCI_CFG_BAR + (8 * barn));
	pci_cfg_write64(bdf, PCI_CFG_BAR + (8 * barn), 0xffffffffffffffffULL);
	tmp = pci_cfg_read64(bdf, PCI_CFG_BAR + (8 * barn));
	barsz = ~(tmp & ~(0xf)) + 1;
	pci_cfg_write64(bdf, PCI_CFG_BAR + (8 * barn), bar);

	return barsz;
}

static int map_shmem_and_bars(struct ivshmem_dev_data *d)
{
	if (0 > pci_find_cap(d->bdf, PCI_CAP_MSIX)) {
		printk("IVSHMEM ERROR: device is not MSI-X capable\n");
		//return 1;
	}

	d->shmemsz = pci_cfg_read64(d->bdf, IVSHMEM_CFG_SHMEM_SZ);
	/*d->shmem = (void *)((u32)(0xffffffff & pci_cfg_read64(d->bdf, IVSHMEM_CFG_SHMEM_PTR)));*/
	d->shmem = (void *)((u64)(0xffffffffffffffff & pci_cfg_read64(d->bdf, IVSHMEM_CFG_SHMEM_PTR)));

	printk("IVSHMEM: shmem is at %p, shmemsz is %llu\n", d->shmem, d->shmemsz);
	//d->registers = (u32 *)(((u64)(d->shmem + d->shmemsz + PAGE_SIZE - 1)) & PAGE_MASK); // this gives 0x800500000 as a result, which I believe is wrong
	d->registers = (u32 *) IVSHMEM_PCI_REG_ADDR; //added by Giovani
 	/* (u32 *)(((u32)(d->shmem + d->shmemsz + PAGE_SIZE - 1)) & PAGE_MASK);*/
	/*pci_cfg_write64(d->bdf, PCI_CFG_BAR, (u32)d->registers);*/
	pci_cfg_write64(d->bdf, PCI_CFG_BAR, (u64)d->registers);
	printk("IVSHMEM: bar0 is at %p\n", d->registers);
	d->bar2sz = get_bar_sz(d->bdf, 2);

	/*
	d->msix_table =
	  (u32 *)(d->registers + PAGE_SIZE);
	pci_cfg_write64(d->bdf, PCI_CFG_BAR + 16, (u32)d->msix_table);
	printk("IVSHMEM: bar2 is at %p\n", d->msix_table);
	*/

	pci_write_config(d->bdf, PCI_CFG_COMMAND,
			 (PCI_CMD_MEM | PCI_CMD_MASTER), 2);
	return 0;
}

static u32 get_ivpos(struct ivshmem_dev_data *d)
{
	return mmio_read32(d->registers + 2);
}

static void send_irq(struct ivshmem_dev_data *d)
{
	printk("IVSHMEM: LSTATE = %d, RSTATE = %d, DOORBELL = %d\n", mmio_read32(d->registers + 4), mmio_read32(d->registers + 5), mmio_read32(d->registers + 3));
	printk("IVSHMEM: sending IRQ - addr = %p\n", (d->registers + 3));
	mmio_write32(d->registers + 3, 1); //DOORBELL register at offset 12byte
	printk("IVSHMEM: LSTATE = %d, RSTATE = %d, DOORBELL = %d\n", mmio_read32(d->registers + 4), mmio_read32(d->registers + 5), mmio_read32(d->registers + 3));
}

static void enable_irq(struct ivshmem_dev_data *d)
{
	printk("IVSHMEM: Enabling IVSHMEM_IRQs registers = %p\n", d->registers);
	mmio_write32(d->registers, 0xffffffff);
}

static void handle_IRQ(unsigned int irqn)
{
	printk("IVSHMEM: handle_irq(irqn:%d) - interrupt #%d\n", irqn, irq_counter++);
	/*if(irqn == IVSHMEM_IRQ) {
		printk("IVSHMEM: sending interrupt.\n");
		send_irq(d); //it works
		expected_ticks = timer_get_ticks() + ticks_per_beat;
		timer_start(ticks_per_beat);
	}*/
}

void inmate_main(void)
{
	unsigned int i = 0;
	int bdf = 0;
	unsigned int class_rev;
	//struct ivshmem_dev_data *d = NULL;
	volatile char *shmem;
	int ndevices = 0;
	

	printk("ivshmem-demo starting...\n");

	gic_setup(handle_IRQ);
	
	map_range((void*)GICD_BASE_ADDR, 0x1f000, MAP_UNCACHED);
	map_range((void*)IVSHMEM_MEM_ADDR, 0x100000, MAP_UNCACHED);
	map_range((void*)PCI_CFG_BASE, 0x1000ff, MAP_UNCACHED); 
	
	while ((ndevices < MAX_NDEV) && (-1 != (bdf = pci_find_device(VENDORID, DEVICEID, bdf)))) {		
		printk("IVSHMEM: Found %04x:%04x at %02x:%02x.%x\n",
		       pci_read_config(bdf, PCI_CFG_VENDOR_ID, 2),
		       pci_read_config(bdf, PCI_CFG_DEVICE_ID, 2),
		       bdf >> 8, (bdf >> 3) & 0x1f, bdf & 0x3);
		class_rev = pci_read_config(bdf, 0x8, 4);
		if (class_rev != (PCI_DEV_CLASS_OTHER << 24 |
				  JAILHOUSE_SHMEM_PROTO_UNDEFINED << 8)) {
			printk("IVSHMEM: class/revision %08x, not supported "
			       "skipping device\n", class_rev);
			bdf++;
			continue;
		}
		ndevices++;
		d = devs + ndevices - 1;
		d->bdf = bdf;
		if (map_shmem_and_bars(d)) {
			printk("IVSHMEM: Failure mapping shmem and bars.\n");
			return;
		}

		printk("IVSHMEM: mapped shmem and bars, guest's ID number is %d - bdf = %d\n",
		       get_ivpos(d), bdf);

		/* NULL terminating the string */
		str[63] = 0;
		printk("IVSHMEM: memcpy(%p, %s, %d)\n", d->shmem, str, 64);
		memcpy(d->shmem, str, 64);
		//printk("IVSHMEM: %p:%p\n", d->shmem, d->shmem);

		gic_enable_irq(IVSHMEM_IRQ + ndevices - 1);
		printk("IVSHMEM: Enabled IRQ:0x%x\n", IVSHMEM_IRQ +  ndevices -1);

		enable_irq(d);

		//pci_msix_set_vector(bdf, IVSHMEM_IRQ + ndevices - 1, 0);
		//printk("IVSHMEM: Vector set for PCI MSI-X.\n");
		bdf++;

	}

	if (!ndevices) {
		printk("IVSHMEM: No PCI devices found .. nothing to do.\n");
		return;
	}

	printk("IVSHMEM: Done setting up...\n");

	/*{

		u8 buf[64];
		memcpy(buf, d->shmem, sizeof(buf)/sizeof(buf[0]));
		buf[63] = 0;
		printk("IVSHMEM: %s\n", buf);
		memcpy(d->shmem, str, sizeof(str)/sizeof(str[0]));
	}*/

	while (1) {
		for (i = 0; i < ndevices; i++) {
			d = devs + i;
			//delay_us(1000*1000);
			shmem = d->shmem;
			shmem[19]++;
			printk("IVSHMEM: sending interrupt.\n");
			send_irq(d);
		}
		printk("IVSHMEM: waiting for interrupt.\n");
		asm volatile("wfi" : : : "memory");
	}
}
