/*
  Victor Luchits

  The MIT License (MIT)

  Copyright (c) 2021 Victor Luchits, Joseph Fenton

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "marshw.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static volatile uint16_t mars_activescreen = 0;

char mars_gamepadport[2] = { 0, 1 };
char mars_mouseport = -1;
static volatile uint16_t mars_controlval[2];

volatile unsigned mars_vblank_count = 0;
volatile unsigned mars_pwdt_ovf_count = 0;
volatile unsigned mars_swdt_ovf_count = 0;
unsigned mars_frtc2msec_frac = 0;
const uint8_t* mars_newpalette = NULL;

int16_t mars_requested_lines = 224;
uint16_t mars_framebuffer_height = 224;

uint16_t mars_cd_ok = 0;
uint16_t mars_num_cd_tracks = 0;

uint16_t mars_refresh_hz = 0;

const int NTSC_CLOCK_SPEED = 23011360; // HZ
const int PAL_CLOCK_SPEED = 22801467; // HZ

static volatile int16_t mars_brightness = 0;

///
//Used for tilemapping
//
#define COLOR_BITS (15)
#define COLOR_PRI  (1<<COLOR_BITS)
#define COLOR_MASK ((1<<COLOR_BITS)-1)
#define HwMDPlaneNum(p) ((p) >= 'A' && (p) <= 'B' ? (p)-'A' : ((p) >= 'a' && (p) <= 'b' ? (p)-'a' : (p)&1))
#define UNCACHED_CURFB (*(short *)((int)&currentFB|0x20000000))

static int X = 0, Y = 0;
static int MX = 40, MY = 25;
static int init = 0;
short fgc = -1, bgc = 0;
static short fgs = -1, bgs = 0;
static short fgp = 0, bgp = 0;
static volatile char new_pri;
volatile unsigned short currentFB = 0;

uint32_t canvas_width = 320;
uint32_t canvas_height = 224;

// 384 seems to be the sweet spot - any other value 
// increases the odds of hitting the "0xFF screen shift
// register bug"
uint32_t canvas_pitch = 384; // canvas_width + scrollwidth
uint32_t canvas_yaw = 256; // canvas_height + scrollheight

void Hw32xFlipWait(void)
{
	while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB);
	UNCACHED_CURFB ^= 1;
}
void HwMdClearPlanes(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0600;
	while (MARS_SYS_COMM0);
}
void HwMdSetPlaneBitmap(char plane, void* data)
{
	while (MARS_SYS_COMM0);
	*(volatile uintptr_t*)&MARS_SYS_COMM12 = (uintptr_t)data;
	MARS_SYS_COMM0 = 0x0700 + HwMDPlaneNum(plane);
	while (MARS_SYS_COMM0);
}
void HwMdHScrollPlane(char plane, int hscroll)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM2 = hscroll;
	MARS_SYS_COMM0 = 0x0800 + HwMDPlaneNum(plane);
	while (MARS_SYS_COMM0);
}
void HwMdVScrollPlane(char plane, int hscroll)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM2 = hscroll;
	MARS_SYS_COMM0 = 0x0802 + HwMDPlaneNum(plane);
	while (MARS_SYS_COMM0);
}
void Hw32xSetFGOverlayPriorityBit(int p)
{
	fgp = p ? COLOR_PRI : 0;
	fgc = (fgc & COLOR_MASK) | fgp;
	new_pri = 1;
}
void Hw32xSetBGOverlayPriorityBit(int p)
{
	bgp = p ? COLOR_PRI : 0;
	bgc = (bgc & COLOR_MASK) | bgp;
	new_pri = 1;
}
void Hw32xUpdateLineTable(int hscroll, int vscroll, int lineskip)
{
	unsigned i;
	int i_lineskip;
	const int ymask = canvas_yaw - 1;
	const int pitch = canvas_pitch >> 1;
	uint16_t* frameBuffer16 = (uint16_t*)&MARS_FRAMEBUFFER;

	hscroll += 0x100;

	if (lineskip == 0)
	{
		unsigned count = canvas_height;
		unsigned n = ((unsigned)count + 7) >> 3;
		const int mpitch = pitch * canvas_yaw;

#define DO_LINE() do { \
            if (ppitch >= mpitch) ppitch -= mpitch; \
            *frameBuffer16++ = ppitch + hscroll; /* word offset of line */ \
            ppitch += pitch; \
        } while(0)

		int ppitch = pitch * vscroll;
		switch (count & 7)
		{
		case 0: do {
			DO_LINE();
		case 7:      DO_LINE();
		case 6:      DO_LINE();
		case 5:      DO_LINE();
		case 4:      DO_LINE();
		case 3:      DO_LINE();
		case 2:      DO_LINE();
		case 1:      DO_LINE();
		} while (--n > 0);
		}
		return;
	}

	i_lineskip = 0;
	for (i = 0; i < canvas_height / (lineskip + 1); i++)
	{
		int j = lineskip + 1;
		while (j)
		{
			frameBuffer16[i_lineskip + (lineskip + 1 - j)] = pitch * (vscroll & ymask) + hscroll; /* word offset of line */
			j--;
		}
		vscroll++;
		i_lineskip += lineskip + 1;
	}
}
void Hw32xScreenFlip(int wait)
{
	// Flip the framebuffer selection bit
	MARS_VDP_FBCTL = UNCACHED_CURFB ^ 1;
	if (wait)
	{
		while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB);
		UNCACHED_CURFB ^= 1;
	}
}
void Hw32xInit(int vmode, int lineskip)
{
	volatile unsigned short* frameBuffer16 = &MARS_FRAMEBUFFER;
	int priority = vmode & (MARS_VDP_PRIO_32X | MARS_VDP_PRIO_68K);
	unsigned i;

	// Wait for the SH2 to gain access to the VDP
	while ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0);

	vmode &= ~(MARS_VDP_PRIO_32X | MARS_VDP_PRIO_68K);
	if (vmode == MARS_VDP_MODE_256)
	{
		// Set 8-bit paletted mode, 224 lines
		MARS_VDP_DISPMODE = MARS_224_LINES | MARS_VDP_MODE_256 | priority;

		// init both framebuffers

		// Flip the framebuffer selection bit and wait for it to take effect
		MARS_VDP_FBCTL = UNCACHED_CURFB ^ 1;
		while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB);
		UNCACHED_CURFB ^= 1;
		// rewrite line table
		Hw32xUpdateLineTable(0, 0, lineskip);
		// clear screen
		for (i = 0x100; i < 0x10000; i++)
			frameBuffer16[i] = 0;

		// Flip the framebuffer selection bit and wait for it to take effect
		MARS_VDP_FBCTL = UNCACHED_CURFB ^ 1;
		while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB);
		UNCACHED_CURFB ^= 1;
		// rewrite line table
		Hw32xUpdateLineTable(0, 0, lineskip);
		// clear screen
		for (i = 0x100; i < 0x10000; i++)
			frameBuffer16[i] = 0;

		MX = 40;
		MY = 28 / (lineskip + 1);
	}
	else if (vmode == MARS_VDP_MODE_32K)
	{
		// Set 16-bit direct mode, 224 lines
		MARS_VDP_DISPMODE = MARS_224_LINES | MARS_VDP_MODE_32K | priority;

		// init both framebuffers

		// Flip the framebuffer selection bit and wait for it to take effect
		MARS_VDP_FBCTL = UNCACHED_CURFB ^ 1;
		while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB);
		UNCACHED_CURFB ^= 1;
		// rewrite line table
		for (i = 0; i < canvas_height / (lineskip + 1); i++)
		{
			if (lineskip)
			{
				int j = lineskip + 1;
				while (j)
				{
					frameBuffer16[i * (lineskip + 1) + (lineskip + 1 - j)] = i * canvas_pitch + 0x100; /* word offset of line */
					j--;
				}
			}
			else
			{
				if (i < 200)
					frameBuffer16[i] = i * canvas_pitch + 0x100; /* word offset of line */
				else
					frameBuffer16[i] = 200 * canvas_pitch + 0x100; /* word offset of line */
			}
		}
		// clear screen
		for (i = 0x100; i < 0x10000; i++)
			frameBuffer16[i] = 0;

		// Flip the framebuffer selection bit and wait for it to take effect
		MARS_VDP_FBCTL = UNCACHED_CURFB ^ 1;
		while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB);
		UNCACHED_CURFB ^= 1;
		// rewrite line table
		for (i = 0; i < canvas_height / (lineskip + 1); i++)
		{
			if (lineskip)
			{
				int j = lineskip + 1;
				while (j)
				{
					frameBuffer16[i * (lineskip + 1) + (lineskip + 1 - j)] = i * canvas_pitch + 0x100; /* word offset of line */
					j--;
				}
			}
			else
			{
				if (i < 200)
					frameBuffer16[i] = i * canvas_pitch + 0x100; /* word offset of line */
				else
					frameBuffer16[i] = 200 * canvas_pitch + 0x100; /* word offset of line */
			}
		}
		// clear screen
		for (i = 0x100; i < 0x10000; i++)
			frameBuffer16[i] = 0;

		MX = 40;
		MY = 25 / (lineskip + 1);
	}

	Hw32xSetFGColor(255, 31, 31, 31);
	Hw32xSetBGColor(0, 0, 0, 0);
	X = Y = 0;
	init = vmode;
}
void Hw32xSetFGColor(int s, int r, int g, int b)
{
	volatile unsigned short* palette = &MARS_CRAM;
	fgs = s;
	if (s < 0) return;
	fgc = COLOR(r, g, b) | fgp;
	palette[fgs] = fgc;
	new_pri = 1;
}
void Hw32xSetBGColor(int s, int r, int g, int b)
{
	volatile unsigned short* palette = &MARS_CRAM;
	if (s < 0) return;
	bgs = s;
	bgc = COLOR(r, g, b) | bgp;
	palette[bgs] = bgc;
	new_pri = 1;
}
void Hw32xScreenSetXY(int x, int y)
{
	if (x < MX && x >= 0)
		X = x;
	if (y < MY && y >= 0)
		Y = y;
}
int GetX()
{
	return X;
}
int GetY()
{
	return Y;
}
///
//used for tilemapping
///

