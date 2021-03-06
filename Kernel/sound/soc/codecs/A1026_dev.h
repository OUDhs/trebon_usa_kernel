#ifndef _A1026_DEV_H
#define _A1026_DEV_H

#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/types.h>


#define A1026_DEBUG

#define error(fmt,arg...) printk(KERN_INFO fmt "\n",## arg)

#ifdef A1026_DEBUG
#define debug(fmt,arg...) printk(KERN_INFO  "[A1026]" fmt "\n",## arg)
#else
#define debug(fmt,arg...)
#endif

typedef struct
{
	int power_state;
	int seek_state;
}dev_state_t;

typedef struct																				
{	
	u8 part_number;
	u16 manufact_number;
} device_id;														

typedef struct														
{
	u8  chip_version;
	u8 	device;
	u8  firmware_version;
}chip_id;															 			

extern int A1026_dev_wait_flag;

/*Function prototypes*/
extern int A1026_dev_init(struct i2c_client *);
extern int A1026_dev_exit(void);
extern int A1026_dev_suspend(void);
extern int A1026_dev_resume(void);
extern int A1026_dev_powerup(void);
extern int A1026_dev_powerdown(void);
extern void A1026SetFeature(unsigned int feature);
extern void A1026Sleep(void);
extern void A1026Wakeup(void);
extern void A1026SetBypass(int bypass);
#endif

