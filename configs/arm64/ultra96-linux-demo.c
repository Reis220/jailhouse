/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Configuration for linux-demo inmate on Avnet Ultra96 board:
 * 2 CPUs, 128M RAM, serial port 2
 *
 * Copyright (c) Siemens AG, 2014-2019
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <jailhouse/types.h>
#include <jailhouse/cell-config.h>

#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

struct {
	struct jailhouse_cell_desc cell;
	__u64 cpus[1];
	struct jailhouse_memory mem_regions[7];
	struct jailhouse_irqchip irqchips[1];
	struct jailhouse_pci_device pci_devices[1];
} __attribute__((packed)) config = {
	.cell = {
		.signature = JAILHOUSE_CELL_DESC_SIGNATURE,
		.revision = JAILHOUSE_CONFIG_REVISION,
		.name = "non-root",
		.flags = JAILHOUSE_CELL_PASSIVE_COMMREG,/* |
		JAILHOUSE_CELL_VIRTUAL_CONSOLE_ACTIVE,*/

		.cpu_set_size = sizeof(config.cpus),
		.num_memory_regions = ARRAY_SIZE(config.mem_regions),
		.num_irqchips = ARRAY_SIZE(config.irqchips),
		.num_pci_devices = ARRAY_SIZE(config.pci_devices),

		.vpci_irq_base = 140-32,

		.console = {
			.address = 0xff010000, /*UART1*/
			//.address = 0xff000000, /*UART0*/ //se eu meter uart0 da erro unhandled trap
			.type= JAILHOUSE_CON_TYPE_XUARTPS,
			.flags = JAILHOUSE_CON_ACCESS_MMIO |
				 JAILHOUSE_CON_REGDIST_4,
		},
	},

	.cpus = {
		0xe, //1110
		//0x8, //1000 - fica com cpu3
		//0xc, //1100
	},

	.mem_regions = {
		/* UART */ {
			.phys_start = 0xff010000,
			.virt_start = 0xff010000,
			/*.phys_start = 0xff000000,
			.virt_start = 0xff000000,*/
			.size = 0x1000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_IO | JAILHOUSE_MEM_ROOTSHARED,
		},
		/* RAM */ {
			.phys_start = 0x7bef0000,
			.virt_start = 0,
			.size = 0x10000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_LOADABLE,
		},
		/* RAM */ {
			.phys_start = 0x74000000,
			.virt_start = 0x74000000,
			.size = 0x7ef0000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_EXECUTE | JAILHOUSE_MEM_DMA |
				JAILHOUSE_MEM_LOADABLE,
		},
		/* IVSHMEM shared memory region */ {
			.phys_start = 0x7bf00000,
			.virt_start = 0x7bf00000,
			.size = 0x100000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* communication region */ {
			.virt_start = 0x80000000,
			.size = 0x00001000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_COMM_REGION,
		},
		/* RAM */{
			.phys_start = 0x3fd00000,
			.virt_start = 0x3fd00000,
			.size = 0x202f0000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
		/* RAM */{
			.phys_start = 0,
			.virt_start = 0x10000,
			.size = 0x3ed00000,
			.flags = JAILHOUSE_MEM_READ | JAILHOUSE_MEM_WRITE |
				JAILHOUSE_MEM_ROOTSHARED,
		},
	},

	.irqchips = {
		/* GIC */ {
			.address = 0xf9010000, /* GICD base address - Display controller */
			.pin_base = 32, /* The first irqchip starts at .pin_base=32 as the first 32 interrupts are 
reserved for SGIs and PPIs. */
			.pin_bitmap = {
				//1 << (54 - 32),
				1 << (53 - 32), // cat /proc/interrupts interrupt da UART0 AQUI ESTA A DIFERENÇA
				0,
				0,
				(1 << (140 - 128)) | (1 << (142 - 128)) //PL to PS interrupt signals 8 to 15.
			},
		},
	},

	.pci_devices = {
		/* 00:00.0 */ {
			.type = JAILHOUSE_PCI_TYPE_IVSHMEM,
			.bdf = 0 << 3, // 00:00.0
			.iommu = 1,//
			.bar_mask = {
				0xffffff00, 0xffffffff, 0x00000000,
				0x00000000, 0x00000000, 0x00000000,
			},
			.shmem_region = 3,
			//.shmem_protocol = JAILHOUSE_SHMEM_PROTO_VETH,
			//.shmem_protocol = JAILHOUSE_SHMEM_PROTO_CUSTOM,
			.shmem_protocol = JAILHOUSE_SHMEM_PROTO_UNDEFINED,
			//.num_msix_vectors = 1,
		},
	},
};