///
//Used for Printing text
///
extern unsigned char msx[];

//Adjust buff[128] if you want larger text's being printed at once.  Otherwise you'll get some funky characters :P
void Hw32xScreenPrintf(const char* format, ...)
{
	va_list  opt;
	char     buff[128];
	int      n;

	va_start(opt, format);
	n = vsnprintf(buff, (size_t)sizeof(buff), format, opt);
	va_end(opt);
	buff[sizeof(buff) - 1] = 0;

	Hw32xScreenPutsn(buff, n);
}
int Hw32xScreenPutsn(const char* str, int len)
{
	int ret;

	// Flip the framebuffer selection bit and wait for it to take effect
	//MARS_VDP_FBCTL = currentFB ^ 1;
	//while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB) ;
	//currentFB ^= 1;

	ret = Hw32xScreenPrintData(str, len);

	// Flip the framebuffer selection bit and wait for it to take effect
	//MARS_VDP_FBCTL = currentFB ^ 1;
	//while ((MARS_VDP_FBCTL & MARS_VDP_FS) == UNCACHED_CURFB) ;
	//currentFB ^= 1;

	return ret;
}
//Adjust the X and Y if padding is off below.  For instance, X = 0 will start text right at the edge of the screen...not so good when reading
int Hw32xScreenPrintData(const char* buff, int size)
{
	int i;
	char c;

	if (!init)
	{
		return 0;
	}

	for (i = 0; i < size; i++)
	{
		c = buff[i];
		switch (c)
		{
		case '\r':
			X = 0;
			break;
		case '\n':
			X = 0;
			Y++;
			if (Y >= MY)
				Y = 0;
			Hw32xScreenClearLine(Y);
			break;
		case '\t':
			X = (X + 4) & ~3;
			if (X >= MX)
			{
				X = 0;
				Y++;
				if (Y >= MY)
					Y = 0;
				Hw32xScreenClearLine(Y);
			}
			break;
		default:
			Hw32xScreenPutChar(X, Y, c);
			X++;
			if (X >= MX)
			{
				X = 0;
				Y++;
				if (Y >= MY)
					Y = 0;
				Hw32xScreenClearLine(Y);
			}
		}
	}

	return i;
}
void Hw32xScreenClearLine(int Y)
{
	int i;

	for (i = 0; i < MX; i++)
	{
		Hw32xScreenPutChar(i, Y, ' ');
	}
}
void Hw32xScreenPutChar(int x, int y, unsigned char ch)
{
	if (init == MARS_VDP_MODE_256)
	{
		debug_put_char_8(x, y, ch);
	}
	else if (init == MARS_VDP_MODE_32K)
	{
		debug_put_char_16(x, y, ch);
	}
}
void debug_put_char_8(int x, int y, unsigned char ch)
{
	volatile unsigned char* fb = (volatile unsigned char*)&MARS_FRAMEBUFFER;
	int i, j;
	unsigned char* font;
	int vram, vram_ptr;

	if (!init)
	{
		return;
	}

	vram = 0x200 + x * 8;
	vram += (y * 8 * canvas_pitch);

	font = &msx[(int)ch * 8];

	for (i = 0; i < 8; i++, font++)
	{
		vram_ptr = vram;
		for (j = 0; j < 8; j++)
		{
			if ((*font & (128 >> j)))
				fb[vram_ptr] = 56;//fgs; //Need to change this to be more dynamic.  56 is the index value of white with my palette
			else
				fb[vram_ptr] = bgs;
			vram_ptr++;
		}
		vram += canvas_pitch;
	}
}
void debug_put_char_16(int x, int y, unsigned char ch)
{
	volatile unsigned short* fb = &MARS_FRAMEBUFFER;
	int i, j;
	unsigned char* font;
	int vram, vram_ptr;

	if (!init)
	{
		return;
	}

	vram = 0x100 + x * 8;
	vram += (y * 8 * canvas_pitch);

	font = &msx[(int)ch * 8];

	for (i = 0; i < 8; i++, font++)
	{
		vram_ptr = vram;
		for (j = 0; j < 8; j++)
		{
			if ((*font & (128 >> j)))
				fb[vram_ptr] = fgc;
			else
				fb[vram_ptr] = bgc;
			vram_ptr++;
		}
		vram += canvas_pitch;
	}
}
///
//Used for printing text
///

