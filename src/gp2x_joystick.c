#include "gp2x_joystick.h"

static volatile unsigned long  *gp2x_memregl;
static volatile unsigned short *gp2x_memregs;
static          unsigned long   gp2x_dev=0;

unsigned MDIV,PDIV,SCALE;
static int cpufreq=0;

#define SYS_CLK_FREQ 7372800

void gp2x_video_waitvsync(void)
{
  while(gp2x_memregs[0x1182>>1]&(1<<4));
}

void set_FCLK(unsigned MHZ)
{
        unsigned v;
        unsigned mdiv,pdiv=3,scale=0;
        MHZ*=1000000;
        mdiv=(MHZ*pdiv)/SYS_CLK_FREQ;
        //printf ("Old value = %04X\r",MEM_REG[0x924>>1]," ");
        //printf ("APLL = %04X\r",MEM_REG[0x91A>>1]," ");
        mdiv=((mdiv-8)<<8) & 0xff00;
        pdiv=((pdiv-2)<<2) & 0xfc;
        scale&=3;
        v=mdiv | pdiv | scale;
        gp2x_memregs[0x910>>1]=v;
}

unsigned get_FCLK()
{
        return gp2x_memregs[0x910>>1];
}

unsigned get_freq_920_CLK()
{
        unsigned i;
        unsigned reg,mdiv,pdiv,scale;
        reg=gp2x_memregs[0x912>>1];
        mdiv = ((reg & 0xff00) >> 8) + 8;
        pdiv = ((reg & 0xfc) >> 2) + 2;
        scale = reg & 3;
        MDIV=mdiv;
        PDIV=pdiv;
        SCALE=scale;
        i = (gp2x_memregs[0x91c>>1] & 7)+1;
        return ((SYS_CLK_FREQ * mdiv)/(pdiv << scale))/i;
}
unsigned short get_920_Div()
{
        return (gp2x_memregs[0x91c>>1] & 0x7);
}

unsigned long gp2x_joystick_read(void)
{
	unsigned long value=(gp2x_memregs[0x1198>>1] & 0x00FF);

	if(value==0xFD) value=0xFA;
	if(value==0xF7) value=0xEB;
	if(value==0xDF) value=0xAF;
	if(value==0x7F) value=0xBE;
 
	return ~((gp2x_memregs[0x1184>>1] & 0xFF00) | value | (gp2x_memregs[0x1186>>1] << 16));
}

void gp2x_joystick_init()
{
 	if(!gp2x_dev)  gp2x_dev = open("/dev/mem",   O_RDWR); 
	gp2x_memregl=(unsigned long  *)mmap(0, 0x10000,                    PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_dev, 0xc0000000);
 	gp2x_memregs=(unsigned short *)gp2x_memregl;
	atexit(gp2x_joystick_deinit);
}

void gp2x_joystick_deinit()
{
	gp2x_restore_cpu_speed();
	munmap((void *)gp2x_memregl,      0x10000);
	if(gp2x_dev) close(gp2x_dev);
}

void gp2x_set_cpu_speed(int overclock) {
        unsigned sysfreq=0;

	/* save FCLOCK */
	sysfreq=get_freq_920_CLK();
        sysfreq*=get_920_Div()+1;
        if(cpufreq==0) cpufreq=sysfreq/1000000;
	if (overclock!=0) {
		if (overclock<66) overclock=66;
		if (overclock>320) overclock=320;
		set_FCLK(overclock);
	}
}

void gp2x_restore_cpu_speed()
{
	if(cpufreq==0) return;
	set_FCLK(cpufreq);
}
