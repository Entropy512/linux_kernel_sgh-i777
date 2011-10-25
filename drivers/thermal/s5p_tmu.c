#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h> 
#include <linux/delay.h>
#include <linux/platform_device.h> 
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/irq.h>

#include <asm/irq.h>

#include <mach/regs-tmu.h>
#include <plat/s5p-tmu.h>
#include <plat/watchdog-reset.h>

#define TRIGGER_LEV0 200
#define	TRIGGER_LEV1 200	
#define	TRIGGER_LEV2 0xFF	
#define	TRIGGER_LEV3 0xFF

#define VREF_SLOPE		0x07000F02
#define TMU_EN			0x1

static struct resource *s5p_tmu_mem;

static int trigger0no = NO_IRQ;
static int trigger1no = NO_IRQ;

static void tmu_initialize(struct platform_device *pdev)
{
	struct tmu_platform_device *tmu_dev = platform_get_drvdata(pdev);
	unsigned int con, en;

	/* Need to initail regsiter setting after getting parameter info */
	
	/* end */

	tmu_dev->dvfs_flag = 0;

	en = (readb(tmu_dev->tmu_base + TMU_STATUS) & 0x1);

	while(!en)
		printk(KERN_INFO "TMU is busy..!! wating......\n");

	/* [28:23] vref [11:8] slope - Tunning parameter
	*  	[0] TMU enable */
	con = (VREF_SLOPE | TMU_EN);
	
	mdelay(3);
	writel(con, tmu_dev->tmu_base + TMU_CON0); /* TMU core enable */
	mdelay(5);
	printk(KERN_INFO "con raw %x   reg value %x\n", con,readl(tmu_dev->tmu_base + TMU_CON0));
}	

static void tmu_compensation(struct platform_device *pdev)
{
	struct tmu_platform_device *tmu_dev = platform_get_drvdata(pdev);
	unsigned int te1,te2, te_temp;
	unsigned int t_new, cur_temp;

	cur_temp = readb(tmu_dev->tmu_base + CURRENT_TEMP);
	te_temp = readb(tmu_dev->tmu_base + TRIMINFO);
	te1 = te_temp & TRIM_TEMP_MASK;
	te2 = ((te_temp & (TRIM_TEMP_MASK <<8)) >> 8);
	
	if(tmu_dev->data.mode)
		t_new = (tmu_dev->data.thr_temp - 25) * (te2 - te1) / ((85 - 25) + te1);
	else
		t_new = tmu_dev->data.thr_temp + (te1 - 25);

	writeb(t_new, tmu_dev->tmu_base + THRESHOLD_TEMP);

	writeb(TRIGGER_LEV0, tmu_dev->tmu_base + TRG_LEV0);
	writeb(TRIGGER_LEV1, tmu_dev->tmu_base + TRG_LEV1);
	writeb(TRIGGER_LEV2, tmu_dev->tmu_base + TRG_LEV2);
	writeb(TRIGGER_LEV3, tmu_dev->tmu_base + TRG_LEV3);

	printk(KERN_INFO "TMU is initialized !!								\
		Currnent temp : %dc  Threshold temp: %dc						\
		Thortting temp: %dc  Trimming temp : %dc						\
	T1            : %dc  T2            : %dc\n",					\
		cur_temp, t_new, t_new + readb(tmu_dev->tmu_base + TRG_LEV0), 	\
		t_new + readb(tmu_dev->tmu_base + TRG_LEV1), 					\
		tmu_dev->data.t1, tmu_dev->data.t2);

	writel(0x1111, tmu_dev->tmu_base + INTSTAT); 

	mdelay(5);
	/*LEV0 LEV1 interrupt enable */
	writel(INTEN0, tmu_dev->tmu_base + INTEN);
		
}

