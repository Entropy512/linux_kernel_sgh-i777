/* Cypress WestBridge C110 Kernel Hal source file (cyashalc110_cram.c)
## ===========================
## Copyright (C) 2010  Cypress Semiconductor
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor,
## Boston, MA  02110-1301, USA.
## ===========================
*/

#ifdef CONFIG_MACH_C110_WESTBRIDGE_AST_CRAM_HAL

#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/mm.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/clk.h>
/* include seems broken moving for patch submission
 * #include <mach/mux.h>
 * #include <mach/gpmc.h>
 * #include <mach/westbridge/westbridge-omap3-pnand-hal/cyashalomap_cram.h>
 * #include <mach/westbridge/westbridge-omap3-pnand-hal/cyasomapdev_cram.h>
 * #include <mach/westbridge/westbridge-omap3-pnand-hal/cyasmemmap.h>
 * #include <linux/westbridge/cyaserr.h>
 * #include <linux/westbridge/cyasregs.h>
 * #include <linux/westbridge/cyasdma.h>
 * #include <linux/westbridge/cyasintr.h>
 */
//#include <mach/mux.h>
#include <mach/gpio.h>
#include <linux/../../arch/arm/plat-samsung/include/plat/gpio-cfg.h>

#include "../plat-c110/include/mach/westbridge/westbridge-c110-cram-hal/cyashalc110_cram.h"
#include "../plat-c110/include/mach/westbridge/westbridge-c110-cram-hal/cyasc110dev_cram.h"
#include "../plat-c110/include/mach/westbridge/westbridge-c110-cram-hal/cyasmemmap.h"
#include "../../../include/linux/westbridge/cyaserr.h"
#include "../../../include/linux/westbridge/cyasregs.h"
#include "../../../include/linux/westbridge/cyasdma.h"
#include "../../../include/linux/westbridge/cyasintr.h"

#define HAL_REV "1.1.0"



/*
 * westbrige astoria ISR options to limit number of
 * back to back DMA transfers per ISR interrupt
 */
#define MAX_DRQ_LOOPS_IN_ISR 128

/*
 * debug prints enabling
 *#define DBGPRN_ENABLED
 *#define DBGPRN_DMA_SETUP_RD
 *#define DBGPRN_DMA_SETUP_WR
 */


/*
 * For performance reasons, we handle storage endpoint transfers upto 4 KB
 * within the HAL itself.
 */
 #define CYASSTORAGE_WRITE_EP_NUM	(4)
 #define CYASSTORAGE_READ_EP_NUM	(8)

/*
 *  size of DMA packet HAL can accept from Storage API
 *  HAL will fragment it into smaller chunks that the P port can accept
 */
#define CYASSTORAGE_MAX_XFER_SIZE	(2*32768)

/*
 *  P port MAX DMA packet size according to interface/ep configurartion
 */
#define HAL_DMA_PKT_SZ 512

#define is_storage_e_p(ep) (((ep) == 2) || ((ep) == 4) || \
				((ep) == 6) || ((ep) == 8))


/*
 * keep processing new WB DRQ in ISR untill all handled (performance feature)
 */
#define PROCESS_MULTIPLE_DRQ_IN_ISR (1)


/*
 * The type of DMA operation, per endpoint
 */
typedef enum cy_as_hal_dma_type {
	cy_as_hal_read,
	cy_as_hal_write,
	cy_as_hal_none
} cy_as_hal_dma_type ;


/*
 * SG list halpers defined in scaterlist.h
#define sg_is_chain(sg)		((sg)->page_link & 0x01)
#define sg_is_last(sg)		((sg)->page_link & 0x02)
#define sg_chain_ptr(sg)	\
	((struct scatterlist *) ((sg)->page_link & ~0x03))
*/
typedef struct cy_as_hal_endpoint_dma {
	cy_bool buffer_valid ;
	uint8_t *data_p ;
	uint32_t size ;
	/*
	 * sg_list_enabled - if true use, r/w DMA transfers use sg list,
	 *		FALSE use pointer to a buffer
	 * sg_p - pointer to the owner's sg list, of there is such
	 *		(like blockdriver)
	 * dma_xfer_sz - size of the next dma xfer on P port
	 * seg_xfer_cnt -  counts xfered bytes for in current sg_list
	 *		memory segment
	 * req_xfer_cnt - total number of bytes transfered so far in
	 *		current request
	 * req_length - total request length
	 */
	bool sg_list_enabled;
	struct scatterlist *sg_p ;
	uint16_t dma_xfer_sz;
	uint32_t seg_xfer_cnt;
	uint16_t req_xfer_cnt;
	uint16_t req_length;
	cy_as_hal_dma_type type ;
	cy_bool pending ;
} cy_as_hal_endpoint_dma ;

/*
 * The list of c110 devices (should be one)
 */
static cy_as_c110_dev_kernel *m_c110_list_p;

/*
 * The callback to call after DMA operations are complete
 */
static cy_as_hal_dma_complete_callback callback;

/*
 * Pending data size for the endpoints
 */
static cy_as_hal_endpoint_dma end_points[16] ;

/*
 * Forward declaration
 */
static void cy_handle_d_r_q_interrupt(cy_as_c110_dev_kernel *dev_p, uint16_t read_val);

static uint16_t intr_sequence_num;
static uint8_t intr__enable;
spinlock_t int_lock ;

static u32 iomux_vma;


bool have_irq;

/*
 * west bridge astoria ISR (Interrupt handler)
 */
