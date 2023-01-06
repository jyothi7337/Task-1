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
#include <linux/err.h>


#define GPIO_21 (21)
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev led_cdev;
 
 

static int led_open(struct inode *inode, struct file *file)
{
  pr_info("Device File is Opened\n");
  return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
  pr_info("Device File is Closed\n");
  return 0;
}

static ssize_t led_read(struct file *filp,char __user *buf, size_t len, loff_t *off)
{
  uint8_t gpio_state = 0;
  
  gpio_state = gpio_get_value(GPIO_21);
  
  len = 1;
  if( copy_to_user(buf, &gpio_state, len) > 0) {
    pr_err("ERROR: Not all the bytes have been copied to user\n");
  }
  
  pr_info("Read function : GPIO_21 = %d \n", gpio_state);
  
  return 0;
}

static ssize_t led_write(struct file *filp,const char __user *buf, size_t len, loff_t *off)
{
  uint8_t rec_buf[10] = {0};
  
  if( copy_from_user( rec_buf, buf, len ) > 0) {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
  }
  
  pr_info("Write Function : GPIO_21 Set = %c\n", rec_buf[0]);
  
  if (rec_buf[0]=='1') {
    gpio_set_value(GPIO_21, 1);
  } else if (rec_buf[0]=='0') {
    gpio_set_value(GPIO_21, 0);
  } else {
    pr_err("Unknown command : Please provide either 1 or 0 \n");
  }
  
  return len;
}


static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = led_read,
  .write          = led_write,
  .open           = led_open,
  .release        = led_release,
};


static int __init led_init(void)
{
  if((alloc_chrdev_region(&dev, 0, 1, "led_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    goto r_unreg;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
  cdev_init(&led_cdev,&fops);
 
  if((cdev_add(&led_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }
 
  if(IS_ERR(dev_class = class_create(THIS_MODULE,"led_class"))){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }
 
  if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"led_device"))){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
 
  if(gpio_is_valid(GPIO_21) == false){
    pr_err("GPIO %d is not valid\n", GPIO_21);
    goto r_device;
  }
  if(gpio_request(GPIO_21,"GPIO_21") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_21);
    goto r_gpio;
  }
  
  gpio_direction_output(GPIO_21, 0);
  
  pr_info("Device Driver Insertion is Done\n");
  return 0;
 
r_gpio:
  gpio_free(GPIO_21);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  cdev_del(&led_cdev);
r_unreg:
  unregister_chrdev_region(dev,1);
  
  return -1;
}


static void __exit led_exit(void)
{
  gpio_free(GPIO_21);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&led_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Removing is Done\n");
}
 
module_init(led_init);
module_exit(led_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jyothirmai");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.0");
