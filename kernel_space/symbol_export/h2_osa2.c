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
MODULE_DESCRIPTION("Module task : Calculator module (h2_osa2.c)");

// Function prototypes
int calculator_result_fnc(int);			// Export calculation result out
extern int parameter_value_fnc(int);	// Import parameter values for calculation

// Make requested calculation (see defines)
int calculator_result_fnc(int oper_type)
{

	int ret_value;

	printk(" - Calculator [h2_osa2] : Got calculation request from [h2_osa3]\n"); 
	printk(" - Calculator [h2_osa2] : Making parameters requests to module [h2_osa1]\n"); 

	// Check what oper type for Result Writer [h2_osa3]
	switch ( oper_type ) {
		case OPER_TYPE_SUB :
			ret_value = parameter_value_fnc(0) - parameter_value_fnc(1);
			printk(" - Calculator [h2_osa2] : Ansver calculation request, type : SUB\n"); 
			break;
		case OPER_TYPE_ADD :
			ret_value = parameter_value_fnc(0) + parameter_value_fnc(1);
			printk(" - Calculator [h2_osa2] : Ansver calculation request, type : ADD\n"); 
			break;
		default :
			ret_value = 0;
			printk(" - Calculator [h2_osa2] : Ansver calculation request, type : UNKNOWN\n"); 
			break;
	}

	return ret_value;

}


static int __init calculator_init(void)
{
	printk("Module task : [h2_osa2] Calculator module inserted\n");
	return 0;
}

static void __exit calculator_exit(void)
{
	printk("Module task : [h2_osa2] Calculator module exit\n");
}

module_init(calculator_init);
module_exit(calculator_exit);

// Export
EXPORT_SYMBOL_GPL(calculator_result_fnc);