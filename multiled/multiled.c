#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include<linux/version.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>

#define MAX 3


#define LED_RED 21
#define LED_GREEN 20
#define LED_YELLOW 16


static struct gpio Leds_gpio[] = {
	{LED_RED,GPIOF_OUT_INIT_LOW,"LED-1"},
	{LED_GREEN,GPIOF_OUT_INIT_LOW,"LED-2"},
	{LED_YELLOW,GPIOF_OUT_INIT_LOW,"LED-3"}
};

static dev_t device_number;
static struct class* Led_class;
static struct cdev Led_cdev[MAX];



//static int minor;


static int Led_open(struct inode *pinode,struct file *pfile){
	int minor;
	printk("Device file opened...\n");
	minor = iminor(file_inode(pfile));
	if( minor == 0 ){
		if(gpio_is_valid(Leds_gpio[0].gpio) == false){
			printk("GPIO - %d is not valid\n",Leds_gpio[0].gpio);
			return -1;
		}
		if(gpio_request(Leds_gpio[0].gpio,"LED_RED") < 0){
			printk("GPIO - %d failed to request\n",Leds_gpio[0].gpio);
			return -1;

		}
		if(gpio_direction_output(Leds_gpio[0].gpio,0) < 0){
			printk("GPIO -%d failed to set direction\n",Leds_gpio[0].gpio);
			gpio_free(Leds_gpio[0].gpio);
			return -1;
		}
	}else if(minor == 1){
		if(gpio_is_valid(Leds_gpio[1].gpio) == false){
			printk("GPIO - %d is not valid\n",Leds_gpio[1].gpio);
			return -1;
		}
		if(gpio_request(Leds_gpio[1].gpio,"LED_GREEN") < 0){
			printk("GPIO - %d failed to request\n",Leds_gpio[1].gpio);
			return -1;
		}
		if(gpio_direction_output(Leds_gpio[1].gpio,0) < 0){
			printk("GPIO -%d failed to set direction\n",Leds_gpio[1].gpio);
			gpio_free(Leds_gpio[1].gpio);
			return -1;
		}  
	}else{
		if(gpio_is_valid(Leds_gpio[2].gpio) == false){
			printk("GPIO - %d is not valid\n",Leds_gpio[2].gpio);
			return -1;
		}
		if(gpio_request(Leds_gpio[2].gpio,"LED_YELLOW") < 0){
			printk("GPIO - %d failed to request\n",Leds_gpio[2].gpio);
			return -1;
		}
		if(gpio_direction_output(Leds_gpio[2].gpio,0) < 0){
			printk("GPIO -%d failed to set direction\n",Leds_gpio[2].gpio);
			gpio_free(Leds_gpio[2].gpio);
			return -1;   
		}
	}
	return 0;
}


static int Led_close(struct inode *pinode,struct file *pfile){
	int minor = iminor(file_inode(pfile));
	printk("Device file closed!\n");
	gpio_free(Leds_gpio[minor].gpio);

	return 0;
}

static ssize_t Led_read(struct file* pfile,char* __user buf,size_t len,loff_t *loff){
	printk("Device file opened to read %ld\n",len);
	return len;
}

static ssize_t Led_write(struct file* pfile,const char* __user buf,size_t len,loff_t *loff){
	int minor = iminor(file_inode(pfile));
	uint8_t dev_buf[10] = {0};
	printk("led state has changed\n");
	if(copy_from_user(dev_buf,buf,len) > 0){
		printk("Couldn't write all the given bytes\n");
		return -1;
	}
	if(minor == 0){
		if(dev_buf[0] == '0'){
			gpio_set_value(Leds_gpio[0].gpio,0);
		}else if(dev_buf[0] == '1'){
			gpio_set_value(Leds_gpio[0].gpio,1);
		}else{
			printk("Please write either 1 or 0\n");
			return -1;
		}
	}else if( minor == 1){
		if(dev_buf[0] == '0'){
			gpio_set_value(Leds_gpio[1].gpio,0);
		}else if(dev_buf[0] == '1'){
			gpio_set_value(Leds_gpio[1].gpio,1);
		}else{
			printk("Please write either 1 or 0\n");
			return -1;
		}
	}else{
		if(dev_buf[0] == '0'){
			gpio_set_value(Leds_gpio[2].gpio,0);
		}else if(dev_buf[0] == '1'){
			gpio_set_value(Leds_gpio[2].gpio,1);
		}else{
			printk("Please write either 1 or 0\n");
			return -1;
		}
	}
	return len;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = Led_open,
	.read = Led_read,
	.write = Led_write,
	.release = Led_close,
};

static int __init Led_init(void){
	int err = alloc_chrdev_region(&device_number,0,MAX,"Led_devices");
	dev_t dev_num;
	int major,i;
	if(err < 0){
		printk("failed to allocate major number\n");
		return -1;
	}
	major = MAJOR(device_number);
	printk("Device register with <MAJOR> = <%d>\n",MAJOR(device_number));
	Led_class = class_create(THIS_MODULE,"Led_class");

	if(IS_ERR(Led_class)){
		printk("failed to create class\n");
		unregister_chrdev_region(device_number,1);
		return -2;
	}
	for(i=0;i<MAX;i++){
		dev_num = MKDEV(major,i);
		printk("1st minor number = %d\n",MINOR(dev_num));
		cdev_init(&Led_cdev[i],&fops);
		if(cdev_add(&Led_cdev[i],dev_num,1) < 0){
			printk("Failed at cdev_add\n");
			class_destroy(Led_class);
			unregister_chrdev_region(device_number,1);
			return -3;
		}
		if(IS_ERR(device_create(Led_class,NULL,dev_num,NULL,"Led_device-%d",i))){
			printk("Failed to create device file in /dev directory\n");
			cdev_del(&Led_cdev[i]);
			class_destroy(Led_class);
			unregister_chrdev_region(device_number,1);
			return -4;
		}
	}
	printk("Module loaded succesfully\n");
	return 0;
}



static void __exit Led_cleanup(void){
	dev_t dev_num;
	int i,major;
	major = MAJOR(device_number);
	for(i=0;i<MAX;i++){
		dev_num = MKDEV(major ,i);
		cdev_del(&Led_cdev[i]);
		device_destroy(Led_class,dev_num);
	}
	class_destroy(Led_class);
	unregister_chrdev_region(device_number,MAX);
	printk("Device driver has been removed succefully\n");
}


module_init(Led_init);
module_exit(Led_cleanup);

MODULE_AUTHOR("Jyothirmai");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(" simple gpio driver for multiple LEDs");
MODULE_VERSION("1.0");
