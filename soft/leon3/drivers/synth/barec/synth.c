/*
 * Copyright (c) 2011-2019 Columbia University, System Level Design Group
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Baremetal device driver for SYNTH
 *
 * Select Scatter-Gather in ESP configuration
 */

#ifndef __riscv
#include <stdio.h>
#endif
#include <stdlib.h>
#include <esp_accelerator.h>
#include <esp_probe.h>

#define SLD_SYNTH   0x014
#define DEV_NAME "sld,synth"
#define TRIALS 10

#define PATTERN_STREAMING 0
#define PATTERN_STRIDED 1
#define PATTERN_IRREGULAR 2

#define SYNTH_OFFSET_REG 0x40
#define SYNTH_PATTERN_REG 0x44
#define SYNTH_IN_SIZE_REG 0x48
#define SYNTH_ACCESS_FACTOR_REG 0x4c
#define SYNTH_BURST_LEN_REG 0x50
#define SYNTH_COMPUTE_BOUND_FACTOR_REG 0x54
#define SYNTH_IRREGULAR_SEED_REG 0x58
#define SYNTH_REUSE_FACTOR_REG 0x5c
#define SYNTH_LD_ST_RATIO_REG 0x60
#define SYNTH_STRIDE_LEN_REG 0x64
#define SYNTH_OUT_SIZE_REG 0x68
#define SYNTH_IN_PLACE_REG 0x6c
#define SYNTH_WR_DATA_REG 0x70

// Accelerator-specific buffe size
#define IN_SIZE 524288
#define LD_ST_RATIO 1
#define ACCESS_FACTOR 0
#define OUT_SIZE ((IN_SIZE / LD_ST_RATIO) >> ACCESS_FACTOR)
#define SYNTH_BUF_SIZE ((IN_SIZE + OUT_SIZE) * sizeof(unsigned))
#define OUT_DATA 0x12345678
#define IN_PLACE 1 
/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK ((SYNTH_BUF_SIZE % CHUNK_SIZE == 0) ?			\
			(SYNTH_BUF_SIZE / CHUNK_SIZE) :			\
			(SYNTH_BUF_SIZE / CHUNK_SIZE) + 1)

// User defined registers