static irqreturn_t cy_astoria_int_handler(int irq,
				void *dev_id, struct pt_regs *regs)
{
	cy_as_c110_dev_kernel *dev_p;
	uint16_t		  read_val = 0 ;
	uint16_t		  mask_val = 0 ;

	/*
	* debug stuff, counts number of loops per one intr trigger
	*/
	uint16_t		  drq_loop_cnt = 0;
	uint8_t		   irq_pin;
	/*
	 * flags to watch
	 */
	const uint16_t	sentinel = (CY_AS_MEM_P0_INTR_REG_MCUINT |
				CY_AS_MEM_P0_INTR_REG_MBINT |
				CY_AS_MEM_P0_INTR_REG_PMINT |
				CY_AS_MEM_P0_INTR_REG_PLLLOCKINT);

	/*
	 * sample IRQ pin level (just for statistics)
	 */
	irq_pin = gpio_get_value(WB_CYAS_INT);

	/*
	 * this one just for debugging
	 */
	intr_sequence_num++ ;

	/*
	 * astoria device handle
	 */
	dev_p = dev_id;

	/*
	 * read Astoria intr register
	 */
	read_val = cy_as_hal_read_register((cy_as_hal_device_tag)dev_p, CY_AS_MEM_P0_INTR_REG) ;

	/*
	 * save current mask value
	 */
	mask_val = cy_as_hal_read_register((cy_as_hal_device_tag)dev_p, CY_AS_MEM_P0_INT_MASK_REG) ;

#ifdef DBGPRN_ENABLED
	DBGPRN("<1>HAL__intr__enter:_seq:%d, P0_INTR_REG:%x\n", intr_sequence_num, read_val);
#endif
	/*
	 * Disable WB interrupt signal generation while we are in ISR
	 */
	cy_as_hal_write_register((cy_as_hal_device_tag)dev_p, CY_AS_MEM_P0_INT_MASK_REG, 0x0000) ;

	/*
	* this is a DRQ Interrupt
	*/
	if (read_val & CY_AS_MEM_P0_INTR_REG_DRQINT) {

		do {
			/*
			 * handle DRQ interrupt
			 */
			drq_loop_cnt++;

			cy_handle_d_r_q_interrupt(dev_p, read_val) ;

			/*
			 * spending to much time in ISR may impact
			 * average system performance
			 */
			if (drq_loop_cnt >= MAX_DRQ_LOOPS_IN_ISR)
				break;

			read_val = cy_as_hal_read_register((cy_as_hal_device_tag)dev_p,
								CY_AS_MEM_P0_INTR_REG) ;
		/*
		 * Keep processing if there is another DRQ int flag
		 */
		} while (read_val &
					CY_AS_MEM_P0_INTR_REG_DRQINT);

		/* Call the API interrupt handler to drain the mailbox queue. */
		cy_as_intr_service_interrupt((cy_as_hal_device_tag)dev_p) ;

	}

	if (read_val & sentinel)
		cy_as_intr_service_interrupt((cy_as_hal_device_tag)dev_p) ;

#ifdef DBGPRN_ENABLED
	DBGPRN("<1>_hal:_intr__exit seq:%d, mask=%4.4x,int_pin:%d DRQ_jobs:%d\n",
			intr_sequence_num, mask_val, irq_pin, drq_loop_cnt);
#endif
	/*
	 * re-enable WB hw interrupts
	 */
	cy_as_hal_write_register((cy_as_hal_device_tag)dev_p, CY_AS_MEM_P0_INT_MASK_REG, mask_val) ;

	return IRQ_HANDLED ;
}

static int cy_as_hal_configure_interrupts(void *dev_p)
{
	int result;
	int irq_pin  = WB_CYAS_INT;

	set_irq_type(WB_CYAS_IRQ_INT, IRQ_TYPE_LEVEL_LOW);

	/*
	 * for shared IRQS must provide non NULL device ptr
	 * othervise the int won't register
	 * */
	result = request_irq(WB_CYAS_IRQ_INT,	(irq_handler_t)cy_astoria_int_handler, IRQF_SHARED, "AST_INT#", dev_p);

	if (result == 0) {
		cy_as_hal_print_message(KERN_INFO"WB_CYAS_INT c110_pin: %d assigned IRQ #%d \n", irq_pin,  WB_CYAS_IRQ_INT);
		have_irq = true;

	} else {
		cy_as_hal_print_message("cyasc110hal: interrupt failed to register\n");
		gpio_free(irq_pin);
		cy_as_hal_print_message(KERN_WARNING"ASTORIA: can't get assigned IRQ%i for INT#\n", WB_CYAS_IRQ_INT);
		have_irq = false;
	}

	return result;
}


/*
 * inits all c110 h/w
 */
