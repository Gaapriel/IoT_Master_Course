/*

Exporting symbols :
• Create three modules who all depend on one another.
• First module takes two module parameters of type int.
• Second module uses these parameters to add and substract them from one another.
• Third module prints out the results
• You need to export symbols as functions in the first and second module.
  First will have typical getters (get a and get b) and the second will have the add and substract

*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

// License and author
MODULE_LICENSE("GPL");
MODULE_AUTHOR("erno.kilpelainen@hotmail.com");
MODULE_DESCRIPTION("Module task : Parameter reader module (h2_osa1.c)");

// Function prototype
int parameter_value_fnc(int); // Export requested parameter values

// Static variable for command line parameters
static int param[2] = { 0, 0 };
static int arr_argc = 0;

// Define format for command line parameter (1 or 2 int)
module_param_array(param, int, &arr_argc, 0000);

// Description of parameter
MODULE_PARM_DESC(param, "Give 2 integers. Example : param=100,-30");

// Parameter export function, return requested parameter number
// Return 0 if requested out of parameter range
int parameter_value_fnc(int req_par_number)
{ 
	int ret_value;	// Return value

	// Check if inside range
	if( req_par_number > -1 && req_par_number < 2 )
		ret_value = param[req_par_number];
	else
		ret_value = 0; 

	// Debug message
	printk(" - Parameter [h2_osa1]  : Ansver parameter request (Number:Value) - (%d:%d)\n", req_par_number, ret_value);
	
	// Return requested value
	return ret_value;
}

static int __exit param_reader_init(void)
{
	printk("Module task : [h2_osa1] Param reader inserted\n");
	printk(" - Parameter [h2_osa1]  : cmd line par 0=%d 1=%d\n", param[0], param[1]);
	return 0;
}

static void __exit param_reader_exit(void)
{
	printk("Module task : [h2_osa1] Param reader removed\n");
}

// Init and exit
module_init(param_reader_init);
module_exit(param_reader_exit);

//Export 
EXPORT_SYMBOL_GPL(parameter_value_fnc);