void pri_vbi_handler(void) MARS_ATTR_DATA_CACHE_ALIGN;

void Mars_WaitFrameBuffersFlip(void)
{
	while ((MARS_VDP_FBCTL & MARS_VDP_FS) != mars_activescreen);
}

void Mars_FlipFrameBuffers(char wait)
{
	mars_activescreen = !mars_activescreen;
	MARS_VDP_FBCTL = mars_activescreen;
	if (wait) Mars_WaitFrameBuffersFlip();
}

char Mars_FramebuffersFlipped(void)
{
	return (MARS_VDP_FBCTL & MARS_VDP_FS) == mars_activescreen;
}

void Mars_InitLineTable(void)
{
	int j;
	int blank;
	int offset = 0; // 224p or 240p
	volatile unsigned short* lines = &MARS_FRAMEBUFFER;

	// initialize the lines section of the framebuffer

	if (mars_requested_lines == -240)
	{
		// letterboxed 240p
		offset = (240 - 224) / 2;
	}

	for (j = 0; j < mars_framebuffer_height; j++)
		lines[offset+j] = j * 320 / 2 + 0x100;

	blank = j * 320 / 2;

	// set the rest of the line table to a blank line
	for (; j < 256; j++)
		lines[offset+j] = blank + 0x100;

	for (j = 0; j < offset; j++)
		lines[j] = blank + 0x100;

	// make sure blank line is clear
	for (j = blank; j < (blank + 160); j++)
		lines[j] = 0;
}