uint32_t cy_as_hal_processor_hw_init(cy_as_c110_dev_kernel *dev_p)
{
	int 	reg_val;
	struct clk *clk ;
	struct resource *nandArea;
	unsigned int cs_mem_base;
	unsigned int cs_vma_base;
	
	cy_as_hal_print_message(KERN_INFO "init C110 hw...\n");


	//set gpio settings for sideload chipset (nxz)

	//SIDE_CLK enable for power
	//s3c_gpio_cfgpin(WB_CLK_EN, S3C_GPIO_OUTPUT);
	//s3c_gpio_setpin(WB_CLK_EN, 1);
	//msleep(10);

	//SIDE_WAKEUP for sideload
	s3c_gpio_cfgpin(WB_WAKEUP, S3C_GPIO_OUTPUT);
	s3c_gpio_setpin(WB_WAKEUP, 1);

	//SIDE_RST
	s3c_gpio_cfgpin(WB_RESET, S3C_GPIO_OUTPUT);
	s3c_gpio_setpin(WB_RESET, 0);
	msleep(10);
	s3c_gpio_setpin(WB_RESET, 1);

	//SIDE_INT for interrupt  
	s3c_gpio_cfgpin(WB_CYAS_INT, S3C_GPIO_SFN(15));
	s3c_gpio_setpull(WB_CYAS_INT, S3C_GPIO_PULL_NONE);

	//SIDE_CD for detecting SD card
	s3c_gpio_cfgpin(WB_SDCD_INT, S3C_GPIO_SFN(15));
	s3c_gpio_setpull(WB_SDCD_INT, S3C_GPIO_PULL_NONE);

#if 0
	//Tri-state MoviNAND bus
	s3c_gpio_cfgpin(GPIO_NAND_CLK, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_NAND_CMD, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_NAND_D0, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_NAND_D1, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_NAND_D2, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_NAND_D3, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_NAND_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_NAND_CMD, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_NAND_D0, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_NAND_D1, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_NAND_D2, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_NAND_D3, S3C_GPIO_PULL_NONE);

	//Tri-state SD
	s3c_gpio_cfgpin(GPIO_T_FLASH_CLK, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_T_FLASH_CMD, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_T_FLASH_D0, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_T_FLASH_D1, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_T_FLASH_D2, S3C_GPIO_INPUT);
	s3c_gpio_cfgpin(GPIO_T_FLASH_D3, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_T_FLASH_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_T_FLASH_CMD, S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(GPIO_T_FLASH_D0, S3C_GPIO_PULL_NONE);
#endif

	//Setup PADs for NAND controller
	// set CS2 pin for SRAM 
	s3c_gpio_cfgpin(S5PV210_MP01(2), S3C_GPIO_SFN(2));
	s3c_gpio_setpull(S5PV210_MP01(2), S3C_GPIO_PULL_NONE);

	/* Enable clock */
	clk = clk_get(NULL, "sromc");
	if ( IS_ERR(clk) )
	{
		printk(KERN_ERR "Cannot get nfcon\n");
	}
	clk_enable(clk);

	//set SROM Controller Configuration
	nandArea = request_mem_region(S5PC110_PA_SROMC, 0x1000, "sromController");
	if ( nandArea == NULL )
	{
		printk(KERN_ERR "Cannot request SRAM controller's register memory region\n");
	}

	cs_vma_base = ioremap(S5PC110_PA_SROMC, 0x1000);
	if ( cs_vma_base == NULL )
	{
		printk(KERN_ERR "Cannot map SRAM controller's register space to memory\n");	
	}
	/* Setup SRAM CFG register */
	// set SROM_BW
	reg_val = readl( cs_vma_base );
	reg_val |= 0x100;
	writel(reg_val, cs_vma_base);

	// set SROM_BC2
	reg_val = 0x01051000; // 0x03061000; //0x33093300;
	writel(reg_val, cs_vma_base+0xC);
	
	//get virtual SRAM Base address 
	nandArea = request_mem_region(CYAS_DEV_BASE_ADDR, 0x1000, "sromController");
	if ( nandArea == NULL )
	{
		printk(KERN_ERR "Cannot request SRAM controller's register memory region\n");
	}

	cs_vma_base = ioremap(CYAS_DEV_BASE_ADDR, 0x1000);
	if ( cs_vma_base == NULL )
	{
		printk(KERN_ERR "Cannot map SRAM controller's register space to memory\n");	
	}
	dev_p->m_phy_addr_base = (void *) CYAS_DEV_BASE_ADDR;
	dev_p->m_vma_addr_base = (void *) cs_vma_base;

	return 0;
}
EXPORT_SYMBOL(cy_as_hal_processor_hw_init);

void cy_as_hal_c110_hardware_deinit(cy_as_c110_dev_kernel *dev_p)
{
	/*
	 * free C110 hw resources
	 */
	if (dev_p->m_vma_addr_base != 0)
		iounmap((void *)dev_p->m_vma_addr_base);

	if (dev_p->m_phy_addr_base != 0)
		release_mem_region((void *)dev_p->m_phy_addr_base, SZ_16K);

	if (have_irq)
		free_irq(WB_CYAS_IRQ_INT, dev_p);

}

/*
 * These are the functions that are not part of the
 * HAL layer, but are required to be called for this HAL
 */

/*
 * Called On AstDevice LKM exit
 */
int cy_as_hal_c110_cram_stop(const char *pgm, cy_as_hal_device_tag tag)
{
	cy_as_c110_dev_kernel *dev_p = (cy_as_c110_dev_kernel *)tag ;

	/*
	 * TODO: Need to disable WB interrupt handlere 1st
	 */
	if (0 == dev_p)
		return 1 ;

	cy_as_hal_print_message("<1>_stopping c110 HAL layer object\n");
	if (dev_p->m_sig != CY_AS_C110_CRAM_HAL_SIG) 
	{
		cy_as_hal_print_message("<1>%s: %s: bad HAL tag\n",	pgm, __func__) ;
		return 1 ;
	}

	/*
	 * disable interrupt
	 */
	cy_as_hal_write_register((cy_as_hal_device_tag)dev_p,	CY_AS_MEM_P0_INT_MASK_REG, 0x0000) ;

#if 0
	if (dev_p->thread_flag == 0) {
		dev_p->thread_flag = 1 ;
		wait_for_completion(&dev_p->thread_complete) ;
		cy_as_hal_print_message("cyasc110hal:"
			"done cleaning thread\n");
		cy_as_hal_destroy_sleep_channel(&dev_p->thread_sc) ;
	}
#endif

	cy_as_hal_c110_hardware_deinit(dev_p);

	/*
	 * Rearrange the list
	 */
	if (m_c110_list_p == dev_p)
		m_c110_list_p = dev_p->m_next_p ;

	cy_as_hal_free(dev_p) ;

	cy_as_hal_print_message(KERN_INFO"C110_kernel_hal stopped\n");
	return 0;
}

int c110_start_intr(cy_as_hal_device_tag tag)
{
	cy_as_c110_dev_kernel *dev_p = (cy_as_c110_dev_kernel *)tag ;
	int ret = 0 ;
	const uint16_t mask = CY_AS_MEM_P0_INTR_REG_DRQINT | CY_AS_MEM_P0_INTR_REG_MBINT ;

	/*
	 * register for interrupts
	 */
	ret = cy_as_hal_configure_interrupts(dev_p) ;

	/*
	 * enable only MBox & DRQ interrupts for now
	 */
	cy_as_hal_write_register((cy_as_hal_device_tag)dev_p, CY_AS_MEM_P0_INT_MASK_REG, mask) ;

	return 1 ;
}

/*
 * Below are the functions that communicate with the WestBridge device.
 * These are system dependent and must be defined by the HAL layer
 * for a given system.
 */



/*
 * This function must be defined to write a register within the WestBridge
 * device.  The addr value is the address of the register to write with
 * respect to the base address of the WestBridge device.
 */
void cy_as_hal_write_register(
					cy_as_hal_device_tag tag,
					uint16_t addr, uint16_t data)
{
	uint32_t write_addr;
	cy_as_c110_dev_kernel *dev_p = (cy_as_c110_dev_kernel *)tag ;

	#ifndef WESTBRIDGE_NDEBUG
	if (dev_p->m_sig != CY_AS_C110_CRAM_HAL_SIG) {
		printk("%s: bad TAG parameter passed\n",  __func__) ;
		return ;
	}
	#endif

	write_addr = (void *)(dev_p->m_vma_addr_base + CYAS_DEV_CALC_ADDR (addr));
    writew ((unsigned short)data, write_addr) ;
}

/*
 * This function must be defined to read a register from the WestBridge
 * device.  The addr value is the address of the register to read with
 * respect to the base address of the WestBridge device.
 */
uint16_t cy_as_hal_read_register(cy_as_hal_device_tag tag, uint16_t addr)
{
	uint16_t data  = 0 ;
	uint32_t read_addr = 0;
	cy_as_c110_dev_kernel *dev_p = (cy_as_c110_dev_kernel *)tag ;

	#ifndef WESTBRIDGE_NDEBUG
	if (dev_p->m_sig != CY_AS_C110_CRAM_HAL_SIG) {
		printk("%s: bad TAG parameter passed\n",  __func__) ;
		return dev_p->m_sig;
	}
	#endif

	read_addr = (void *) (dev_p->m_vma_addr_base + CYAS_DEV_CALC_ADDR(addr));
    data     = (unsigned short) readw (read_addr) ;

	return data ;
}

/*
 * preps Ep pointers & data counters for next packet
 * (fragment of the request) xfer returns true if
 * there is a next transfer, and false if all bytes in
 * current request have been xfered
 */
static inline bool prep_for_next_xfer(cy_as_hal_device_tag tag, uint8_t ep)
{

	if (!end_points[ep].sg_list_enabled) {
		/*
		 * no further transfers for non storage EPs
		 * (like EP2 during firmware download, done
		 * in 64 byte chunks)
		 */
		if (end_points[ep].req_xfer_cnt >= end_points[ep].req_length) {
#ifdef DBGPRN_ENABLED
			DBGPRN("<1> %s():RQ sz:%d non-_sg EP:%d completed\n",
				__func__, end_points[ep].req_length, ep);
#endif
			/*
			 * no more transfers, we are done with the request
			 */
			return false;
		}

		/*
		 * calculate size of the next DMA xfer, corner
		 * case for non-storage EPs where transfer size
		 * is not egual N * HAL_DMA_PKT_SZ xfers
		 */
		if ((end_points[ep].req_length - end_points[ep].req_xfer_cnt)	>= HAL_DMA_PKT_SZ) {
				end_points[ep].dma_xfer_sz = HAL_DMA_PKT_SZ;
		}
		else {
			/*
			 * that would be the last chunk less
			 * than P-port max size
			 */
			end_points[ep].dma_xfer_sz = end_points[ep].req_length -
					end_points[ep].req_xfer_cnt;
		}

		return true;
	}

	/*
	 * for SG_list assisted dma xfers
	 * are we done with current SG ?
	 */
	if (end_points[ep].seg_xfer_cnt ==  end_points[ep].sg_p->length) {
		/*
		 *  was it the Last SG segment on the list ?
		 */
		if (sg_is_last(end_points[ep].sg_p)) {
#ifdef DBGPRN_ENABLED
			DBGPRN("<1> %s: EP:%d completed,"
					"%d bytes xfered\n",
					__func__,
					ep,
					end_points[ep].req_xfer_cnt
			);
#endif
			return false;
		} else {
			/*
			 * There are more SG segments in current
			 * request's sg list setup new segment
			 */

			end_points[ep].seg_xfer_cnt = 0;
			end_points[ep].sg_p = sg_next(end_points[ep].sg_p);
			/* set data pointer for next DMA sg transfer*/
			end_points[ep].data_p = sg_virt(end_points[ep].sg_p);
#ifdef DBGPRN_ENABLED
			DBGPRN("<1> %s new SG:_va:%p\n\n",
					__func__, end_points[ep].data_p);
#endif
		}

	}

	/*
	 * for sg list xfers it will always be 512 or 1024
	 */
	end_points[ep].dma_xfer_sz = HAL_DMA_PKT_SZ;

	/*
	 * next transfer is required
	 */

	return true;
}

/*
 * Astoria DMA read request, APP_CPU reads from WB ep buffer
 */
static void cy_service_e_p_dma_read_request(
			cy_as_c110_dev_kernel *dev_p, uint8_t ep)
{
	cy_as_hal_device_tag tag = (cy_as_hal_device_tag)dev_p ;
	uint16_t  v, i, size;
	uint16_t	*dptr;
	uint16_t ep_dma_reg = CY_AS_MEM_P0_EP2_DMA_REG + ep - 2;
    register void     *read_addr ;
    register uint16_t a,b,c,d,e,f,g,h ;
	/*
	 * get the XFER size frtom WB eP DMA REGISTER
	 */
	v = cy_as_hal_read_register(tag, ep_dma_reg);

	/*
	 * amount of data in EP buff in  bytes
	 */
	size =  v & CY_AS_MEM_P0_E_pn_DMA_REG_COUNT_MASK;

	/*
	 * memory pointer for this DMA packet xfer (sub_segment)
	 */
	dptr = (uint16_t *) end_points[ep].data_p;

	read_addr = dev_p->m_vma_addr_base + CYAS_DEV_CALC_EP_ADDR(ep) ;

	cy_as_hal_assert(size != 0);

	if (size) {
#if 1
	     /*
		 * Now, read the data from the device
		 */
		for(i = size/16 ; i > 0 ; i--) {
			a = (unsigned short) __raw_readw (read_addr) ;
			b = (unsigned short) __raw_readw (read_addr) ;
			c = (unsigned short) __raw_readw (read_addr) ;
			d = (unsigned short) __raw_readw (read_addr) ;
			e = (unsigned short) __raw_readw (read_addr) ;
			f = (unsigned short) __raw_readw (read_addr) ;
			g = (unsigned short) __raw_readw (read_addr) ;
			h = (unsigned short) __raw_readw (read_addr) ;
	                *dptr++ = a ;
	                *dptr++ = b ;
	                *dptr++ = c ;
	                *dptr++ = d ;
	                *dptr++ = e ;
	                *dptr++ = f ;
	                *dptr++ = g ;
	                *dptr++ = h ;
		}
	
		switch ((size & 0xF)/2) {
		case 7:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
		case 6:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
		case 5:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
		case 4:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
		case 3:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
		case 2:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
		case 1:
		    *dptr = (unsigned short) readw(read_addr) ;
		    dptr++ ;
	            break ;
		}
#else
		__raw_readsw(read_addr, dptr,  size/2);
		//__cy_rawreadsw(dptr, read_addr, size);
#endif
		if (size & 1) {
			/* Odd sized packet */
			uint16_t d = (unsigned short) readw (read_addr) ;
			*((uint8_t *)dptr) = (d & 0xff) ;
		}
	}
	/*
	 * clear DMAVALID bit indicating that the data has been read
	 */
	cy_as_hal_write_register(tag, ep_dma_reg, 0) ;

	end_points[ep].seg_xfer_cnt += size;
	end_points[ep].req_xfer_cnt += size;

	/*
	 *  pre-advance data pointer (if it's outside sg
	 * list it will be reset anyway
	 */
	end_points[ep].data_p += size;

	if (prep_for_next_xfer(tag, ep)) {
		/*
		 * we have more data to read in this request,
		 * setup next dma packet due tell WB how much
		 * data we are going to xfer next
		 */
		v = end_points[ep].dma_xfer_sz/*HAL_DMA_PKT_SZ*/ |
				CY_AS_MEM_P0_E_pn_DMA_REG_DMAVAL ;
		cy_as_hal_write_register(tag, ep_dma_reg, v);
		ndelay(200);
	} else {
		end_points[ep].pending	  = cy_false ;
		end_points[ep].type		 = cy_as_hal_none ;
		end_points[ep].buffer_valid = cy_false ;

		/*
		 * notify the API that we are done with rq on this EP
		 */
		if (callback) {
#ifdef DBGPRN_ENABLED
			DBGPRN("<1>trigg rd_dma completion cb: xfer_sz:%d\n",
				end_points[ep].req_xfer_cnt);
#endif
				callback(tag, ep,
					end_points[ep].req_xfer_cnt,
					CY_AS_ERROR_SUCCESS);
		}
	}
}

/*
 * c110_cpu needs to transfer data to ASTORIA EP buffer
 */
static void cy_service_e_p_dma_write_request(
			cy_as_c110_dev_kernel *dev_p, uint8_t ep)
{
	uint16_t  addr;
	uint16_t v  = 0, i = 0;
	uint32_t  size;
	uint16_t	*dptr;
	register void     *write_addr ;
    register uint16_t a,b,c,d ;
   
	cy_as_hal_device_tag tag = (cy_as_hal_device_tag)dev_p ;
	/*
	 * note: size here its the size of the dma transfer could be
	 * anything > 0 && < P_PORT packet size
	 */
	size = end_points[ep].dma_xfer_sz ;
	dptr = end_points[ep].data_p ;

	write_addr = (void *) (dev_p->m_vma_addr_base + CYAS_DEV_CALC_EP_ADDR(ep)) ;

	/*
	 * perform the soft DMA transfer, soft in this case
	 */
	if (size){
		/*
		 * Now, write the data to the device
		 */
		for(i = size/8 ; i > 0 ; i--) {
	            a = *dptr++ ;
	            b = *dptr++ ;
	            c = *dptr++ ;
	            d = *dptr++ ;
		    writew (a, write_addr) ;
		    writew (b, write_addr) ;
		    writew (c, write_addr) ;
		    writew (d, write_addr) ;
		}
	
		switch ((size & 7)/2) {
		case 3:
		    writew (*dptr, write_addr) ;
		    dptr++ ;
		case 2:
		    writew (*dptr, write_addr) ;
		    dptr++ ;
		case 1:
		    writew (*dptr, write_addr) ;
		    dptr++ ;
	            break ;
		}
	
		if (size & 1) {
		    uint16_t v = *((uint8_t *)dptr) ;
		    writew (v, write_addr);
		}
	}

	end_points[ep].seg_xfer_cnt += size;
	end_points[ep].req_xfer_cnt += size;
	/*
	 * pre-advance data pointer
	 * (if it's outside sg list it will be reset anyway)
	 */
	end_points[ep].data_p += size;

	/*
	 * now clear DMAVAL bit to indicate we are done
	 * transferring data and that the data can now be
	 * sent via USB to the USB host, sent to storage,
	 * or used internally.
	 */

	addr = CY_AS_MEM_P0_EP2_DMA_REG + ep - 2 ;
	cy_as_hal_write_register(tag, addr, size) ;

	/*
	 * finally, tell the USB subsystem that the
	 * data is gone and we can accept the
	 * next request if one exists.
	 */
	if (prep_for_next_xfer(tag, ep)) {
		/*
		 * There is more data to go. Re-init the WestBridge DMA side
		 */
		v = end_points[ep].dma_xfer_sz |
			CY_AS_MEM_P0_E_pn_DMA_REG_DMAVAL ;
		cy_as_hal_write_register(tag, addr, v) ;
		udelay(2);
	} else {

	   end_points[ep].pending	  = cy_false ;
	   end_points[ep].type		 = cy_as_hal_none ;
	   end_points[ep].buffer_valid = cy_false ;

		/*
		 * notify the API that we are done with rq on this EP
		 */
		if (callback) {
			/*
			 * this callback will wake up the process that might be
			 * sleeping on the EP which data is being transferred
			 */
			callback(tag, ep,
					end_points[ep].req_xfer_cnt,
					CY_AS_ERROR_SUCCESS);
		}
	}
}

/*
 * HANDLE DRQINT from Astoria (called in AS_Intr context
 */
static void cy_handle_d_r_q_interrupt(cy_as_c110_dev_kernel *dev_p, uint16_t read_val)
{
	uint16_t v ;
	static uint8_t service_ep = 2 ;

	/*
	 * We've got DRQ INT, read DRQ STATUS Register */
	v = cy_as_hal_read_register((cy_as_hal_device_tag)dev_p,CY_AS_MEM_P0_DRQ) ;

	if (v == 0) {
#ifndef WESTBRIDGE_NDEBUG
		printk("stray drq interrupt detected: read_val = 0x%x", read_val) ;
		printk("P0_DRQ=0x%x", cy_as_hal_read_register(
				(cy_as_hal_device_tag)dev_p,CY_AS_MEM_P0_DRQ));
		printk("P0_INT=0x%x\n", cy_as_hal_read_register(
				(cy_as_hal_device_tag)dev_p,CY_AS_MEM_P0_INTR_REG));
#endif
		return;
	}

	/*
	 * Now, pick a given DMA request to handle, for now, we just
	 * go round robin.  Each bit position in the service_mask
	 * represents an endpoint from EP2 to EP15.  We rotate through
	 * each of the endpoints to find one that needs to be serviced.
	 */
	while ((v & (1 << service_ep)) == 0) {

		if (service_ep == 15)
			service_ep = 2 ;
		else
			service_ep++ ;
	}

	if (end_points[service_ep].type == cy_as_hal_write) {
		/*
		 * handle DMA WRITE REQUEST: app_cpu will
		 * write data into astoria EP buffer
		 */
		cy_service_e_p_dma_write_request(dev_p, service_ep) ;
	} else if (end_points[service_ep].type == cy_as_hal_read) {
		/*
		 * handle DMA READ REQUEST: cpu will
		 * read EP buffer from Astoria
		 */
		cy_service_e_p_dma_read_request(dev_p, service_ep) ;
	}
#ifndef WESTBRIDGE_NDEBUG
	else
		cy_as_hal_print_message("cyashalc110:interrupt,"
					" w/o pending DMA job,"
					"-check DRQ_MASK logic\n") ;
#endif

	/*
	 * Now bump the EP ahead, so other endpoints get
	 * a shot before the one we just serviced
	 */
	if (end_points[service_ep].type == cy_as_hal_none) {
		if (service_ep == 15)
			service_ep = 2 ;
		else
			service_ep++ ;
	}

}

void cy_as_hal_dma_cancel_request(cy_as_hal_device_tag tag, uint8_t ep)
{
#ifdef DBGPRN_ENABLED
	DBGPRN("cy_as_hal_dma_cancel_request on ep:%d", ep);
#endif
	if (end_points[ep].pending)
		cy_as_hal_write_register(tag, CY_AS_MEM_P0_EP2_DMA_REG + ep - 2, 0);

	end_points[ep].buffer_valid = cy_false ;
	end_points[ep].type = cy_as_hal_none;
}

/*
 * enables/disables SG list assisted DMA xfers for the given EP
 * sg_list assisted XFERS can use physical addresses of mem pages in case if the
 * xfer is performed by a h/w DMA controller rather then the CPU on P port
 */
void cy_as_hal_set_ep_dma_mode(uint8_t ep, bool sg_xfer_enabled)
{
	end_points[ep].sg_list_enabled = sg_xfer_enabled;
#ifdef DBGPRN_ENABLED
	DBGPRN("<1> EP:%d sg_list assisted DMA mode set to = %d\n",
			ep, end_points[ep].sg_list_enabled);
#endif
}
EXPORT_SYMBOL(cy_as_hal_set_ep_dma_mode);

/*
 * This function must be defined to transfer a block of data to
 * the WestBridge device.  This function can use the burst write
 * (DMA) capabilities of WestBridge to do this, or it can just copy
 * the data using writes.
 */
void cy_as_hal_dma_setup_write(cy_as_hal_device_tag tag,
						uint8_t ep, void *buf,
						uint32_t size, uint16_t maxsize)
{
	uint32_t addr = 0 ;
	uint16_t v  = 0;

	/*
	 * Note: "size" is the actual request size
	 * "maxsize" - is the P port fragment size
	 * No EP0 or EP1 traffic should get here
	 */
	cy_as_hal_assert(ep != 0 && ep != 1) ;

	/*
	 * If this asserts, we have an ordering problem.  Another DMA request
	 * is coming down before the previous one has completed.
	 */
	cy_as_hal_assert(end_points[ep].buffer_valid == cy_false) ;
	end_points[ep].buffer_valid = cy_true ;
	end_points[ep].type = cy_as_hal_write ;
	end_points[ep].pending = cy_true;

	/*
	 * total length of the request
	 */
	end_points[ep].req_length = size;

	if (size >= maxsize) {
		/*
		 * set xfer size for very 1st DMA xfer operation
		 * port max packet size ( typically 512 or 1024)
		 */
		end_points[ep].dma_xfer_sz = maxsize;
	} else {
		/*
		 * smaller xfers for non-storage EPs
		 */
		end_points[ep].dma_xfer_sz = size;
	}

	/*
	 * check the EP transfer mode uses sg_list rather then a memory buffer
	 * block devices pass it to the HAL, so the hAL could get to the real
	 * physical address for each segment and set up a DMA controller
	 * hardware ( if there is one)
	 */
	if (end_points[ep].sg_list_enabled) {
		/*
		 * buf -  pointer to the SG list
		 * data_p - data pointer to the 1st DMA segment
		 * seg_xfer_cnt - keeps track of N of bytes sent in current
		 *		sg_list segment
		 * req_xfer_cnt - keeps track of the total N of bytes
		 *		transferred for the request
		 */
		end_points[ep].sg_p = buf;
		end_points[ep].data_p = sg_virt(end_points[ep].sg_p);
		end_points[ep].seg_xfer_cnt = 0 ;
		end_points[ep].req_xfer_cnt = 0;

#ifdef DBGPRN_DMA_SETUP_WR
		DBGPRN("cyasc110hal:%s: EP:%d, buf:%p, buf_va:%p, req_sz:%d, maxsz:%d\n",
				__func__, ep,	buf, end_points[ep].data_p, size,	maxsize);
#endif

	} else {
		/*
		 * setup XFER for non sg_list assisted EPs
		 */

		#ifdef DBGPRN_DMA_SETUP_WR
			DBGPRN("<1>%s non storage or sz < 512:EP:%d, sz:%d\n", __func__, ep, size);
		#endif

		end_points[ep].sg_p = NULL;

		/*
		 * must be a VMA of a membuf in kernel space
		 */
		end_points[ep].data_p = buf;

		/*
		 * will keep track No of bytes xferred for the request
		 */
		end_points[ep].req_xfer_cnt = 0;
	}

	/*
	 * Tell WB we are ready to send data on the given endpoint
	 */
	v = (end_points[ep].dma_xfer_sz & CY_AS_MEM_P0_E_pn_DMA_REG_COUNT_MASK) | CY_AS_MEM_P0_E_pn_DMA_REG_DMAVAL ;

	addr = CY_AS_MEM_P0_EP2_DMA_REG + ep - 2 ;

	cy_as_hal_write_register(tag, addr, v) ;
}

/*
 * This function must be defined to transfer a block of data from
 * the WestBridge device.  This function can use the burst read
 * (DMA) capabilities of WestBridge to do this, or it can just
 * copy the data using reads.
 */
void cy_as_hal_dma_setup_read(cy_as_hal_device_tag tag,
					uint8_t ep, void *buf,
					uint32_t size, uint16_t maxsize)
{
	uint32_t addr ;
	uint16_t v ;

	/*
	 * Note: "size" is the actual request size
	 * "maxsize" - is the P port fragment size
	 * No EP0 or EP1 traffic should get here
	 */
	cy_as_hal_assert(ep != 0 && ep != 1) ;

	/*
	 * If this asserts, we have an ordering problem.
	 * Another DMA request is coming down before the
	 * previous one has completed. we should not get
	 * new requests if current is still in process
	 */

	cy_as_hal_assert(end_points[ep].buffer_valid == cy_false);

	end_points[ep].buffer_valid = cy_true ;
	end_points[ep].type = cy_as_hal_read ;
	end_points[ep].pending = cy_true;
	end_points[ep].req_xfer_cnt = 0;
	end_points[ep].req_length = size;

	if (size >= maxsize) {
		/*
		 * set xfer size for very 1st DMA xfer operation
		 * port max packet size ( typically 512 or 1024)
		 */
		end_points[ep].dma_xfer_sz = maxsize;
	} else {
		/*
		 * so that we could handle small xfers on in case
		 * of non-storage EPs
		 */
		end_points[ep].dma_xfer_sz = size;
	}

	addr = CY_AS_MEM_P0_EP2_DMA_REG + ep - 2 ;

	if (end_points[ep].sg_list_enabled) {
		/*
		 * Handle sg-list assisted EPs
		 * seg_xfer_cnt - keeps track of N of sent packets
		 * buf - pointer to the SG list
		 * data_p - data pointer for the 1st DMA segment
		 */
		end_points[ep].seg_xfer_cnt = 0 ;
		end_points[ep].sg_p = buf;
		end_points[ep].data_p = sg_virt(end_points[ep].sg_p);

		#ifdef DBGPRN_DMA_SETUP_RD
		DBGPRN("cyasc110hal:DMA_setup_read sg_list EP:%d, buf:%p, buf_va:%p, req_sz:%d, maxsz:%d\n",
				ep,	buf, end_points[ep].data_p, size, maxsize);
		#endif
		v = (end_points[ep].dma_xfer_sz & CY_AS_MEM_P0_E_pn_DMA_REG_COUNT_MASK) |	CY_AS_MEM_P0_E_pn_DMA_REG_DMAVAL ;
		cy_as_hal_write_register(tag, addr, v);
	} else {
		/*
		 * Non sg list EP passed  void *buf rather then scatterlist *sg
		 */
		#ifdef DBGPRN_DMA_SETUP_RD
			DBGPRN("%s:non-sg_list EP:%d, RQ_sz:%d, maxsz:%d\n",	__func__, ep, size,  maxsize);
		#endif

		end_points[ep].sg_p = NULL;

		/*
		 * must be a VMA of a membuf in kernel space
		 */
		end_points[ep].data_p = buf;

		/*
		 * Program the EP DMA register for Storage endpoints only.
		 */
		if (is_storage_e_p(ep)) {
			v = (end_points[ep].dma_xfer_sz &	CY_AS_MEM_P0_E_pn_DMA_REG_COUNT_MASK) |	CY_AS_MEM_P0_E_pn_DMA_REG_DMAVAL ;
			cy_as_hal_write_register(tag, addr, v);
		}
	}
}

/*
 * This function must be defined to allow the WB API to
 * register a callback function that is called when a
 * DMA transfer is complete.
 */
void cy_as_hal_dma_register_callback(cy_as_hal_device_tag tag,
					cy_as_hal_dma_complete_callback cb)
{
	DBGPRN("<1>\n%s: WB API has registered a dma_complete callback:%x\n",	__func__, (uint32_t)cb);
	callback = cb ;
}

/*
 * This function must be defined to return the maximum size of
 * DMA request that can be handled on the given endpoint.  The
 * return value should be the maximum size in bytes that the DMA
 * module can handle.
 */
uint32_t cy_as_hal_dma_max_request_size(cy_as_hal_device_tag tag,
					cy_as_end_point_number_t ep)
{
	/*
	 * Storage reads and writes are always done in 512 byte blocks.
	 * So, we do the count handling within the HAL, and save on
	 * some of the data transfer delay.
	 */
	if ((ep == CYASSTORAGE_READ_EP_NUM) ||
	(ep == CYASSTORAGE_WRITE_EP_NUM)) {
		/* max DMA request size HAL can handle by itself */
		return CYASSTORAGE_MAX_XFER_SIZE;
	} else {
	/*
	 * For the USB - Processor endpoints, the maximum transfer
	 * size depends on the speed of USB operation. So, we use
	 * the following constant to indicate to the API that
	 * splitting of the data into chunks less that or equal to
	 * the max transfer size should be handled internally.
	 */

		/* DEFINED AS 0xffffffff in cyasdma.h */
		return CY_AS_DMA_MAX_SIZE_HW_SIZE;
	}
}

/*
 * This function must be defined to set the state of the WAKEUP pin
 * on the WestBridge device.  Generally this is done via a GPIO of
 * some type.
 */
cy_bool cy_as_hal_set_wakeup_pin(cy_as_hal_device_tag tag, cy_bool state)
{
    if(state) {
     	gpio_set_value(WB_WAKEUP, 1);
    } else {
    	gpio_set_value(WB_WAKEUP, 0);
    }
    
    return cy_true;
}

void cy_as_hal_pll_lock_loss_handler(cy_as_hal_device_tag tag)
{
	cy_as_hal_print_message("error: astoria PLL lock is lost\n") ;
	cy_as_hal_print_message("please check the input voltage levels");
	cy_as_hal_print_message("and clock, and restart the system\n") ;
}

/*
 * Below are the functions that must be defined to provide the basic
 * operating system services required by the API.
 */

/*
 * This function is required by the API to allocate memory.
 * This function is expected to work exactly like malloc().
 */
void *cy_as_hal_alloc(uint32_t cnt)
{
	void *ret_p ;

	ret_p = kmalloc(cnt, GFP_ATOMIC) ;
	return ret_p ;
}

/*
 * This function is required by the API to free memory allocated
 * with CyAsHalAlloc().  This function is'expected to work exacly
 * like free().
 */
void cy_as_hal_free(void *mem_p)
{
	kfree(mem_p) ;
}

/*
 * Allocator that can be used in interrupt context.
 * We have to ensure that the kmalloc call does not
 * sleep in this case.
 */
void *cy_as_hal_c_b_alloc(uint32_t cnt)
{
	void *ret_p ;

	ret_p = kmalloc(cnt, GFP_ATOMIC) ;
	return ret_p ;
}

/*
 * This function is required to set a block of memory to a
 * specific value.  This function is expected to work exactly
 * like memset()
 */
void cy_as_hal_mem_set(void *ptr, uint8_t value, uint32_t cnt)
{
	memset(ptr, value, cnt) ;
}

/*
 * This function is expected to create a sleep channel.
 * The data structure that represents the sleep channel object
 * sleep channel (which is Linux "wait_queue_head_t wq" for this paticular HAL)
 * passed as a pointer, and allpocated by the caller
 * (typically as a local var on the stack) "Create" word should read as
 * "SleepOn", this func doesn't actually create anything
 */
cy_bool cy_as_hal_create_sleep_channel(cy_as_hal_sleep_channel *channel)
{
	init_waitqueue_head(&channel->wq) ;
	return cy_true ;
}

/*
 * for this particular HAL it doesn't actually destroy anything
 * since no actual sleep object is created in CreateSleepChannel()
 * sleep channel is given by the pointer in the argument.
 */
cy_bool cy_as_hal_destroy_sleep_channel(cy_as_hal_sleep_channel *channel)
{
	return cy_true ;
}

/*
 * platform specific wakeable Sleep implementation
 */
cy_bool cy_as_hal_sleep_on(cy_as_hal_sleep_channel *channel, uint32_t ms)
{
	wait_event_interruptible_timeout(channel->wq, 0, ((ms * HZ)/1000)) ;
	return cy_true ;
}

/*
 * wakes up the process waiting on the CHANNEL
 */
cy_bool cy_as_hal_wake(cy_as_hal_sleep_channel *channel)
{
	wake_up_interruptible_all(&channel->wq);
	return cy_true ;
}

uint32_t cy_as_hal_disable_interrupts()
{
	if (0 == intr__enable)
	{
		//cy_as_hal_write_register( m_c110_list_p, CY_AS_MEM_P0_INT_MASK_REG, 0 );
	}
	intr__enable++ ;
	return 0 ;
}

void cy_as_hal_enable_interrupts(uint32_t val)
{
	intr__enable-- ;
	if (0 == intr__enable)
	{
		//cy_as_hal_write_register( m_c110_list_p, CY_AS_MEM_P0_INT_MASK_REG, CYAS_INT_MASK | CYAS_DRQ_MASK );
	}
}

/*
 * Sleep atleast 150ns, cpu dependent
 */
void cy_as_hal_sleep150(void)
{
	uint32_t i, j;

	j = 0;
	for (i = 0; i < 1000; i++)
		j += (~i);
}

void cy_as_hal_sleep(uint32_t ms)
{
	cy_as_hal_sleep_channel channel;

	cy_as_hal_create_sleep_channel(&channel) ;
	cy_as_hal_sleep_on(&channel, ms) ;
	cy_as_hal_destroy_sleep_channel(&channel) ;
}

cy_bool cy_as_hal_is_polling()
{
	return cy_false;
}

void cy_as_hal_c_b_free(void *ptr)
{
	cy_as_hal_free(ptr);
}

/*
 * suppose to reinstate the astoria registers
 * that may be clobbered in sleep mode
 */
void cy_as_hal_init_dev_registers(cy_as_hal_device_tag tag,
					cy_bool is_standby_wakeup)
{
	/* specific to SPI, no implementation required */
	(void) tag;
	(void) is_standby_wakeup;
}

void cy_as_hal_read_regs_before_standby(cy_as_hal_device_tag tag)
{
	/* specific to SPI, no implementation required */
	(void) tag;
}

cy_bool cy_as_hal_sync_device_clocks(cy_as_hal_device_tag tag)
{
	/*
	 * we are in asynchronous mode. so no need to handle this
	 */
	return true;
}


void cy_as_hal_dump_reg(cy_as_hal_device_tag tag)
{
	u16		data16;
	int 	i;
	int	retval = 0;
#if 0
	printk(KERN_ERR "=======================================\n");
	printk(KERN_ERR "========   Astoria REG Dump      =========\n");
	for ( i=0 ; i<256 ; i++ )
	{
		data16 = cy_as_hal_read_register(tag, i);
		printk(KERN_ERR "%4.4x ", data16);
		if( i%8 == 7 )
			printk(KERN_ERR "\n");
	}
#endif
	printk(KERN_ERR "=======================================\n\n");

	printk(KERN_ERR "========   Astoria REG Test      =========\n");
	
	cy_as_hal_write_register(tag, CY_AS_MEM_MCU_MAILBOX1, 0xAAAA);
	cy_as_hal_write_register(tag, CY_AS_MEM_MCU_MAILBOX2, 0x5555);
	cy_as_hal_write_register(tag, CY_AS_MEM_MCU_MAILBOX3, 0xB4C3);

	data16 = cy_as_hal_read_register(tag, CY_AS_MEM_MCU_MAILBOX1);
	if( data16 != 0xAAAA)
	{
		printk(KERN_ERR "REG Test Error in CY_AS_MEM_MCU_MAILBOX1 - %4.4x\n", data16);
		retval = 1; 
	}
	data16 = cy_as_hal_read_register(tag, CY_AS_MEM_MCU_MAILBOX2);
	if( data16 != 0x5555)
	{
		printk(KERN_ERR "REG Test Error in CY_AS_MEM_MCU_MAILBOX2 - %4.4x\n", data16);
		retval = 1;
	}
	data16 = cy_as_hal_read_register(tag, CY_AS_MEM_MCU_MAILBOX3);
	if( data16 != 0xB4C3)
	{	
		printk(KERN_ERR "REG Test Error in CY_AS_MEM_MCU_MAILBOX3 - %4.4x\n", data16);
		retval = 1;
	}

	if( retval)
		printk(KERN_ERR "REG Test fail !!!!!\n");
	else
		printk(KERN_ERR "REG Test success !!!!!\n");

	printk(KERN_ERR "=======================================\n\n");
}

/*
 * init C110 h/w resources
 */
int cy_as_hal_c110_cram_start(const char *pgm,
				cy_as_hal_device_tag *tag, cy_bool debug)
{
	cy_as_c110_dev_kernel *dev_p ;
	int i;
	u16 data16[4];
	uint32_t err = 0;
	/* No debug mode support through argument as of now */
	(void)debug;

	have_irq = false;

	/*
	 * Initialize the HAL level endpoint DMA data.
	 */
	for (i = 0 ; i < sizeof(end_points)/sizeof(end_points[0]) ; i++) {
		end_points[i].data_p = 0 ;
		end_points[i].pending = cy_false ;
		end_points[i].size = 0 ;	/* No debug mode support through argument as of now */
	(void)debug;
		
		end_points[i].type = cy_as_hal_none ;
		end_points[i].sg_list_enabled = cy_false;

		/*
		 * by default the DMA transfers to/from the E_ps don't
		 * use sg_list that implies that the upper devices like
		 * blockdevice have to enable it for the E_ps in their
		 * initialization code
		 */
	}

	/* allocate memory for c110 HAL*/
	dev_p = (cy_as_c110_dev_kernel *)cy_as_hal_alloc(sizeof(cy_as_c110_dev_kernel)) ;
	if (dev_p == 0) {
		cy_as_hal_print_message("out of memory allocating C110"
					"device structure\n") ;
		return 0 ;
	}

	dev_p->m_sig = CY_AS_C110_CRAM_HAL_SIG;

	/* initialize C110 hardware and StartC110Kernelall gpio pins */
	err = cy_as_hal_processor_hw_init(dev_p);
	if(err)
		goto bus_acc_error;

	/*
	 * Now perform a hard reset of the device to have
	 * the new settings take effect
	 */
	gpio_set_value(WB_WAKEUP, 1);

	/*
	 * do Astoria  h/w reset
	 */
	DBGPRN(KERN_INFO"-_-_pulse -> westbridge RST pin\n");

	/*
	 * NEGATIVE PULSE on RST pin
	 */
	gpio_set_value(WB_RESET,0);
	mdelay(1);
	gpio_set_value(WB_RESET,1);
	mdelay(50);


   /*
	*  NOTE: if you want to capture bus activity on the LA,
	*  don't use printks in between the activities you want to capture.
	*  prinks may take milliseconds, and the data of interest
	*  will fall outside the LA capture window/buffer
	*/
	cy_as_hal_dump_reg((cy_as_hal_device_tag)dev_p);

	data16[0] = cy_as_hal_read_register((cy_as_hal_device_tag)dev_p, CY_AS_MEM_CM_WB_CFG_ID);

	if ( (data16[0]&0xA100 != 0xA100) ||  (data16[0]&0xA200 != 0xA200))  {
		/*
		 * astoria device is not found
		 */
		printk(KERN_ERR "ERROR: astoria device is not found, "
			"CY_AS_MEM_CM_WB_CFG_ID %4.4x", data16[0]);
		goto bus_acc_error;
	}

	cy_as_hal_print_message(KERN_INFO" register access test:"
				"\n CY_AS_MEM_CM_WB_CFG_ID:%4.4x\n"
				"after cfg_wr:%4.4x\n\n",
				data16[0], data16[1]);

	dev_p->thread_flag = 1 ;
	spin_lock_init(&int_lock) ;
	dev_p->m_next_p = m_c110_list_p ;

	m_c110_list_p = dev_p ;
	*tag = dev_p;

	cy_as_hal_configure_interrupts((void *)dev_p);

	cy_as_hal_print_message(KERN_INFO"C110__hal started tag:%p, kernel HZ:%d\n", dev_p, HZ);

	/*
	 *make processor to storage endpoints SG assisted by default
	 */
	cy_as_hal_set_ep_dma_mode(4, true);
	cy_as_hal_set_ep_dma_mode(8, true);

	return 1 ;

	/*
	 * there's been a NAND bus access error or
	 * astoria device is not connected
	 */
bus_acc_error:
	/*
	 * at this point hal tag hasn't been set yet
	 * so the device will not call C110_stop
	 */
	cy_as_hal_c110_hardware_deinit(dev_p);
	cy_as_hal_free(dev_p) ;
	return 0;
}

int 	cy_as_hal_enable_irq(void)
{
	enable_irq(WB_CYAS_IRQ_INT);
	enable_irq(WB_SDCD_IRQ_INT);
	return 0;
}

int	cy_as_hal_disable_irq(void)
{
	disable_irq(WB_CYAS_IRQ_INT);
	disable_irq(WB_SDCD_IRQ_INT);
	return 0;
}

int	cy_as_hal_detect_SD(void)
{
	uint8_t f_det;
	f_det = gpio_get_value(WB_SDCD_INT);
	if( f_det ) // removed;
		return 0;
	//inserted 
	return 1;
}

int cy_as_hal_configure_sd_isr(void *dev_p, irq_handler_t isr_function)
{
	int result;
	int irq_pin  = WB_SDCD_INT;

	set_irq_type(WB_SDCD_IRQ_INT, IRQ_TYPE_EDGE_BOTH);

	/*
	 * for shared IRQS must provide non NULL device ptr
	 * othervise the int won't register
	 * */
	cy_as_hal_print_message("%s: set_irq_type\n", __func__);

	result = request_irq(WB_SDCD_IRQ_INT, (irq_handler_t)isr_function, IRQF_SHARED, "AST_CD#", dev_p);

	cy_as_hal_print_message("%s: request_irq - %d\n", __func__, result);

	if (result == 0) 
	{
		cy_as_hal_print_message(KERN_INFO"WB_SDCD_IRQ_INT C110_pin: %d assigned IRQ #%d \n",irq_pin, WB_SDCD_IRQ_INT );
	}
	else 
	{
		cy_as_hal_print_message("WB_SDCD_IRQ_INT: interrupt failed to register - %d\n", result);
		gpio_free(irq_pin);
		cy_as_hal_print_message("WB_SDCD_IRQ_INT: can't get assigned IRQ %i for INT#\n", WB_SDCD_IRQ_INT);
	}

	return result;
}

int cy_as_hal_free_sd_isr(void)
{
	free_irq( WB_SDCD_IRQ_INT, NULL );
	return 0;
}


#else
/*
 * Some compilers do not like empty C files, so if the C110 hal is not being
 * compiled, we compile this single function.  We do this so that for a
 * given target HAL there are not multiple sources for the HAL functions.
 */
void my_C110_kernel_hal_dummy_function(void)
{
}

#endif
