// http://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html
// https://lwn.net/Kernel/LDD3/
#include <linux/init.h>   // __init and __exit macros
#include <linux/module.h>


// Driver Information
#define DRIVER_VERSION  "1.0.0"
#define DRIVER_AUTHOR   "Igor Lesik"
#define DRIVER_DESC     "Test module"
#define DRIVER_LICENSE  "GPL"

// Kernel Module Information
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#include "test1.c"

#if 0
// A non 0 return means init_module failed; module can't be loaded.
static
int __init igor_init(void) {
    printk(KERN_ALERT "Hello from Igor\n");
    return 0;
}

static
void __exit igor_exit(void) {
    printk(KERN_ALERT "Goodby from Igor\n");
}
#endif

// module_init and module_exit lines use special kernel macros
// to indicate the role of <module>_init and <module>_exit two functions.
module_init(kbdevstat_init);
module_exit(kbdevstat_exit);
