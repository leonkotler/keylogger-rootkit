obj-m += keylogger.o
keylogger-objs := keylogger_module.o kl_utils.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean