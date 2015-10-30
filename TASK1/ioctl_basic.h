#include <linux/ioctl.h>

#define IOC_MAGIC 'k'
#define IOCTL_FLASHGETS _IOR(IOC_MAGIC, 0, int) 
#define IOCTL_FLASHSETP _IOW(IOC_MAGIC, 1, unsigned long)
#define IOCTL_FLASHERASE _IOW(IOC_MAGIC, 2, int) 
#define IOCTL_FLASHGETP _IOR(IOC_MAGIC, 3, unsigned long)