static irqreturn_t s5p_tmu_throtting_irq(int irq, void *id)
{
	struct	tmu_platform_device *dev = id;

	printk(KERN_INFO "Throtting interrupt occured!!!!\n");
	writel(INTCLEAR0, dev->tmu_base + INTCLEAR);
	dev->dvfs_flag=1;

	disable_irq_nosync(irq);

	return IRQ_HANDLED;
}

static irqreturn_t s5p_tmu_trimming_irq(int irq, void *id)
{
	struct tmu_platform_device *dev = id;

	printk(KERN_INFO "Trimming interrupt occured!!!!\n");
	writel(INTCLEAR1, dev->tmu_base + INTCLEAR);

	disable_irq_nosync(irq);

	mdelay(100);

	/* Temperature is higher than T2, Start watchdog */
	arch_wdt_reset();

	return IRQ_HANDLED;
}

static int __devinit s5p_tmu_probe(struct platform_device *pdev)
{
	struct tmu_platform_device *tmu_dev = platform_get_drvdata(pdev);
	struct resource *res;
	int	ret;

	pr_debug("%s: probe=%p\n", __func__, pdev);

	trigger0no = platform_get_irq(pdev, 0);
	if (trigger0no < 0) {
		dev_err(&pdev->dev, "no irq for thermal throtting \n");
		return -ENOENT;
	}
								    
	trigger1no = platform_get_irq(pdev, 1);
	if (trigger1no < 0) {
	dev_err(&pdev->dev, "no irq for thermal tripping\n");
	return -ENOENT;
	}

	printk("Thermal INT info: Throtting  irq_no %d, Tripping irq_no %d\n",
			 trigger0no, trigger1no);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
	dev_err(&pdev->dev, "failed to get memory region resource\n");
	return -ENOENT;
    }

	s5p_tmu_mem = request_mem_region(res->start,
					res->end-res->start+1,
					pdev->name);
	if (s5p_tmu_mem == NULL) {
		dev_err(&pdev->dev, "failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_nores;
	}
	
	tmu_dev->tmu_base = ioremap(res->start, (res->end - res->start) + 1);
	if (tmu_dev->tmu_base == NULL) {
		dev_err(&pdev->dev, "failed ioremap()\n");
		ret = -EINVAL;
		goto err_nomap;
	}

	ret = request_irq(trigger0no, s5p_tmu_throtting_irq,
			IRQF_DISABLED,  "s5p-tmu throtting", tmu_dev);
	if (ret) {
       dev_err(&pdev->dev, "IRQ%d error %d\n", trigger0no, ret);
		goto err_throtting;
	}

	ret = request_irq(trigger1no, s5p_tmu_trimming_irq,
			IRQF_DISABLED,  "s5p-tmu trimming", tmu_dev);

	if (ret) {
       dev_err(&pdev->dev, "IRQ%d error %d\n", trigger1no, ret);
		goto err_trimming;
	}

	tmu_initialize(pdev);
	tmu_compensation(pdev);

	return 0;

err_nomap:
    release_resource(s5p_tmu_mem);

err_throtting:
	free_irq(trigger0no, tmu_dev);

err_trimming:
	free_irq(trigger1no, tmu_dev);

err_nores:
	return ret;
}

#ifdef CONFIG_PM
static int s5p_tmu_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int s5p_tmu_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define s5p_tmu_suspend	NULL
#define s5p_tmu_resume	NULL
#endif
 
static struct platform_driver s5p_tmu_driver = {
	.probe		= s5p_tmu_probe,
	.suspend	= s5p_tmu_suspend,	
	.resume		= s5p_tmu_resume,
	.driver		= {
		.name	=	"s5p-tmu",
		.owner	=	THIS_MODULE,
		},
};

static int __init s5p_tmu_driver_init(void)
{
	printk(KERN_INFO "TMU driver is loaded....!\n");
	return platform_driver_register(&s5p_tmu_driver);
}

static void __exit s5p_tmu_driver_exit(void)
{       
	platform_driver_unregister(&s5p_tmu_driver);
}

module_init(s5p_tmu_driver_init);
module_exit(s5p_tmu_driver_exit);

