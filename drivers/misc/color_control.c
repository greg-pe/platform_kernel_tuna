/* drivers/misc/color_control.c
 *
 * Copyright 2012  Ezekeel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#define COLORCONTROL_VERSION 3

extern void colorcontrol_update(bool multiplier_updated);

static bool safety_enabled = true;

static int * v1_offset;

static u32 * color_multiplier;

static u32 original_multiplier[3];

void colorcontrol_register_offset(int * offset)
{
    v1_offset = offset;

    return;
}
EXPORT_SYMBOL(colorcontrol_register_offset);

void colorcontrol_register_multiplier(u32 * multiplier)
{
    int i;

    color_multiplier = multiplier;

    for (i = 0; i < 3; i++)
	original_multiplier[i] = color_multiplier[i];

    return;
}
EXPORT_SYMBOL(colorcontrol_register_multiplier);

static ssize_t colorcontrol_offset_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%i %i %i\n", v1_offset[0], v1_offset[1], v1_offset[2]);
}

static ssize_t colorcontrol_offset_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    int i;
    int new_offset[3];

    if(sscanf(buf, "%i %i %i\n", &new_offset[0], &new_offset[1], &new_offset[2]) == 3) 
	{
	    for (i = 0; i < 3; i++)
		v1_offset[i] = new_offset[i];

	    pr_info("COLORCONTROL V1 offsets changed\n");

	    colorcontrol_update(false);
	} 
    else 
	{
	    pr_info("%s: invalid input\n", __FUNCTION__);
	}

    return size;
}

// imoseyon - le sighhhh
static ssize_t red_v1_offset_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%i\n", v1_offset[0]+60);
}
static ssize_t green_v1_offset_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%i\n", v1_offset[1]+60);
}
static ssize_t blue_v1_offset_show(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%i\n", v1_offset[2]+60);
}
static ssize_t red_v1_offset_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
  int value;
  if (sscanf(buf, "%i", &value) == 1) {
	v1_offset[0] = value-60;
	colorcontrol_update(false);
  }
  return size;
}
static ssize_t green_v1_offset_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
  int value;
  if (sscanf(buf, "%i", &value) == 1) {
	v1_offset[1] = value-60;
	colorcontrol_update(false);
  }
  return size;
}
static ssize_t blue_v1_offset_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
  int value;
  if (sscanf(buf, "%i", &value) == 1) {
	v1_offset[2] = value-60;
	colorcontrol_update(false);
  }
  return size;
}

static ssize_t red_multiplier_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%u\n", color_multiplier[0]);
}
static ssize_t green_multiplier_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%u\n", color_multiplier[1]);
}
static ssize_t blue_multiplier_show(struct device *dev, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%u\n", color_multiplier[2]);
}
static ssize_t red_multiplier_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
  u32 value;
  if (sscanf(buf, "%u", &value) == 1)
  {
    	color_multiplier[0] = value;
	colorcontrol_update(true);
  }
  return size;
}
static ssize_t green_multiplier_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
  u32 value;
  if (sscanf(buf, "%u", &value) == 1)
  {
    	color_multiplier[1] = value;
	colorcontrol_update(true);
  }
  return size;
}
static ssize_t blue_multiplier_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
  u32 value;
  if (sscanf(buf, "%u", &value) == 1)
  {
    	color_multiplier[2] = value;
	colorcontrol_update(true);
  }
  return size;
}
// end 

static ssize_t colorcontrol_multiplier_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u %u %u\n", color_multiplier[0], color_multiplier[1], color_multiplier[2]);
}

static ssize_t colorcontrol_multiplier_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    int i;

    u32 new_multiplier[3];

    if(sscanf(buf, "%u %u %u\n", &new_multiplier[0], &new_multiplier[1], &new_multiplier[2]) == 3) 
	{
	    for (i = 0; i < 3; i++)
		{
		    if (safety_enabled)
			color_multiplier[i] = min(new_multiplier[i], original_multiplier[i]);
		    else
			color_multiplier[i] = new_multiplier[i];
		}

	    pr_info("COLORCONTROL color multipliers changed\n");

	    colorcontrol_update(true);
	} 
    else 
	{
	    pr_info("%s: invalid input\n", __FUNCTION__);
	}

    return size;
}

static ssize_t colorcontrol_safety_read(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", (safety_enabled ? 1 : 0));
}

static ssize_t colorcontrol_safety_write(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    unsigned int data;

    int i;

    bool multipliers_modified = false;

    if(sscanf(buf, "%u\n", &data) == 1) 
	{
	    pr_devel("%s: %u \n", __FUNCTION__, data);
	    
	    if (data == 1) 
		{
		    pr_info("%s: COLORCONTROL safety enabled\n", __FUNCTION__);

		    safety_enabled = true;

		    for (i = 0; i < 3; i++)
			{
			    if (color_multiplier[i] > original_multiplier[i])
				{
				    color_multiplier[i] = original_multiplier[i];

				    multipliers_modified = true;
				}
			}

		    if (multipliers_modified)
			colorcontrol_update(true);
		} 
	    else if (data == 0) 
		{
		    pr_info("%s: COLORCONTROL safety disabled\n", __FUNCTION__);

		    safety_enabled = false;
		} 
	    else 
		{
		    pr_info("%s: invalid input range %u\n", __FUNCTION__, data);
		}
	} 
    else 
	{
	    pr_info("%s: invalid input\n", __FUNCTION__);
	}

    return size;
}

static ssize_t colorcontrol_version(struct device * dev, struct device_attribute * attr, char * buf)
{
    return sprintf(buf, "%u\n", COLORCONTROL_VERSION);
}

static DEVICE_ATTR(v1_offset, S_IRUGO | S_IWUGO, colorcontrol_offset_read, colorcontrol_offset_write);
static DEVICE_ATTR(multiplier, S_IRUGO | S_IWUGO, colorcontrol_multiplier_read, colorcontrol_multiplier_write);
static DEVICE_ATTR(safety_enabled, S_IRUGO | S_IWUGO, colorcontrol_safety_read, colorcontrol_safety_write);
static DEVICE_ATTR(version, S_IRUGO , colorcontrol_version, NULL);

// cm9 stuff
static DEVICE_ATTR(red_v1_offset, S_IRUGO | S_IWUGO, red_v1_offset_show, red_v1_offset_store);
static DEVICE_ATTR(green_v1_offset, S_IRUGO | S_IWUGO, green_v1_offset_show, green_v1_offset_store);
static DEVICE_ATTR(blue_v1_offset, S_IRUGO | S_IWUGO, blue_v1_offset_show, blue_v1_offset_store);
static DEVICE_ATTR(red_multiplier, S_IRUGO | S_IWUGO, red_multiplier_show, red_multiplier_store);
static DEVICE_ATTR(green_multiplier, S_IRUGO | S_IWUGO, green_multiplier_show, green_multiplier_store);
static DEVICE_ATTR(blue_multiplier, S_IRUGO | S_IWUGO, blue_multiplier_show, blue_multiplier_store);

static struct attribute *colorcontrol_attributes[] = 
    {
	&dev_attr_v1_offset.attr,
	&dev_attr_multiplier.attr,
	&dev_attr_safety_enabled.attr,
	&dev_attr_version.attr,
	// imoseyon - next three for cm9
        &dev_attr_red_v1_offset.attr,
        &dev_attr_green_v1_offset.attr,
        &dev_attr_blue_v1_offset.attr,
	&dev_attr_red_multiplier.attr,
	&dev_attr_green_multiplier.attr,
	&dev_attr_blue_multiplier.attr,
	NULL
    };

static struct attribute_group colorcontrol_group = 
    {
	.attrs  = colorcontrol_attributes,
    };

static struct miscdevice colorcontrol_device = 
    {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "colorcontrol",
    };

static int __init colorcontrol_init(void)
{
    int ret;

    pr_info("%s misc_register(%s)\n", __FUNCTION__, colorcontrol_device.name);

    ret = misc_register(&colorcontrol_device);

    if (ret) 
	{
	    pr_err("%s misc_register(%s) fail\n", __FUNCTION__, colorcontrol_device.name);

	    return 1;
	}

    if (sysfs_create_group(&colorcontrol_device.this_device->kobj, &colorcontrol_group) < 0) 
	{
	    pr_err("%s sysfs_create_group fail\n", __FUNCTION__);
	    pr_err("Failed to create sysfs group for device (%s)!\n", colorcontrol_device.name);
	}

    return 0;
}

device_initcall(colorcontrol_init);
