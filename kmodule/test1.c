#include <linux/interrupt.h>
//#include <linux/irq.h>
#include <linux/workqueue.h>

#define xstr(s) str(s)
#define str(s) #s

#define MODNAME kbdevstat
#define MODULE_NAME xstr(MODNAME)
#define KBD_IRQ 1

#define KBD_HSC_Q_NAME "kbdscwq"

static struct workqueue_struct *kbd_wq = NULL;

static atomic_long_t interrupt_count;

struct kbd_work {
    struct work_struct work;
    unsigned long scancode;
} kbd_work;

/* 
 * This will get called by the kernel as soon as it's safe
 * to do everything normally allowed by kernel modules.
 */
static void kbd_handle_scancode(struct work_struct *ws)
{
    struct kbd_work *kbdw = container_of(ws, struct kbd_work, work);
    unsigned long scancode = kbdw->scancode;

    printk(KERN_INFO MODULE_NAME ": Scan Code 0x%lx %s.\n",
        scancode & 0x7F,
        scancode & 0x80 ? "Released" : "Pressed");
}

/* 
 * This function services keyboard interrupts. It reads the relevant
 * information from the keyboard and then puts the non time critical
 * part into the work queue. This will be run when the kernel considers it safe.
 */
irqreturn_t irq_handler(int irq, void *dev_id)
{
    unsigned char status;
    unsigned char scancode;

    atomic_long_inc(&interrupt_count);

    /* 
     * Read keyboard status and scancode
     */
    status = inb(0x64);
    scancode = inb(0x60);

    INIT_WORK(&kbd_work.work, kbd_handle_scancode);
    kbd_work.scancode = scancode;

    queue_work(kbd_wq, &kbd_work.work);

    return IRQ_HANDLED;
}

static
int __init kbdevstat_init(void) {

    int irq_request_result = -1;

    printk(KERN_INFO MODULE_NAME ": Initializing\n");

    atomic_long_set(&interrupt_count, 0);

    kbd_wq = create_singlethread_workqueue(KBD_HSC_Q_NAME);

    if (kbd_wq == NULL) {
        printk(KERN_ALERT MODULE_NAME ": can't create workqueue");
        return -1;
    }

    irq_request_result =
        request_irq(KBD_IRQ,
                    irq_handler,
                    /*flags*/  IRQF_SHARED,
                    /*name*/   "test_keyboard_irq_handler",
                    /*dev_id*/ (void *)(irq_handler));

    return irq_request_result;
}

static
void __exit kbdevstat_exit(void) {

    printk(KERN_INFO MODULE_NAME ": Exiting after %lu interrupts\n",
        atomic_long_read(&interrupt_count));

    free_irq(KBD_IRQ, /*dev_id*/irq_handler);

    if (kbd_wq) {
        destroy_workqueue(kbd_wq);
    }
}

