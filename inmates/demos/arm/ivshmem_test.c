/*#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <getopt.h>
#include <sched.h>
#include <sys/mman.h>
#include <fcntl.h>*/
#include <inmate.h>

int
inmate_main(int argc, char* argv[])
{
	/*(unsigned int *) memfd = NULL;
	(unsigned int *) mapped_base = NULL;
	(unsigned int *) mapped_dev_base = NULL;

	memfd = open("/dev/mem", O_RDWR | O_SYNC);

	mapped_base = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0xfc100000 & ~MEM_MASK);

	mapped_dev_base = mapped_base + (dev_base & MEM_MASK);

	for(int i = 0; i < 8; i += 4) {
		*((volatile unsigned int *) (mapped_dev_base + i)) = 1;
		printf("Address: %p, Read valeu = %d\n", (void *)(mapped_dev_base + i), *((unsigned int *) (mapped_dev_base + i)));
	}*/

	unsigned int i=0;
	void * a=NULL;
	unsigned int readback = 10;
	printk("hello... %d \n",i);
	map_range((void*) 0x7bf00000, 0x10000, MAP_UNCACHED);
	a = (void *)(unsigned long) 0x7bf00000;
	mmio_write32(a, 1);
	printk("written at: %d the value: 1\n", a);
	
	return 0;
}
