#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  
#include <linux/gpio.h>     
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/jiffies.h>



#define GPIO_21_OUT (21)


#define GPIO_20_IN  (20)



unsigned int led_toggle = 0; 

unsigned int gpio_irq_number;



static irqreturn_t gpio_irq_handler(int irq,void *dev_id) {
	uint8_t switch_state = 0;
	switch_state = gpio_get_value(GPIO_20_IN);
	gpio_set_value(GPIO_21_OUT,switch_state);                      
	printk("Interrupt Occurred : GPIO_21_OUT : %d ",gpio_get_value(GPIO_21_OUT));
	if(switch_state == 1){
		mdelay(1000);
	}
	else if(switch_state == 0){
		mdelay(500);
	}

	return IRQ_HANDLED;
}

dev_t switch_dev;
static struct class *switch_class;
static struct cdev switch_cdev;


static int switch_open(struct inode *inode, struct file *file){
	printk("Device File Opened...!!!\n");
	if(gpio_is_valid(GPIO_20_IN) == false){
		printk("GPIO - %d is not valid\n",GPIO_20_IN);
		return -1;
	}
	if(gpio_request(GPIO_20_IN,"GPIO_20_IN") < 0){
		printk("GPIO -%d faild to request\n",GPIO_20_IN);
		return -1;
	}
	if(gpio_direction_input(GPIO_20_IN) <0){
		printk("GPIO - %d failed to set direction\n",GPIO_20_IN);
		gpio_free(GPIO_20_IN);
		return -1;
	}
	gpio_irq_number = gpio_to_irq(GPIO_20_IN);
	printk("GPIO IRQ Number = %d\n", gpio_irq_number);
	if (request_irq(gpio_irq_number,(void *)gpio_irq_handler,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"switch_device",NULL)){                    
		printk("Switch cannot register IRQ\n");
		return -1;
	}
	return 0;
}

static int switch_release(struct inode *inode, struct file *file){
	printk("Device File Closed...!!!\n");
	free_irq(gpio_irq_number,NULL);
	gpio_free(GPIO_20_IN);
	return 0;
}


static ssize_t switch_read(struct file *filp,char __user *buf, size_t len, loff_t *off){
	uint8_t gpio_state = 0;

	gpio_state = gpio_get_value(GPIO_20_IN);
	if (len<1){
		printk("buffer size is invalid\n");
		return -1;
	}

	printk("Read function : GPIO_20 = %d \n", gpio_state);

	return len;
}


static struct file_operations fops ={
	.owner          = THIS_MODULE,
	.read           = switch_read,
	.open           = switch_open,
	.release        = switch_release,
};


static int __init switch_driver_init(void){
	int err = alloc_chrdev_region(&switch_dev, 0, 1, "switch_Dev");
	if( err < 0){
		printk("Failed to allocate character device region\n");
		return -1;
	}
	printk("Major = %d Minor = %d \n",MAJOR(switch_dev), MINOR(switch_dev));
	
	cdev_init(&switch_cdev,&fops);
	switch_cdev.owner = THIS_MODULE;

	if(cdev_add(&switch_cdev,switch_dev,1) < 0){
		printk("Failed at cdev_add\n");
		unregister_chrdev_region(switch_dev, 1);
		return -1;
	}
	switch_class = class_create(THIS_MODULE,"switch_class");

	if(IS_ERR(switch_class)){
		printk("Failed to create class\n");
		cdev_del(&switch_cdev);
		unregister_chrdev_region(switch_dev, 1);
		return -1;
	}

	if(IS_ERR(device_create(switch_class,NULL,switch_dev,NULL,"switch_device"))){
		printk("Failed to create device file in /dev directory\n");
		class_destroy(switch_class);
		cdev_del(&switch_cdev);
		unregister_chrdev_region(switch_dev, 1);
		return -1;
	}

	printk("Device Driver Insert...Done!!!\n");
	return 0;
}

static void __exit switch_driver_exit(void){
	device_destroy(switch_class,switch_dev);
	class_destroy(switch_class);
	cdev_del(&switch_cdev);
	unregister_chrdev_region(switch_dev, 1);
	printk("Device Driver Remove...Done!!\n");
}

module_init(switch_driver_init);
module_exit(switch_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothirmai");
MODULE_DESCRIPTION("A simple GPIO Driver (Switch Interrupt)");
MODULE_VERSION("1.0");
