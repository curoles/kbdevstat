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

## Let us call `inb` inside our handler.

<!-- References -->
[IRQ handler example]: http://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html#AEN1320
[atkbd]: https://github.com/torvalds/linux/blob/master/drivers/input/keyboard/atkbd.c
[usbkbd]: http://elixir.free-electrons.com/linux/latest/source/drivers/hid/usbhid/usbkbd.c

<!--
https://www.kernel.org/doc/Documentation/input/input.txt
https://www.kernel.org/doc/Documentation/input/input-programming.txt
http://www.kneuro.net/cgi-bin/lxr/http/source/drivers/input/keybdev.c

http://www.linux.it/~rubini/docs/input/input.html

Information on Internet suggests that second `serio_register_driver(&atkbd_drv);` would not work too.

-->
