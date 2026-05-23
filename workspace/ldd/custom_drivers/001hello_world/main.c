#include <linux/module.h>

static int __init helloworld_init(void)
{
    pr_info("Hello World\n");
    return 0;
}

static void __exit helloworld_cleanup(void)
{
    pr_info("GoodBye\n");
}

module_init(helloworld_init);
module_exit(helloworld_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("A simple kernel hello world kernel module");
MODULE_INFO(board, "RPI 5");