void Mars_SetBrightness(int16_t brightness)
{
	mars_brightness = brightness;
}

int Mars_BackBuffer(void) {
	return mars_activescreen;
}

char Mars_UploadPalette(const uint8_t* palette)
{
	int	i;
	unsigned short* cram = (unsigned short *)&MARS_CRAM;
	int16_t br = mars_brightness;

	if ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0)
		return 0;

	for (i = 0; i < 256; i++) {
		int16_t b = br + *palette++;
		int16_t g = br + *palette++;
		int16_t r = br + *palette++;
		int16_t a = br + *palette++;

		if (r < 0) r = 0; else if (r > 255) r = 255;
		if (g < 0) g = 0; else if (g > 255) g = 255;
		if (b < 0) b = 0; else if (b > 255) b = 255;

		unsigned short b1 = ((b >> 3) & 0x1f) << 10;
		unsigned short g1 = ((g >> 3) & 0x1f) << 5;
		unsigned short r1 = ((r >> 3) & 0x1f) << 0;
		cram[i] = r1 | g1 | b1;
	}

	return 1;
}

int Mars_PollMouse(void)
{
	unsigned int mouse1, mouse2;
	int port = mars_mouseport;

	if (port < 0)
		return -1;

	while (MARS_SYS_COMM0); // wait until 68000 has responded to any earlier requests
	MARS_SYS_COMM0 = 0x0500 | port; // tells 68000 to read mouse
	while (MARS_SYS_COMM0 == (0x0500 | port)); // wait for mouse value

	mouse1 = MARS_SYS_COMM0;
	mouse2 = MARS_SYS_COMM2;
	MARS_SYS_COMM0 = 0; // tells 68000 we got the mouse value

	return (int)((mouse1 << 16) | mouse2);
}

