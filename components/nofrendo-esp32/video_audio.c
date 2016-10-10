/* vim: set tabstop=3 expandtab:
**
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** sdl.c
**
** $Id: sdl.c,v 1.2 2001/04/27 14:37:11 neil Exp $
**
*/

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#undef false
#undef true
#undef bool


#include <math.h>
#include <string.h>
#include <noftypes.h>
#include <bitmap.h>
#include <nofconfig.h>
#include <event.h>
#include <gui.h>
#include <log.h>
#include <nes.h>
#include <nes_pal.h>
#include <nesinput.h>
#include <osd.h>
#include <stdint.h>

#include <driver/spi_lcd.h>



#define  DEFAULT_SAMPLERATE   22050
#define  DEFAULT_BPS          8
#define  DEFAULT_FRAGSIZE     1024

#define  DEFAULT_WIDTH        256
#define  DEFAULT_HEIGHT       NES_VISIBLE_HEIGHT


TimerHandle_t timer;

//Seemingly, this will be called only once. Should call func with a freq of frequency,
int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
	printf("Timer install, freq=%d\n", frequency);
	timer=xTimerCreate("nes",configTICK_RATE_HZ/frequency, pdTRUE, NULL, func);
	xTimerStart(timer, 0);
   return 0;
}


/*
** Audio
*/
static int sound_bps = DEFAULT_BPS;
static int sound_samplerate = DEFAULT_SAMPLERATE;
static int sound_fragsize = DEFAULT_FRAGSIZE;
static unsigned char *audioBuffer = NULL;
static void (*audio_callback)(void *buffer, int length) = NULL;

/* this is the callback that SDL calls to obtain more audio data */
static void sdl_audio_player(void *udata, unsigned char *stream, int len)
{
   /* SDL requests buffer fills in terms of bytes, not samples */
   if (16 == sound_bps)
      len /= 2;

   if (audio_callback)
      audio_callback(stream, len);
}

void osd_setsound(void (*playfunc)(void *buffer, int length))
{
   //Indicates we should call playfunc() to get more data.
   audio_callback = playfunc;
}

static void osd_stopsound(void)
{
   audio_callback = NULL;
}

static int osd_init_sound(void)
{
   sound_bps = DEFAULT_BPS;
   sound_samplerate = DEFAULT_SAMPLERATE;
   sound_fragsize = DEFAULT_FRAGSIZE;

   audio_callback = NULL;

   return 0;
}

void osd_getsoundinfo(sndinfo_t *info)
{
   info->sample_rate = DEFAULT_SAMPLERATE;
   info->bps = DEFAULT_FRAGSIZE;
}

/*
** Video
*/

static int init(int width, int height);
static void shutdown(void);
static int set_mode(int width, int height);
static void set_palette(rgb_t *pal);
static void clear(uint8 color);
static bitmap_t *lock_write(void);
static uint16_t fb[1];
static void free_write(int num_dirties, rect_t *dirty_rects);
static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects);

uint16 line[320];


viddriver_t sdlDriver =
{
   "Simple DirectMedia Layer",         /* name */
   init,          /* init */
   shutdown,      /* shutdown */
   set_mode,      /* set_mode */
   set_palette,   /* set_palette */
   clear,         /* clear */
   lock_write,    /* lock_write */
   free_write,    /* free_write */
   custom_blit,   /* custom_blit */
   false          /* invalidate flag */
};


bitmap_t *myBitmap;

void osd_getvideoinfo(vidinfo_t *info)
{
   info->default_width = DEFAULT_WIDTH;
   info->default_height = DEFAULT_HEIGHT;
   info->driver = &sdlDriver;
}

/* flip between full screen and windowed */
void osd_togglefullscreen(int code)
{
}

/* initialise video */
static int init(int width, int height)
{
	return 0;
}

static void shutdown(void)
{
}

/* set a video mode */
static int set_mode(int width, int height)
{
   return 0;
}

uint16 myPalette[256];

/* copy nes palette over to hardware */
static void set_palette(rgb_t *pal)
{
	uint16 c;

   int i;

   for (i = 0; i < 256; i++)
   {
      c=(pal[i].b>>3)+((pal[i].g>>2)<<5)+((pal[i].r>>3)<<11);
      //myPalette[i]=(c>>8)|((c&0xff)<<8);
      myPalette[i]=c;
   }

}

/* clear all frames to a particular color */
static void clear(uint8 color)
{
//   SDL_FillRect(mySurface, 0, color);
}



/* acquire the directbuffer for writing */
static bitmap_t *lock_write(void)
{
//   SDL_LockSurface(mySurface);
   myBitmap = bmp_createhw((uint8*)fb, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_WIDTH*2);
   return myBitmap;
}

/* release the resource */
static void free_write(int num_dirties, rect_t *dirty_rects)
{
   bmp_destroy(&myBitmap);
}


static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects) {
	int x, y;
	for (y=0; y<DEFAULT_HEIGHT; y++) {
		for (x=0; x<DEFAULT_WIDTH; x++) {
			line[x]=myPalette[(unsigned char)bmp->line[y][x]];
		}
		ili9341_send_data((320-DEFAULT_WIDTH)/2, y+((240-DEFAULT_HEIGHT)/2), DEFAULT_WIDTH, 1, line);
	}
}



/*
** Input
*/

static void osd_initinput()
{
}

void osd_getinput(void)
{
//            func_event(code);
}

static void osd_freeinput(void)
{
}

void osd_getmouse(int *x, int *y, int *button)
{
}

/*
** Shutdown
*/

/* this is at the bottom, to eliminate warnings */
void osd_shutdown()
{
   osd_stopsound();
   osd_freeinput();
}

static int logprint(const char *string)
{
   return printf("%s", string);
}

/*
** Startup
*/

int osd_init()
{
	int y;
   log_chain_logfunc(logprint);

   if (osd_init_sound())
      return -1;

	ili9341_init();
	memset(line, 0, 320);
	for (y=0; y<240; y++) ili9341_send_data(0, y, 320, 1, line);
   osd_initinput();

   return 0;
}