#ifndef __GP2X_JOYSTICK_H__
#define __GP2X_JOYSTICK_H__

#include <fcntl.h>
#include <linux/fb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

enum  { GP2X_UP=0x1,         GP2X_LEFT=0x4,         GP2X_DOWN=0x10,      GP2X_RIGHT=0x40,
        GP2X_START=(1<<8),   GP2X_SELECT=1<<9,      GP2X_L=(1<<10),      GP2X_R=(1<<11),
        GP2X_A=1<<12,        GP2X_B=1<<13,          GP2X_X=1<<14,        GP2X_Y=1<<15,
        GP2X_VOL_UP=1<<23,   GP2X_VOL_DOWN=(1<<22), GP2X_CLICK=1<<27 };

extern unsigned long  gp2x_joystick_read(void);
extern void           gp2x_joystick_init(void);
extern void           gp2x_joystick_deinit(void);

void gp2x_set_cpu_speed(int overclock);
void gp2x_restore_cpu_speed(void);
void gp2x_video_waitvsync();

#endif