int Mars_ParseMousePacket(int mouse, int* pmx, int* pmy)
{
	int mx, my;

	// (YO XO YS XS S  M  R  L  X7 X6 X5 X4 X3 X2 X1 X0 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0)

	mx = ((unsigned)mouse >> 8) & 0xFF;
	// check overflow
	if (mouse & 0x00400000)
		mx = (mouse & 0x00100000) ? -256 : 256;
	else if (mouse & 0x00100000)
		mx |= 0xFFFFFF00;

	my = mouse & 0xFF;
	// check overflow
	if (mouse & 0x00800000)
		my = (mouse & 0x00200000) ? -256 : 256;
	else if (mouse & 0x00200000)
		my |= 0xFFFFFF00;

	*pmx = mx;
	*pmy = my;

	return mouse;
}

int Mars_GetFRTCounter(void)
{
	unsigned int cnt = SH2_WDT_RTCNT;
	return (int)((mars_pwdt_ovf_count << 8) | cnt);
}

void Mars_InitVideo(int lines)
{
	int i;
	char NTSC;
	int mars_lines = lines == 240 || lines == -240 ? MARS_240_LINES : MARS_224_LINES;

	while ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0);

	MARS_VDP_DISPMODE = mars_lines | MARS_VDP_MODE_256;
	NTSC = (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT) != 0;

	// change 4096.0f to something else if WDT TCSR is changed!
	mars_frtc2msec_frac = 4096.0f * 1000.0f / (NTSC ? NTSC_CLOCK_SPEED : PAL_CLOCK_SPEED) * 65536.0f;

	mars_refresh_hz = NTSC ? 60 : 50;
	mars_requested_lines = lines;
	mars_framebuffer_height = lines == 240 ? 240 : 224;
	mars_activescreen = MARS_VDP_FBCTL;

	Mars_FlipFrameBuffers(1);

	for (i = 0; i < 2; i++)
	{
		volatile int* p, * p_end;

		//Mars_InitLineTable();
		Hw32xUpdateLineTable(0, 0, 0);

		p = (int*)(&MARS_FRAMEBUFFER + 0x100);
		p_end = (int*)p + 320 / 4 * mars_framebuffer_height;
		do {
			*p = 0;
		} while (++p < p_end);

		Mars_FlipFrameBuffers(1);
	}
}

