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
	{ 0x7135ea23, "kthread_stop" },
	{ 0xfe990052, "gpio_free" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x122c3a7e, "_printk" },
	{ 0xdcb764ad, "memset" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x13929d91, "gpio_to_desc" },
	{ 0xf01ce0d5, "gpiod_set_raw_value" },
	{ 0xd75c6742, "__register_chrdev" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x3e5d99eb, "gpiod_direction_output_raw" },
	{ 0x678b0c30, "kthread_create_on_node" },
	{ 0x5a543dd9, "wake_up_process" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xf9a482f9, "msleep" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x37befc70, "jiffies_to_msecs" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9C0152179BD7CA2A06ED7B4");
