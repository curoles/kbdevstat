/** @file
 *  @brief  Keyboard data, kobject attributes.
 *  @author Igor Lesik 2018
 *
 *
 */
#pragma once

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/kobject.h>

typedef struct kbdata
{
    /// Sysfs module/kbdevstat/ps2
    struct kobject *ps2_kobject;

    /// Static variable to keep total count of interrupts for PS2 keyboard.
    atomic_long_t ps2_kbd_interrupts;


    struct kobj_attribute ps2_kbd_interrupts_attr;

} Kbdata;

struct kbdata* kbdata_instance(void);

int kbdata_init(struct kbdata *kbd);
int kbdata_clean(struct kbdata *kbd);