/// <summary>
/// Unlike the Mars_Init, this Init is simple and doesn't init for other things like CD.  This Init ONLY works with tilemapper and Hw32 printing functions
/// </summary>
/// <param name=""></param>
void Mars_Simple_Init(void)
{
	char NTSC;

	Hw32xInit(MARS_VDP_MODE_256, 0);

	SetSH2SR(1);

	while ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0);

	NTSC = (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT) != 0;

	SH2_WDT_WTCSR_TCNT = 0x5A00; // WDT TCNT = 0 
	SH2_WDT_WTCSR_TCNT = 0xA53E; // WDT TCSR = clr OVF, IT mode, timer on, clksel = Fs/4096 

	//init hires timer system
	SH2_WDT_VCR = (65 << 8) | (SH2_WDT_VCR & 0x00FF); // set exception vector for WDT
	SH2_INT_IPRA = (SH2_INT_IPRA & 0xFF0F) | 0x0020; // set WDT INT to priority 2

	// change 4096.0f to something else if WDT TCSR is changed!
	mars_frtc2msec_frac = 4096.0f * 1000.0f / (NTSC ? 23011360 : 22801467) * 65536.0f;

	MARS_SYS_COMM4 = 0;
	MARS_SYS_COMM6 = 0;
}

void Mars_Init(void)
{
	Mars_InitVideo(224);
	Mars_SetMDColor(1, 0);

	//SH2_WDT_WTCSR_TCNT = 0xA518; /* WDT TCSR = clr OVF, IT mode, timer off, clksel = Fs/2 */
	SH2_WDT_WTCSR_TCNT = 0x5A00; // WDT TCNT = 0 
	SH2_WDT_WTCSR_TCNT = 0xA53E; // WDT TCSR = clr OVF, IT mode, timer on, clksel = Fs/4096 

	/* init hires timer system */
	SH2_WDT_VCR = (65<<8) | (SH2_WDT_VCR & 0x00FF); // set exception vector for WDT
	SH2_INT_IPRA = (SH2_INT_IPRA & 0xFF0F) | 0x0020; // set WDT INT to priority 2

	MARS_SYS_COMM4 = 0;

	/* detect input devices */
	Mars_DetectInputDevices();

	Mars_UpdateCD();

	if (mars_cd_ok && !(mars_cd_ok & 0x2))
	{
		/* if the CD is present and it's */
		/* not an MD+, give it seconds to init */
		Mars_WaitTicks(180);
	}
}

uint16_t* Mars_FrameBufferLines(void)
{
	uint16_t* lines = (uint16_t*)&MARS_FRAMEBUFFER;
	if (mars_requested_lines == -240)
		lines += (240 - 224) / 2;
	return lines;
}

void pri_vbi_handler(void)
{
	mars_vblank_count++;

	if (mars_newpalette)
	{
		if (Mars_UploadPalette(mars_newpalette))
			mars_newpalette = NULL;
	}

	Mars_DetectInputDevices();
}

void Mars_ReadSRAM(uint8_t * buffer, int offset, int len)
{
	uint8_t *ptr = buffer;

	while (MARS_SYS_COMM0);
	while (len-- > 0) {
		MARS_SYS_COMM2 = offset++;
		MARS_SYS_COMM0 = 0x0100;    /* Read SRAM */
		while (MARS_SYS_COMM0);
		*ptr++ = MARS_SYS_COMM2 & 0x00FF;
	}
}

void Mars_WriteSRAM(const uint8_t* buffer, int offset, int len)
{
	const uint8_t *ptr = buffer;

	while (MARS_SYS_COMM0);
	while (len-- > 0) {
		MARS_SYS_COMM2 = offset++;
		MARS_SYS_COMM0 = 0x0200 | *ptr++;    /* Write SRAM */
		while (MARS_SYS_COMM0);
	}
}

void Mars_UpdateCD(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0600;
	while (MARS_SYS_COMM0);
	mars_cd_ok = MARS_SYS_COMM2;
	mars_num_cd_tracks = mars_cd_ok >> 2;
	mars_cd_ok = mars_cd_ok & 0x3;
}

void Mars_UseCD(int usecd)
{
	while (MARS_SYS_COMM0);

	if (!mars_cd_ok)
		return;

	MARS_SYS_COMM2 = usecd & 1;
	MARS_SYS_COMM0 = 0x0700;
	while (MARS_SYS_COMM0);
}

