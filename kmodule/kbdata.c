/** @file
 *  @brief  Keyboard data, kobject attributes.
 *  @author Igor Lesik 2018
 *
 *
 */
#include "kbdata.h"

#include <linux/module.h>

static
ssize_t ps2_kbd_interrupts_show(struct kobject *kobj,
    struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%ld\n",
        atomic_long_read(&kbdata_instance()->ps2_kbd_interrupts));
}

static
Kbdata kbdata = {
    .ps2_kobject = NULL,
    .ps2_kbd_interrupts = ATOMIC_LONG_INIT(0),
    .ps2_kbd_interrupts_attr = __ATTR_RO(ps2_kbd_interrupts)
};

struct kbdata* kbdata_instance(void) {
    return &kbdata;
}

int kbdata_init(struct kbdata *kbd)
{
    int err = 0;

    atomic_long_set(&kbd->ps2_kbd_interrupts, 0);

    kbd->ps2_kobject = kobject_create_and_add("ps2", &THIS_MODULE->mkobj.kobj);
    if (!kbd->ps2_kobject) {
        err= -ENOMEM;
        goto do_err;
    }

    err = sysfs_create_file(kbd->ps2_kobject, &kbd->ps2_kbd_interrupts_attr.attr);
    if (err) {
        pr_info("failed to create /sys/module/kbdevstat/ps2/ps2_kbd_interrupts\n");
        goto do_ps2_clean;
    }

    return err;

do_ps2_clean:
    kobject_put(kbd->ps2_kobject);
do_err:
    return err;
}

int kbdata_clean(struct kbdata *kbd)
{
    kobject_put(kbd->ps2_kobject);

    return 0;
}
