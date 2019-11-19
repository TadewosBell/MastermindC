#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xa8be126c, "module_layout" },
	{ 0x3fe7e3dd, "misc_deregister" },
	{ 0x999e8297, "vfree" },
	{ 0x842cfd4e, "misc_register" },
	{ 0xd6ee688f, "vmalloc" },
	{ 0xc5850110, "printk" },
	{ 0x65948cb8, "remap_pfn_range" },
	{ 0x3744cf36, "vmalloc_to_pfn" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "0C238CF7710B9D660805959");
