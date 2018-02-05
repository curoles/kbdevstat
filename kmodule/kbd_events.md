# How to observe keyboard events in Linux?

First that came to my mind was to write a module that will
handle keyboard IRQ. In PC architecture keyboard controller always
uses IRQ=1, see [wiki](https://en.wikipedia.org/wiki/Interrupt_request_(PC_architecture)).
Keyboard/mouse can be connected either via PS2 or USB.
Laptop makers prefer PS2 since it draws less energy (PS2 interrupts vs USB pulling, I guess).
I am using Lenovo T510 laptop, to test that the keyboard and mouse
are using PS2 and not USB I wrote a simple script (Ubuntu):


```bash
watch -n 1 "cat /proc/interrupts | grep i8042"
```

The output changes when I press a key on the keyboard or move the mouse.

```terminal
  1:        180      32620      12157      20330   IO-APIC   1-edge      i8042
 12:       9797    1349473    1157669    1160025   IO-APIC  12-edge      i8042
```

The output does **NOT** change when USB mouse is connected.

All right, but I already foresee two problems with this idea:

1. It might not be even possible to install my IRQ handler
   without uninstalling/disabling default Linux keyboard driver.
   Even if it is possible, will `inb(x64/60)` return meaningful data second time
   when called by second handler in the chain? 
2. I will have to handle events from USB internal or external keyboard separately,
   double work.

## Can we install our interrupt handler for IRQ1?

Let us explore first problem by using [IRQ handler example].
The code explicitly removes existing handler by doing `free_irq(1, NULL);` inside `init_module()`.
And the comments suggest that only one handler can be installed.
Is it really the case?
Let us test it with following code:

```c
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
```

Results:

1. Nothing bad seems to happen upon `insmod`.
2. After `rmmod` the whole system hangs/freezes. Makes sense, I do not
   un-register my handle, so when the system tries to call it
   after the module was un-loaded, the system can't "find" it anymore and
   goes into the weeds.

Looking at [`__free_irq(irq, dev_id)`](http://elixir.free-electrons.com/linux/latest/source/kernel/irq/manage.c#L1522)
function one can see that it actually has facility to remove a handler
from the chain by using `dev_id`.

Add code to function `kbdevstat_exit`:

```c
free_irq(KBD_IRQ, /*dev_id*/irq_handler);
```

It works! The system works after `insmod` and after `rmmod`.

```terminal
[ 6297.135644] kbdevstat: loading out-of-tree module taints kernel.
[ 6297.135730] kbdevstat: module verification failed: signature and/or required key missing - tainting kernel
[ 6297.136040] kbdevstat Initializing
[ 6353.221547] kbdevstat Exiting
```

Great, after all it is possible to install our own interrupt handler
on IRQ1 in addition to already installed by _atkbd_ handler.
But will it ALL work if we call `inb(0x64/60)` from our handler?

## Let us call `inb` inside our handler and see what happens.

I will use Linux _workqueue_ for bottom-half of the interrupt handler.
In the top-half I call `inb(0x60)` to get value of the scancode,
then I copy this value into `kbd_work` structure and schedule the work
in my work queue. In the bottom-half I just print the scancode to
see with with `dmesg`.

```c
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
```

Results:

1. It works somehow.
2. The keyboard stays responsive.
3. I do not observe any missing keystrokes.

```terminal
$ 123456                     => 6 letters + 1[Enter]
$ sudo rmmod ./kbdevstat.ko  => 4[Arrow] + 1[Enter]
----------------------------------------------------
                                12 times
```

```terminal
[17924.134751] kbdevstat: Scan Code 0x1c Released.
[17926.290520] kbdevstat: Scan Code 0x2 Pressed.    1
[17926.401790] kbdevstat: Scan Code 0x2 Released.
[17926.785378] kbdevstat: Scan Code 0x3 Pressed.    2
[17926.880517] kbdevstat: Scan Code 0x3 Released.
[17927.215976] kbdevstat: Scan Code 0x4 Pressed.    3
[17927.319252] kbdevstat: Scan Code 0x4 Released.
[17927.630762] kbdevstat: Scan Code 0x5 Pressed.    4
[17927.806131] kbdevstat: Scan Code 0x5 Released.
[17928.164981] kbdevstat: Scan Code 0x6 Pressed.    5
[17928.292304] kbdevstat: Scan Code 0x6 Released.
[17928.659732] kbdevstat: Scan Code 0x7 Pressed.    6
[17928.762928] kbdevstat: Scan Code 0x7 Released.
[17930.508630] kbdevstat: Scan Code 0x1c Pressed.   7 Enter
[17930.587844] kbdevstat: Scan Code 0x1c Released.
[17940.961820] kbdevstat: Scan Code 0x60 Released.
[17940.962463] kbdevstat: Scan Code 0x48 Pressed.   8
[17941.072696] kbdevstat: Scan Code 0x60 Released.
[17941.073619] kbdevstat: Scan Code 0x48 Released.  
[17941.968335] kbdevstat: Scan Code 0x60 Released.
[17941.968845] kbdevstat: Scan Code 0x48 Pressed.   9
[17942.047089] kbdevstat: Scan Code 0x60 Released.
[17942.047854] kbdevstat: Scan Code 0x48 Released.
[17943.046862] kbdevstat: Scan Code 0x60 Released.
[17943.047201] kbdevstat: Scan Code 0x48 Pressed.   10
[17943.117668] kbdevstat: Scan Code 0x60 Released.
[17943.118354] kbdevstat: Scan Code 0x48 Released.
[17943.789247] kbdevstat: Scan Code 0x60 Released.
[17943.789833] kbdevstat: Scan Code 0x48 Pressed.   11
[17943.868098] kbdevstat: Scan Code 0x60 Released.
[17943.868796] kbdevstat: Scan Code 0x48 Released.
[17947.419083] kbdevstat: Scan Code 0x1c Pressed.   12 Enter
[17947.432750] kbdevstat: Exiting after 32 interrupts
```

Lessons learned:

1. Do NOT use `data` field of `queue_work` structure to bring data
   from interrupt handler to bottom-half, it breaks the queue.
2. Need to use `INIT_WORK` **EVERY** time before calling `queue_work`.

Let's see `INIT_WORK` implementation in [workqueue] header file.

```c
#define __INIT_WORK(_work, _func, _onstack)				\
	do {								\
		__init_work((_work), _onstack);				\
		(_work)->data = (atomic_long_t) WORK_DATA_INIT();	\
		INIT_LIST_HEAD(&(_work)->entry);			\
		(_work)->func = (_func);				\
```

First, it is obvious that `data` field is used internally.

`__init_work` calls `__debug_object_init`

```c
static void
__debug_object_init(void *addr, struct debug_obj_descr *descr, int onstack)
{
...
	db = get_bucket((unsigned long) addr);

	raw_spin_lock_irqsave(&db->lock, flags);

	obj = lookup_object(addr, db);
	if (!obj) {
		obj = alloc_object(addr, db, descr);
		if (!obj) {
```

```c
static inline void INIT_LIST_HEAD(struct list_head *list)
{
	WRITE_ONCE(list->next, list);
	list->prev = list;
}
```

`queue_work` calls `insert_work` that calls `list_add_tail`, where the list
is `worklist = &pwq->pool->worklist`.

```c
static void insert_work(struct pool_workqueue *pwq, struct work_struct *work,
			struct list_head *head, unsigned int extra_flags)
{
	struct worker_pool *pool = pwq->pool;

	/* we own @work, set data and link */
	set_work_pwq(work, pwq, extra_flags);
	list_add_tail(&work->entry, head);
```

<!-- References -->
[IRQ handler example]: http://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html#AEN1320
[atkbd]: https://github.com/torvalds/linux/blob/master/drivers/input/keyboard/atkbd.c
[usbkbd]: http://elixir.free-electrons.com/linux/latest/source/drivers/hid/usbhid/usbkbd.c
[workqueue]: http://elixir.free-electrons.com/linux/v4.3.1/source/include/linux/workqueue.h

<!--
https://www.kernel.org/doc/Documentation/input/input.txt
https://www.kernel.org/doc/Documentation/input/input-programming.txt
http://www.kneuro.net/cgi-bin/lxr/http/source/drivers/input/keybdev.c

http://www.linux.it/~rubini/docs/input/input.html

Information on Internet suggests that second `serio_register_driver(&atkbd_drv);` would not work too.

-->
