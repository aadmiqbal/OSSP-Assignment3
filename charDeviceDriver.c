#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "charDeviceDriver.h" 

struct my_list{
     struct list_head list; 
     char * data;
};
 
static int device_open(struct inode *inode, struct file *file) 
{
    mutex_lock(&lock);
    try_module_get(THIS_MODULE); 
    return SUCCESS;
} 
 
static int device_release(struct inode *inode, struct file *file) 
{ 
    mutex_unlock(&lock);
    module_put(THIS_MODULE); 
    return SUCCESS; 
} 
 
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) 
{   

    int len;
    int g;
    int newlinecount;	
    int bytes_read = 0; 
    struct my_list *temp;
    const char *msgptr;
 
    if(list_empty(&head1)){
    return -EAGAIN;}
        
    temp=list_first_entry(&head1,struct my_list,list);
    strscpy(message,temp->data,sizeof(message)-1);
    list_del(&temp->list);
    kfree(temp->data);
    kfree(temp); 
    counter--;
    
     len = strlen(message);
    
    for(g=0;g<len;g++){
	if (message[g] == '\n'){
	printk(KERN_INFO "true");
	newlinecount ++;
	}    
    }
    
    if(newlinecount>1)
    	message[strcspn(message, "\n")+1] = 0;
    
    msgptr = message;
 
    while (*msgptr) { 
        put_user(*(msgptr++), buffer++); 
        bytes_read++; 
    }
    
        
    return bytes_read;
} 
 
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) 
{
    int i;
    struct my_list *node;
    
    if(counter>999){
    return -EBUSY;}
    
    if(length > BUF_LEN){
    return -EINVAL;}
 
    for (i = 0; i < length && i < BUF_LEN; i++) {get_user(message[i], buffer + i);}
        
    node=kmalloc(sizeof(struct my_list),GFP_KERNEL);
    node->data = kmalloc(sizeof(message)+1,GFP_KERNEL);
    strscpy(node->data,message,sizeof(message));
    list_add_tail(&node->list,&head1);
    counter++;
    
    return i; 
} 
 

static int initmodule(void)
{ 

    Major = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops); 
 
    if (Major < 0) { 
	printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major; 
    } 
 
    class1 = class_create(THIS_MODULE, DEVICE_NAME); 
    device_create(class1, NULL, MKDEV(Major, 0), NULL, DEVICE_NAME); 
 
    	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");
    
    INIT_LIST_HEAD(&head1);
    mutex_init(&lock);
 
    return 0; 
} 
 
static void cleanup(void)
{ 
        struct my_list *a, *temp;
        list_for_each_entry_safe(a, temp, &head1, list) {
            list_del(&a->list);
            kfree(a);
        }
        
    device_destroy(class1, MKDEV(Major, 0)); 
    class_destroy(class1); 
    
    unregister_chrdev(Major, DEVICE_NAME);
     
    mutex_destroy(&lock); 
} 
 
module_init(initmodule);
module_exit(cleanup);
 
MODULE_LICENSE("GPL");
