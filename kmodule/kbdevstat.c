/** @file
 *  @brief  Linux Kernel module that collects keyboard and mouse events.
 *  @author Igor Lesik 2018
 *
 *
 *  References:
 *  http://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html
 *  https://lwn.net/Kernel/LDD3/
 */
//#define DEBUG  // see https://www.kernel.org/doc/local/pr_debug.txt

#include <linux/init.h>   // __init and __exit macros
#include <linux/module.h>

#include <linux/interrupt.h> // to observe IRQ1 line from PS2 keyboard 
#include <linux/workqueue.h> // to call bottom-halfs

#include "kbdata.h"

// Driver Information
#define KBDEVSTAT_VERSION  "1.0.0"
#define KBDEVSTAT_AUTHOR   "Igor Lesik"
#define KBDEVSTAT_DESC     "Keyboard Events Statistics module"
#define KBDEVSTAT_LICENSE  "GPL"

// Kernel Module Information
MODULE_VERSION    (KBDEVSTAT_VERSION);
MODULE_AUTHOR     (KBDEVSTAT_AUTHOR);
MODULE_DESCRIPTION(KBDEVSTAT_DESC);
MODULE_LICENSE    (KBDEVSTAT_LICENSE);

/// Stringizing macros.
/// See https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html.
#define xstr(s) str(s)
#define str(s) #s

#define MODNAME kbdevstat
#define MODULE_NAME xstr(MODNAME)

#define KBD_IRQ 1
#define MOUSE_IRQ 12

#define KBDEVSTAT_WQ_NAME "kbdevstat_wq"

/// Workqueue do move processing code out from interrupt handlers.
static struct workqueue_struct *kbd_wq = NULL;


/// Structure that transfers data from keyboard interrupt handler to bottom-half. 
struct kbd_work {
    struct work_struct work;
    unsigned long scancode;
};

/// Static variable which is an instance of kbd_work.  
struct kbd_work kbd_work;

/** Processes keyboard interrupt information, bottom-half.
 *
 *  This will get called by the kernel as soon as it's safe
 *  to do everything normally allowed by kernel modules.
 */
static void kbd_handle_scancode(struct work_struct *ws)
{
    struct kbd_work *kbdw = container_of(ws, struct kbd_work, work);
    unsigned long scancode = kbdw->scancode;

    pr_debug(KERN_INFO MODULE_NAME ": Scan Code 0x%lx %s.\n",
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

    atomic_long_inc(&kbdata_instance()->ps2_kbd_interrupts);

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

/** Initializes this kernel module.
 *
 *  1. Create workqueue to process information outside of interrupt handlers.
 *  2. Install interrupt handler for PS2 keyboard line IRQ1.
 */
static
int __init kbdevstat_init(void)
{
    int err = 0;

    printk(KERN_INFO MODULE_NAME ": Initializing\n");

    err = kbdata_init(kbdata_instance());

    if (err) {
        pr_info(MODULE_NAME ": can't init data\n");
        goto do_kbdata_clean;
    }

    kbd_wq = create_singlethread_workqueue(KBDEVSTAT_WQ_NAME);

    if (kbd_wq == NULL) {
        printk(KERN_ALERT MODULE_NAME ": can't create workqueue\n");
        goto do_kbdata_clean;
    }

    err = request_irq(KBD_IRQ,
                    irq_handler,
                    /*flags*/  IRQF_SHARED,
                    /*name*/   "test_keyboard_irq_handler",
                    /*dev_id*/ (void *)(irq_handler));

    if (err) {
        pr_alert(MODULE_NAME ": can't request IRQ1\n");
        goto do_kbd_wq_clean;
    }

    return err;

do_kbd_wq_clean:
    if (kbd_wq) { destroy_workqueue(kbd_wq); }
do_kbdata_clean:
    kbdata_clean(kbdata_instance());

    return err;
}

/** Cleans and de-allocates resources once module gets un-loaded.
 *
 *  1. Un-install PS2 keyboard IRQ1 interrupt handler.
 *  2. Flush and destroy the workqueue.
 */
static
void __exit kbdevstat_exit(void) {

    printk(KERN_INFO MODULE_NAME ": Exiting after %lu interrupts\n",
        atomic_long_read(&kbdata_instance()->ps2_kbd_interrupts));

    kbdata_clean(kbdata_instance());

    free_irq(KBD_IRQ, /*dev_id=*/irq_handler);

    if (kbd_wq) {
        destroy_workqueue(kbd_wq);
    }

}


// module_init and module_exit are special kernel macros
// to indicate the role of <module>_init and <module>_exit functions.
module_init(kbdevstat_init);
module_exit(kbdevstat_exit);