int main(int argc, char * argv[])
{
	int n, trial;
	int ndev;
	struct esp_device *espdevs = NULL;
	unsigned coherence;

	ndev = probe(&espdevs, SLD_SYNTH, DEV_NAME);
	if (!ndev) {
#ifndef __riscv
		printf("Error: %s device not found!\n", DEV_NAME);
#else
		print_uart("Error: "); print_uart(DEV_NAME); print_uart(" device not found!\n");
#endif
		exit(EXIT_FAILURE);
	}

	for (trial = 0; trial < TRIALS; trial++) {
#ifndef __riscv
		for (coherence = ACC_COH_NONE; coherence <= ACC_COH_RECALL; coherence++) {
#else
		{
			/* TODO: Restore full test once ESP caches are integrated */
			coherence = ACC_COH_NONE;
#endif
            struct esp_device *dev;
            unsigned **ptable;
            unsigned *mem;
            int scatter_gather = 1;
            for (n = 0; n < 1; n++){
                dev = &espdevs[n];
                int i;
#ifndef __riscv
                printf("******************** %s.%d ********************\n", DEV_NAME, n);
#else
                print_uart("******************** "); print_uart(DEV_NAME); print_uart(".");
                print_uart_int(n); print_uart(" ********************\n");
#endif
                // Check if scatter-gather DMA is disabled
                if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
#ifndef __riscv
                    printf("  -> scatter-gather DMA is disabled; revert to contiguous buffer.\n");
#else
                    print_uart("  -> scatter-gather DMA is disabled; revert to contiguous buffer.\n");
#endif
                    scatter_gather = 0;
                } else {
#ifndef __riscv
                    printf("  -> scatter-gather DMA is enabled.\n");
#else
                    print_uart("  -> scatter-gather DMA is enabled.\n");
#endif
                }

                if (scatter_gather)
                    if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK) {
#ifndef __riscv
                        printf("  Trying to allocate %lu chunks on %d TLB available entries\n",
                            NCHUNK, ioread32(dev, PT_NCHUNK_MAX_REG));
#else
                        print_uart("  Trying to allocate more chunks than available TLB entries\n");
#endif
                        break;
                    }

                // Allocate memory (will be contigous anyway in baremetal)
                mem = aligned_malloc(SYNTH_BUF_SIZE);
#ifndef __riscv
                printf("  memory buffer base-address = %p\n", mem);
#else
                print_uart("  memory buffer base-address = 0x"); print_uart_int((unsigned long) mem); print_uart("\n");
#endif

                if (scatter_gather) {
                    //Alocate and populate page table
                    ptable = aligned_malloc(NCHUNK * sizeof(unsigned *));
                    for (i = 0; i < NCHUNK; i++)
                        ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(unsigned))];
#ifndef __riscv
                    printf("  ptable = %p\n", ptable);
                    printf("  nchunk = %lu\n", NCHUNK);
#else
                    print_uart("  ptable = 0x"); print_uart_int((unsigned long) ptable); print_uart("\n");
                    print_uart("  nchunk = 0x"); print_uart_int(NCHUNK); print_uart("\n");
#endif
                }

                // Initialize input: write floating point hex values (simpler to debug)

                // Configure device
                iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
                iowrite32(dev, COHERENCE_REG, coherence);

                if (scatter_gather) {
                    iowrite32(dev, PT_ADDRESS_REG, (unsigned long) ptable);
                    iowrite32(dev, PT_NCHUNK_REG, NCHUNK);
                    iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
                    iowrite32(dev, SRC_OFFSET_REG, 0);
                    iowrite32(dev, DST_OFFSET_REG, 0); // Sort runs in place
                } else {
                    iowrite32(dev, SRC_OFFSET_REG, (unsigned long) mem);
                    iowrite32(dev, DST_OFFSET_REG, (unsigned long) mem); // Sort runs in place
                }

                // Accelerator-specific registers
                iowrite32(dev, SYNTH_OFFSET_REG, 0);
                iowrite32(dev, SYNTH_PATTERN_REG, PATTERN_STRIDED); 
                iowrite32(dev, SYNTH_IN_SIZE_REG, IN_SIZE);
                iowrite32(dev, SYNTH_ACCESS_FACTOR_REG, ACCESS_FACTOR);
                iowrite32(dev, SYNTH_BURST_LEN_REG, 4);
                iowrite32(dev, SYNTH_COMPUTE_BOUND_FACTOR_REG, 2);
                iowrite32(dev, SYNTH_IRREGULAR_SEED_REG, 0); 
                iowrite32(dev, SYNTH_REUSE_FACTOR_REG, 8);
                iowrite32(dev, SYNTH_LD_ST_RATIO_REG, LD_ST_RATIO);
                iowrite32(dev, SYNTH_STRIDE_LEN_REG, 4); 
                iowrite32(dev, SYNTH_OUT_SIZE_REG, OUT_SIZE); 
                iowrite32(dev, SYNTH_IN_PLACE_REG, IN_PLACE);
                iowrite32(dev, SYNTH_WR_DATA_REG, OUT_DATA);

                // Flush for non-coherent DMA
                esp_flush(coherence);
            }

			// Start accelerator
            for (n = 0; n < 1; n++){
#ifndef __riscv
			    printf("  Start..\n");
#else
			    print_uart("  Start..\n");
#endif
                dev = &espdevs[n];
			    iowrite32(dev, CMD_REG, CMD_MASK_START);
            }
            
            unsigned done0 = 0;
			
			while (!done0) {
                dev = &espdevs[0];
				done0 = ioread32(dev, STATUS_REG);
				done0 &= STATUS_MASK_DONE;
            }
			
            iowrite32(dev, CMD_REG, 0x0);
#ifndef __riscv
			printf("  Validating...\n");
#else
			print_uart("  Validating...\n");
#endif

			
            uint32_t start = IN_PLACE ? 0 : IN_SIZE;
            uint32_t errors = 0;
            uint32_t i;
            for (i = start; i < start + OUT_SIZE; i++){
                if (mem[i] != OUT_DATA) 
                    errors++;
            }
#ifndef __riscv
			printf(" %d errors \n", errors);
#else
			print_uart_int(errors); print_uart(" errors\n");
#endif

#ifndef __riscv
			printf("  Done\n");
#else
			print_uart("  Done\n");
#endif

            
            // Validation

			if (scatter_gather)
				aligned_free(ptable);
			aligned_free(mem);

#ifndef __riscv
			printf("**************************************************\n\n");
#else
			print_uart("**************************************************\n\n");
#endif
		}
	}
	return 0;
}
