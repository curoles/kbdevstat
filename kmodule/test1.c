#include <linux/interrupt.h>

#define xstr(s) str(s)
#define str(s) #s

#define MODNAME kbdevstat
#define MODULE_NAME str(MODNAME)
#define KBD_IRQ 1

irqreturn_t irq_handler(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}

static
int __init kbdevstat_init(void) {

    int irq_request_result = -1;

    printk(KERN_INFO MODULE_NAME " Initializing \n");

    irq_request_result =
        request_irq(KBD_IRQ,
                    irq_handler,
                    IRQF_SHARED, "test_keyboard_irq_handler",
                    (void *)(irq_handler));

    return irq_request_result;
}

static
void __exit kbdevstat_exit(void) {
    printk(KERN_INFO MODULE_NAME " Exiting\n");
}

