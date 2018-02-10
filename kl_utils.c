#ifndef KL_UTILS_H
#define KL_UTILS_H

#include "kl_utils.h"

uint8_t shift_pressed = 0;
loff_t pos = 0;

bool isShift(uint16_t code) {
   	return code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT;
}

char *getKeyText(uint16_t code) {

   char **arr;
   if (shift_pressed != 0) {
      arr = shift_key_names;
   } else {
      arr = key_names;
   }

   return arr[code];
}

void write_to_file(struct file* file, char* buffer, int size){

	vfs_write(file, buffer, size, &pos);
	pos += size;

}

void read_from_file(struct file* file, char* buffer, int size){
	char ker_buffer[50];

	vfs_read(file, buffer, size, &pos);
}

struct file* open_file(char* file_name){

  	return filp_open(file_name, O_RDWR|O_CREAT, 0644);	
}

void close_file(struct file* file){

	filp_close(file, NULL);
}

#endif