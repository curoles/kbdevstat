#include <linux/interrupt.h>
//#include <linux/irq.h>
#include <linux/workqueue.h>

#define xstr(s) str(s)
#define str(s) #s

#define MODNAME kbdevstat
#define MODULE_NAME xstr(MODNAME)
#define KBD_IRQ 1

#define KBD_HSC_Q_NAME "kbdscwq"

static struct workqueue_struct *kbd_handle_scancode_workqueue;
//static struct work_struct kbd_handle_scancode_task;
//static DECLARE_WORK(mykmod_work, mykmod_work_handler);
//static unsigned char scancode;
// alternative to using data field is something like:
// https://github.com/fervagar/kernel_modules/blob/master/workQueue.c

/* 
 * This will get called by the kernel as soon as it's safe
 * to do everything normally allowed by kernel modules.
 */
static void kbd_handle_scancode(struct work_struct *ws)
{
    unsigned long scancode = atomic_long_read(&ws->data);

    printk(KERN_INFO MODULE_NAME ": Scan Code %x %s.\n",
        scancode & 0x7F,
        scancode & 0x80 ? "Released" : "Pressed");
}

static DECLARE_WORK(kbd_handle_scancode_task, kbd_handle_scancode);

/* 
 * This function services keyboard interrupts. It reads the relevant
 * information from the keyboard and then puts the non time critical
 * part into the work queue. This will be run when the kernel considers it safe.
 */
irqreturn_t irq_handler(int irq, void *dev_id)
{
    unsigned char status;
    unsigned char scancode;

    /* 
     * Read keyboard status and scancode
     */
    status = 0;//inb(0x64);
    scancode = 55;//inb(0x60);

    atomic_long_set(&(kbd_handle_scancode_task.data), scancode);

    queue_work(kbd_handle_scancode_workqueue, &kbd_handle_scancode_task);

    return IRQ_HANDLED;
}

static
int __init kbdevstat_init(void) {

    int irq_request_result = -1;

    printk(KERN_INFO MODULE_NAME ": Initializing\n");

    kbd_handle_scancode_workqueue = create_singlethread_workqueue(KBD_HSC_Q_NAME);
    INIT_WORK(&kbd_handle_scancode_task, kbd_handle_scancode);

    //if (!can_request_irq(KBD_IRQ, IRQF_SHARED)) {
    //    printk(KERN_ALERT MODULE_NAME " Can't request IRQ " str(KBD_IRQ) "\n");
    //    return 1;
    //}

    // TODO bool irq_percpu_is_enabled(unsigned int irq)
    // TODO Do I care about request_threaded_irq()?

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
    printk(KERN_INFO MODULE_NAME ": Exiting\n");

    free_irq(KBD_IRQ, /*dev_id*/irq_handler);

    flush_work(&kbd_handle_scancode_task);

    destroy_workqueue(kbd_handle_scancode_workqueue);
}