void Mars_PlayTrack(char usecd, int playtrack, void *vgmptr, int vgmsize, char looping)
{
	Mars_UseCD(usecd);

	if (!usecd)
	{
		int i;
		uint16_t s[4];

		s[0] = (uintptr_t)vgmsize>>16, s[1] = (uintptr_t)vgmsize&0xffff;
		s[2] = (uintptr_t)vgmptr >>16, s[3] = (uintptr_t)vgmptr &0xffff;

		for (i = 0; i < 4; i++) {
			MARS_SYS_COMM2 = s[i];
			MARS_SYS_COMM0 = 0x0301+i;
			while (MARS_SYS_COMM0);
		}
	}

	MARS_SYS_COMM2 = playtrack | (looping ? 0x8000 : 0x0000);
	MARS_SYS_COMM0 = 0x0300; /* start music */
	while (MARS_SYS_COMM0);
}

void Mars_StopTrack(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0400; /* stop music */
	while (MARS_SYS_COMM0);
}

void Mars_SetMusicVolume(uint8_t volume)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1800|volume;
	while (MARS_SYS_COMM0);
}

void Mars_WaitTicks(int ticks)
{
	unsigned ticend = mars_vblank_count + ticks;
	while (mars_vblank_count < ticend);
}

/*
 *  MD network functions
 */

static inline unsigned short GetNetByte(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1200;	/* get a byte from the network */
	while (MARS_SYS_COMM0);
	return MARS_SYS_COMM2;		/* status:byte */
}

/*
 *  Get a byte from the network. The number of ticks to wait for a byte
 *  is passed in. A wait time of 0 means return immediately. The return
 *  value is -2 for a timeout/no bytes are waiting, -1 if a network error
 *  occurred, and 0 to 255 for a received byte.
 */
int Mars_GetNetByte(int wait)
{
	unsigned short ret;
	unsigned ticend;

	if (!wait)
	{
		/* no wait - return a value immediately */
		ret = GetNetByte();
		return (ret == 0xFF00) ? -2 : (ret & 0xFF00) ? -1 : (int)(ret & 0x00FF);
	}

	/* quick check for byte in rec buffer */
	ret = GetNetByte();
	if (ret != 0xFF00)
		return (ret & 0xFF00) ? -1 : (int)(ret & 0x00FF);

	/* nothing waiting - do timeout loop */
	ticend = mars_vblank_count + wait;
	while (mars_vblank_count < ticend)
	{
		ret = GetNetByte();
		if (ret == 0xFF00)
			continue;	/* no bytes waiting */
		/* GetNetByte returned a byte or a net error */
		return (ret & 0xFF00) ? -1 : (int)(ret & 0x00FF);
	}
	return -2;	/* timeout */
}

/*
 *  Put a byte to the network. Returns -1 if timeout.
 */
int Mars_PutNetByte(int val)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1300 | (val & 0x00FF);	/* send a byte to the network */
	while (MARS_SYS_COMM0);
	return (MARS_SYS_COMM2 == 0xFFFF) ? -1 : 0;
}

void Mars_SetupNet(int type)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1400 | (type & 255);		/* init joyport 2 for networking */
	while (MARS_SYS_COMM0);
}

void Mars_CleanupNet(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1500;		/* cleanup networking */
	while (MARS_SYS_COMM0);
}

void Mars_SetNetLinkTimeout(int timeout)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM2 = timeout;
	MARS_SYS_COMM0 = 0x1700;
	while (MARS_SYS_COMM0);
}


/*
 *  MD video debug functions
 */
void Mars_SetMDCrsr(int x, int y)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM2 = (x<<6)|y;
	MARS_SYS_COMM0 = 0x0800;			/* set current md cursor */
}

void Mars_GetMDCrsr(int *x, int *y)
{
	unsigned t;
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0900;			/* get current md cursor */
	while (MARS_SYS_COMM0);
	t = MARS_SYS_COMM2;
	*y = t & 31;
	*x = t >> 6;
}

void Mars_SetMDColor(int fc, int bc)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0A00 | (bc << 4) | fc;			/* set font fg and bg colors */
}

