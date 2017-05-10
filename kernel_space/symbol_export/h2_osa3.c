/*

Exporting symbols :
• Create three modules who all depend on one another.
• First module takes two module parameters of type int.
• Second module uses these parameters to add and substract them from one another.
• Third module prints out the results
• You need to export symbols as functions in the first and second module.
  First will have typical getters (get a and get b) and the second will have the add and substract

*/

#define OPER_TYPE_ADD 0
#define OPER_TYPE_SUB 1

#include <linux/init.h>
#include <linux/module.h>


// License and author
MODULE_LICENSE("GPL");
MODULE_AUTHOR("erno.kilpelainen@hotmail.com");
MODULE_DESCRIPTION("Module task : Result Writer module (h2_osa3.c)");

// Function prototype
extern int calculator_result_fnc(int);	// Import requested calculation result

static int __init result_writer_init(void)
{
	
	printk("Module task : [h2_osa3] Result Writer inserted\n");
   	
	// Ask result from calculation module [h2_osa2] and print values
	printk(" - Print [h2_osa3]      : Request calculation value from [h2_osa2] with parameter ADD\n");
	printk(" - Print [h2_osa3]      : Result is : %d\n", calculator_result_fnc(OPER_TYPE_ADD));
	printk(" - Print [h2_osa3]      : Request calculation value from [h2_osa2] with parameter ADD\n");
	printk(" - Print [h2_osa3]      : Result is : %d\n", calculator_result_fnc(OPER_TYPE_SUB));
	
	return 0;
}

static void __exit result_writer_exit(void)
{
	printk("Module task : [h2_osa3] Result Writer exit\n");
}

module_init(result_writer_init);
module_exit(result_writer_exit);
