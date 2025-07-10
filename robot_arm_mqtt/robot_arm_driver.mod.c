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
	{ 0x13929d91, "gpio_to_desc" },
	{ 0xf01ce0d5, "gpiod_set_raw_value" },
	{ 0xfe990052, "gpio_free" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xdcb764ad, "memset" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xd75c6742, "__register_chrdev" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x3e5d99eb, "gpiod_direction_output_raw" },
	{ 0xf9a482f9, "msleep" },
	{ 0x678b0c30, "kthread_create_on_node" },
	{ 0x5a543dd9, "wake_up_process" },
	{ 0xbcab6ee6, "sscanf" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "92A5FBC89B251BC19A6BB9A");
