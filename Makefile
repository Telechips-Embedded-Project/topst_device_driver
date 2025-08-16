# drivers/mytopst/Makefile
obj-$(CONFIG_MYTOPST_AMBIENT)   += ambient_driver.o
obj-$(CONFIG_MYTOPST_WIPER)     += wiper_driver.o
obj-$(CONFIG_MYTOPST_WINDOW)    += window_driver.o
obj-$(CONFIG_MYTOPST_AIRCON)    += aircon_driver.o
obj-$(CONFIG_MYTOPST_HEADLAMP)  += headlamp_driver.o
