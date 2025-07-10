#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0xdcb764ad, "memset" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x102fe6de, "hrtimer_cancel" },
	{ 0xd9ec4b80, "gpio_to_desc" },
	{ 0x178f2652, "gpiod_set_raw_value" },
	{ 0xc0b7c197, "hrtimer_start_range_ns" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xf0024008, "gpiod_direction_output_raw" },
	{ 0xea82d349, "hrtimer_init" },
	{ 0xd75c6742, "__register_chrdev" },
	{ 0xf311fc60, "class_create" },
	{ 0xad100f12, "device_create" },
	{ 0xfe990052, "gpio_free" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x4a41ecb3, "class_destroy" },
	{ 0xb477e7f4, "device_destroy" },
	{ 0x77b39d3d, "class_unregister" },
	{ 0x135bb7ec, "hrtimer_forward" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "F0AC1E739BEE77F51602A7D");