void Mars_GetMDColor(int *fc, int *bc)
{
	while (MARS_SYS_COMM0);
	for (MARS_SYS_COMM0 = 0x0B00; MARS_SYS_COMM0;);		/* get font fg and bg colors */
	*fc = (unsigned)(MARS_SYS_COMM2 >> 0) & 15;
	*bc = (unsigned)(MARS_SYS_COMM2 >> 4) & 15;
}

void Mars_SetMDPal(int cpsel)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0C00 | cpsel;	/* set palette select */
}

void Mars_MDPutChar(char chr)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0D00 | chr;		/* put char at current cursor pos */
}

void Mars_ClearNTA(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0E00;			/* clear name table a */
}

void Mars_MDPutString(char *str)
{
	while (*str)
		Mars_MDPutChar(*str++);
}

void Mars_DebugStart(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x0F00;			/* start debug queue */
}

void Mars_DebugQueue(int id, short val)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM2 = val;
	MARS_SYS_COMM0 = 0x1000 | id;		/* queue debug entry */
}

void Mars_DebugEnd(void)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1100;			/* end debug queue and display */
}

void Mars_SetBankPage(int bank, int page)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1600 | (page<<3) | bank;
	while (MARS_SYS_COMM0);
}

void Mars_SetBankPageSec(int bank, int page)
{
	volatile unsigned short bcomm4 = MARS_SYS_COMM4;

	MARS_SYS_COMM4 = 0x1600 | (page<<3) | bank;
	while (MARS_SYS_COMM4 != 0x1000);

	MARS_SYS_COMM4 = bcomm4;
}

int Mars_ROMSize(void)
{
	return *((volatile uint32_t *)0x020001a4) - *((volatile uint32_t *)0x020001a0) + 1;
}

void Mars_DetectInputDevices(void)
{
	unsigned i;
	volatile uint16_t *addr = (volatile uint16_t *)&MARS_SYS_COMM12;

	mars_mouseport = -1;
	for (i = 0; i < 2; i++)
		mars_gamepadport[i] = -1;

	for (i = 0; i < 2; i++)
	{
		int val = *addr++;
		if (val == 0xF000)
		{
			mars_controlval[i] = 0;
			continue;	// nothing here
		}
		if (val == 0xF001)
		{
			mars_mouseport = i;
			mars_controlval[i] = 0;
		}
		else
		{
			mars_gamepadport[i] = i;
			mars_controlval[i] |= val;
		}
	}

	/* swap controller 1 and 2 around if the former isn't present */
	if (mars_gamepadport[0] < 0 && mars_gamepadport[1] >= 0)
	{
		mars_gamepadport[0] = mars_gamepadport[1];
		mars_gamepadport[1] = -1;
	}
}

int Mars_ReadController(int port)
{
	int val;
	if (port < 0)
		return -1;
	val = mars_controlval[port];
	mars_controlval[port] = 0;
	return val;
}

void Mars_CtlMDVDP(int sel)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM0 = 0x1900 | (sel & 0x00FF);
	while (MARS_SYS_COMM0);
}

void Mars_StoreWordColumnInMDVRAM(int c)
{
	while (MARS_SYS_COMM0);
	MARS_SYS_COMM2 = c;
	MARS_SYS_COMM0 = 0x1A00;		/* sel = to VRAM, offset in comm2, start move */
}

void Mars_LoadWordColumnFromMDVRAM(int c, int offset, int len)
{
	while (MARS_SYS_COMM0 != 0);
	MARS_SYS_COMM2 = c;
	MARS_SYS_COMM0 = 0x1A01;		/* sel = to VRAM, offset in comm2, start move */
	while (MARS_SYS_COMM0 != 0x9A00);

	MARS_SYS_COMM2 = (((uint16_t)len)<<8) | offset;  /* (length<<8)|offset */
	MARS_SYS_COMM0 = 0x1A01;		/* sel = to VRAM, offset in comm2, start move */
}

void Mars_SwapWordColumnWithMDVRAM(int c)
{
    while (MARS_SYS_COMM0);
    MARS_SYS_COMM2 = c;
    MARS_SYS_COMM0 = 0x1A02;        /* sel = swap with VRAM, column in comm2, start move */
}

void Mars_Finish(void)
{
	while (MARS_SYS_COMM0 != 0);
}
