
#include "GameDef.h"
#include "mars.h"
#include "mars_ringbuf.h"


#define MAX_SAMPLES        316		// 70Hz

#define SAMPLE_RATE      22050
#define SAMPLE_MIN       2
#define SAMPLE_MAX       1032
#define SAMPLE_CENTER    (SAMPLE_MAX-SAMPLE_MIN)/2

#define SPATIALIZATION_RATE		15 // Hz
// this many samples between each spatialization call
#define SPATIALIZATION_SRATE	(int)(MAX_SAMPLES*(((float)SAMPLE_RATE/MAX_SAMPLES)/SPATIALIZATION_RATE))

// when to clip out sounds
// Does not fit the large outdoor areas.

#define S_CLIPPING_DIST (1224 * FRACUNIT)

#define S_STEREO_SWING (96 * FRACUNIT)
#define S_CLOSE_DIST (200 * FRACUNIT)
#define S_ATTENUATOR (S_CLIPPING_DIST - S_CLOSE_DIST)

int __attribute__((aligned(16))) pcm_data[4];
int16_t __attribute__((aligned(16))) snd_buffer[2][MAX_SAMPLES * 2];
static uint8_t snd_bufidx = 0;
static uint8_t	snd_init = 0;
static marsrb_t	soundcmds = { 0 };
unsigned        snd_nopaintcount = 0;

sfxchannel_t	pcmchannel;
sfxchannel_t	sfxchannels[SFXCHANNELS];
uint8_t* vgm_ptr;
VINT 			sfxvolume = 64;	/* range 0 - 64 */
VINT 			musicvolume = 64;	/* range 0 - 64 */
int             samplecount = 0;

//some declarations
static void S_StartSoundReal(mobj_t* mobj, unsigned sound_id, int vol, getsoundpos_t getpos) ATTR_DATA_CACHE_ALIGN;
static void S_PaintChannel(sfxchannel_t* ch, int16_t* buffer) ATTR_DATA_CACHE_ALIGN;
int S_PaintChannel4IMA(void* mixer, int16_t* buffer, int32_t cnt, int32_t scale) ATTR_DATA_CACHE_ALIGN;
int S_PaintChannel4IMA2x(void* mixer, int16_t* buffer, int32_t cnt, int32_t scale) ATTR_DATA_CACHE_ALIGN;
void S_PaintChannel8(void* mixer, int16_t* buffer, int32_t cnt, int32_t scale) ATTR_DATA_CACHE_ALIGN;
static void S_SpatializeAt(fixed_t* origin, mobj_t* listener, int* pvol, int* psep) ATTR_DATA_CACHE_ALIGN;
static void S_Spatialize(mobj_t* mobj, int* pvol, int* psep, getsoundpos_t getpos) ATTR_DATA_CACHE_ALIGN;
static void S_ClearPCM(void);

#define finesine(x) finesine_[x]

// WAV

#define S_LE_SHORT(chunk) (((chunk)[0]>>8)|(((chunk)[0]&0xff) << 8))
#define S_LE_LONG(chunk)  (S_LE_SHORT(chunk) | (S_LE_SHORT(chunk+1)<<16))

#define S_WAV_FORMAT_PCM         0x1
#define S_WAV_FORMAT_IMA_ADPCM   0x11
#define S_WAV_FORMAT_EXTENSIBLE  0xfffe

//all sounds loaded from cart
extern char sfx1[], sfx2[], sfx3[];

void S_Init(void)
{
	Mars_RB_ResetAll(&soundcmds);

	Mars_InitSoundDMA();
}

enum
{
	SNDCMD_NONE,
	SNDCMD_CLEAR,
	SNDCMD_STARTSND,
	SNDCMD_STARTORGSND
};

void pri_cmd_handler(void)
{
	volatile int* pcm_cachethru = (volatile int*)((intptr_t)pcm_data | 0x20000000);
	unsigned int offs, len, freq;

	((volatile short*)pcm_cachethru)[7] = 0;	/* make sure data array isn't being read */

	// check comm0 for command
	if (MARS_SYS_COMM0 == 0xFFFF)
	{
		/* stop pcm channel */
		pcm_cachethru[2] = -1;
		pcm_cachethru[3] = 1;
	}
	else
	{
		freq = (unsigned)MARS_SYS_COMM0;

		/* top 8 bits of COMM2 */
		offs = ((unsigned)(MARS_SYS_COMM2 & 0x00FF) << 16);
		/* lower 8 bits of COMM2 */
		len = ((unsigned)(MARS_SYS_COMM2 & 0xFF00) << 8);

		/* handshake */
		for (MARS_SYS_COMM0 = 0; MARS_SYS_COMM0 == 0; );

		offs = offs | (unsigned)MARS_SYS_COMM2;
		/* COMM0 has the LSB set to 1 */
		len = len | ((unsigned)MARS_SYS_COMM0 >> 1);

		/* limit offset and length to 1M since that's our block size */
		offs &= 0xFFFFF;
		len &= 0xFFFFF;

		pcm_cachethru[0] = (freq << 14) / SAMPLE_RATE;
		pcm_cachethru[1] = len << 14;
		pcm_cachethru[2] = offs;
		pcm_cachethru[3] = 1;
	}

	// done
	MARS_SYS_COMM0 = 0xA55A;					/* handshake with code */
	while (MARS_SYS_COMM0 == 0xA55A);
}

void sec_cmd_handler(void)
{
	volatile unsigned short bcomm4 = MARS_SYS_COMM4;	/* save COMM4 reg */

	// done
	MARS_SYS_COMM4 = 0xA55A;					/* handshake with code */
	while (MARS_SYS_COMM4 == 0xA55A);

	MARS_SYS_COMM4 = bcomm4;					/* restore COMM4 reg */
}

void sec_dma1_handler(void)
{
	SH2_DMA_CHCR1; // read TE
	SH2_DMA_CHCR1 = 0; // clear TE

	// start DMA on buffer and fill the other one
	SH2_DMA_SAR1 = (uintptr_t)snd_buffer[snd_bufidx];
	SH2_DMA_TCR1 = MAX_SAMPLES; // number longs
	SH2_DMA_CHCR1 = 0x18E5; // dest fixed, src incr, size long, ext req, dack mem to dev, dack hi, dack edge, dreq rising edge, cycle-steal, dual addr, intr enabled, clear TE, dma enabled

	snd_bufidx ^= 1; // flip audio buffer

	Mars_Sec_ReadSoundCmds();

	S_Update(snd_buffer[snd_bufidx]);
}


static void S_StartSoundReal(mobj_t* mobj, unsigned sound_id, int vol, getsoundpos_t getpos)
{
	sfxchannel_t* channel, * newchannel;
	int i;
	int length, loop_length;
	sfxinfo_t* sfx;
	sfx_t* md_data;

	//Now we assign sfx based upon sound_id
	switch (sound_id) 
	{
		case 1:
			md_data = sfx1;
			break;
		case 2:
			md_data = sfx2;
			break;
		case 3:
			md_data = sfx3;
			break;
	}

	newchannel = NULL;
	//sfx = &S_sfx[sound_id];
	//md_data = W_POINTLUMPNUM(sfx->lump);
	length = md_data->samples;
	loop_length = 0;

	if (length < 4)
		return;

	// reject sounds started at the same instant and singular sounds
	for (channel = sfxchannels, i = 0; i < SFXCHANNELS; i++, channel++)
	{
		if (channel->sfx == sfx)
		{
			if (channel->position <= 0) // ADPCM has the position set to -1 initially
			{
				if (channel->volume < vol)
				{
					newchannel = channel;
					goto gotchannel;	//overlay this
				}
				return;		// exact sound allready started
			}

			if (sfx->singularity)
			{
				newchannel = channel;	//overlay this
				goto gotchannel;
			}
		}
		if (mobj && channel->mobj == mobj)
		{	//cut off whatever was coming from this origin 
			newchannel = channel;
			goto gotchannel;
		}

		if (channel->data == NULL)
			newchannel = channel;	//this is a dead channel, ok to reuse
	}

	//if there weren't any dead channels, try to kill an equal or lower 
	// priority channel

	if (!newchannel)
	{
		for (newchannel = sfxchannels, i = 0; i < SFXCHANNELS; i++, newchannel++)
			if (newchannel->sfx->priority >= sfx->priority)
				goto gotchannel;
		return;		// couldn't override a channel
	}

	//
	// fill in the new values
	//
gotchannel:
	if (length == 0x52494646) {	// RIFF
		// find the format chunk
		uint16_t* chunk = (uint16_t*)((char*)md_data + 12);
		uint16_t* end = (uint16_t*)((char*)md_data + 0x40 - 4);
		int format = 0;

		// set default sample rate and block size
		newchannel->increment = (11025 << 14) / SAMPLE_RATE;
		newchannel->block_size = 256;

		while (chunk < end) {
			// a long value in little endian format
			length = S_LE_LONG(&chunk[2]);

			if (chunk[0] == 0x6461 && chunk[1] == 0x7461) // 'data'
				break;

			if (chunk[0] == 0x666D && chunk[1] == 0x7420) // 'fmt '
			{
				int sample_rate = S_LE_LONG(&chunk[6]);
				int block_align = S_LE_SHORT(&chunk[10]);

				format = S_LE_SHORT(&chunk[4]);
				if (format == S_WAV_FORMAT_EXTENSIBLE && length == 40) {
					format = S_LE_LONG(&chunk[16]); // sub-format
				}

				// increment = (SampleRate << 14) / mixer sample rate
				if (sample_rate > SAMPLE_RATE)
					newchannel->increment = 1 << 14; // limit increment to max of 1.0
				else
					newchannel->increment = (sample_rate << 14) / SAMPLE_RATE;
				newchannel->block_size = block_align;
			}

			chunk += 4 + ((length + 1) >> 1);
		}

		if (chunk >= end)
			return;
		
		newchannel->data = (uint8_t*)&chunk[4];
		if (format == S_WAV_FORMAT_PCM) {
			newchannel->length = length << 14;
			newchannel->loop_length = 0;
			newchannel->width = 8;
			newchannel->position = 0;
		}
		else if (format == S_WAV_FORMAT_IMA_ADPCM) {
			newchannel->remaining_bytes = length;
			newchannel->length = 0;
			newchannel->loop_length = 0;
			newchannel->width = 4;
			newchannel->position = -1;
		}
		else {
			return;
		}
	}
	else {
		newchannel->increment = (11025 << 14) / SAMPLE_RATE;
		newchannel->length = length << 14;
		newchannel->loop_length = loop_length << 14;
		newchannel->data = &md_data->data[0];
		newchannel->width = 8;
		newchannel->position = 0;

	}

	newchannel->sfx = sfx;
	newchannel->mobj = mobj;
	newchannel->getpos = getpos;

	// volume and panning will be updated later in S_Spatialize
	newchannel->volume = vol;
	newchannel->pan = 128;

	// cache-through access the sample data
	//newchannel->data = (void *)((uintptr_t)newchannel->data | 0x20000000);
}


void Mars_Sec_InitSoundDMA(void)
{
	uint16_t sample, ix;

	Mars_ClearCache();

	Mars_RB_ResetRead(&soundcmds);

	// init DMA
	SH2_DMA_SAR1 = 0;
	SH2_DMA_DAR1 = (uint32_t)&MARS_PWM_STEREO; // storing a long here will set the left and right channels
	SH2_DMA_TCR1 = 0;
	SH2_DMA_CHCR1 = 0;
	SH2_DMA_DRCR1 = 0;

	// init the sound hardware
	MARS_PWM_MONO = 1;
	MARS_PWM_MONO = 1;
	MARS_PWM_MONO = 1;
	if (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT)
		MARS_PWM_CYCLE = (((23011361 << 1) / (SAMPLE_RATE)+1) >> 1) + 1; // for NTSC clock
	else
		MARS_PWM_CYCLE = (((22801467 << 1) / (SAMPLE_RATE)+1) >> 1) + 1; // for PAL clock
	MARS_PWM_CTRL = 0x0185; // TM = 1, RTP, RMD = right, LMD = left

	sample = SAMPLE_MIN;

	// ramp up to SAMPLE_CENTER to avoid click in audio (real 32X)
	while (sample < SAMPLE_CENTER)
	{
		for (ix = 0; ix < (SAMPLE_RATE * 2) / (SAMPLE_CENTER - SAMPLE_MIN); ix++)
		{
			while (MARS_PWM_MONO & 0x8000); // wait while full
			MARS_PWM_MONO = sample;
		}
		sample++;
	}

	snd_bufidx = 0;
	snd_init = 1;

	Mars_Sec_StartSoundMixer();
}

void Mars_Sec_StartSoundMixer(void)
{
	snd_nopaintcount = 0;

	S_ClearPCM();

	// fill first buffer
	S_Update(snd_buffer[snd_bufidx]);

	// start DMA
	sec_dma1_handler();
}

static void S_ClearPCM(void)
{
	D_memset(pcm_data, 0, sizeof(pcm_data));
	D_memset(&pcmchannel, 0, sizeof(pcmchannel));
	pcmchannel.volume = /*musicvolume*/40; /* 64 seems to be too loud */
	pcmchannel.pan = 128;
}

static void S_PaintChannel(sfxchannel_t* ch, int16_t* buffer)
{
	if (!ch->data)
		return;

	if (ch->width == 4)
	{
		int i = MAX_SAMPLES;
		int32_t* end = (int32_t*)buffer + MAX_SAMPLES;
		do
		{
			if (ch->position >= ch->length)
			{
				// advance to next block
				ch->data += ch->block_size - 3;
				ch->length = 0;
			}

			if (!ch->length)
			{
				uint8_t* block = (uint8_t*)ch->data;
				int block_size = ch->block_size;

				if (block_size > ch->remaining_bytes)
					block_size = ch->remaining_bytes;
				if (block_size < 4)
				{
					// EOF
					ch->data = NULL;
					break;
				}

				ch->position = 1 << 14;
				ch->prev_pos = -1; // force output of initial predictor
				ch->length = ((block_size - 3) << 1) << 14;
				// initial step_index : initial predictor
				ch->loop_length = ((unsigned)block[2] << 16) | ((unsigned)block[1] << 8) | block[0];
				ch->data += 3;
				ch->remaining_bytes -= block_size;
			}

			if ((ch->increment < (1 << 14)) && !(ch->increment & (ch->increment - 1))) {
				// optimized 2x upsampling mixer: from 11kHz to 22kHz or 44kHz
				i = S_PaintChannel4IMA2x(ch, (int16_t*)(end - i), i, 64);
			}
			else {
				// general mixing routine
				i = S_PaintChannel4IMA(ch, (int16_t*)(end - i), i, 64);
			}
		} while (i > 0);
	}
	else
	{
		S_PaintChannel8(ch, buffer, MAX_SAMPLES, 64);
		if (ch->position >= ch->length)
		{
			ch->data = NULL;
		}
	}
}


void Mars_Sec_ReadSoundCmds(void)
{
	if (!snd_init)
		return;

	while (Mars_RB_Len(&soundcmds) >= 8) {
		short* p = Mars_RB_GetReadBuf(&soundcmds, 8);

		int cmd = *p;
		switch (cmd) {
		case SNDCMD_CLEAR:
			D_memset(sfxchannels, 0, sizeof(*sfxchannels) * SFXCHANNELS);
			S_ClearPCM();
			break;
		case SNDCMD_STARTSND:
			S_StartSoundReal((void*)(*(intptr_t*)&p[2]), p[1], p[4], NULL);
			break;
		case SNDCMD_STARTORGSND:
			S_StartSoundReal((void*)(*(intptr_t*)&p[2]), p[1], p[6], (getsoundpos_t)(*(intptr_t*)&p[4]));
			break;
		}

		Mars_RB_CommitRead(&soundcmds);
	}
}

static void S_StartSoundEx(mobj_t* mobj, int sound_id, getsoundpos_t getpos)
{
	int vol, sep;
	sfxinfo_t* sfx;

	/* Get sound effect data pointer */
	if (sound_id <= 0 || sound_id >= NUMSFX)
		return;

	//sfx = &S_sfx[sound_id]; //TODO: add array for sounds
	if (sfx->lump < 0)
		return;

	/* */
	/* spatialize */
	/* */
	S_Spatialize(mobj, &vol, &sep, getpos);
	if (!vol)
		return; /* too far away */

	// HACK: boost volume for item pickups
	if (sound_id == sfx_itemup)
		vol <<= 1;

	uint16_t* p = (uint16_t*)Mars_RB_GetWriteBuf(&soundcmds, 8, false);
	if (!p)
		return;

	if (getpos)
	{
		*p++ = SNDCMD_STARTORGSND;
		*p++ = sound_id;
		*(int*)p = (intptr_t)mobj, p += 2;
		*(int*)p = (intptr_t)getpos, p += 2;
		*p++ = vol;
	}
	else
	{
		*p++ = SNDCMD_STARTSND;
		*p++ = sound_id;
		*(int*)p = (intptr_t)mobj, p += 2;
		*p++ = vol;
	}

	Mars_RB_CommitWrite(&soundcmds);
}

void S_StartSound(mobj_t* mobj, int sound_id)
{
	S_StartSoundEx(mobj, sound_id, NULL);
}

void S_StartPositionedSound(mobj_t* mobj, int sound_id, getsoundpos_t getpos)
{
	S_StartSoundEx(mobj, sound_id, getpos);
}


static void S_UpdatePCM(void)
{
	int inc, len, offs, flag;

	Mars_ClearCacheLine(pcm_data);
	inc = pcm_data[0]; /* cache read line loads all vars at once to cache */
	len = pcm_data[1];
	offs = pcm_data[2];
	flag = pcm_data[3];

	if (flag & 1)
	{
		if (offs == -1)
		{
			pcmchannel.data = NULL;
			((volatile short*)pcm_data)[7] = 0; // unset bit 0 for flag
			return;
		}

		/* do once */
		pcmchannel.position = 0;
		pcmchannel.increment = inc;
		pcmchannel.length = len;
		pcmchannel.data = vgm_ptr + offs;
		((volatile short*)pcm_data)[7] = 0; // unset bit 0 for flag
	}
}

/*
==================
=
= S_SpatializeAt
=
==================
*/
static void S_SpatializeAt(fixed_t* origin, mobj_t* listener, int* pvol, int* psep)
{
	int dist_approx;
	int dx, dy;
	angle_t angle;
	int	vol, sep;

	dx = D_abs(origin[0] - listener->x);
	dy = D_abs(origin[1] - listener->y);
	dist_approx = dx + dy - ((dx < dy ? dx : dy) >> 1);
	if (dist_approx > S_CLIPPING_DIST)
	{
		vol = 0;
		sep = 128;
	}
	else
	{
		// angle of source to listener
		angle = R_PointToAngle2(listener->x, listener->y,
			origin[0], origin[1]);

		if (angle > listener->angle)
			angle = angle - listener->angle;
		else
			angle = angle + (0xffffffff - listener->angle);
		angle >>= ANGLETOFINESHIFT;

		FixedMul2(sep, S_STEREO_SWING, finesine(angle));
		sep >>= FRACBITS;

		sep = 128 - sep;
		if (sep < 0)
			sep = 0;
		else if (sep > 255)
			sep = 255;

		if (dist_approx < S_CLOSE_DIST)
			vol = sfxvolume;
		else if (dist_approx >= S_CLIPPING_DIST)
			vol = 0;
		else
		{
			vol = sfxvolume * (S_CLIPPING_DIST - dist_approx);
			vol = (unsigned)vol / S_ATTENUATOR;
			if (vol > sfxvolume)
				vol = sfxvolume;
		}
	}

	*pvol = vol;
	*psep = sep;
}

static void S_Spatialize(mobj_t* mobj, int* pvol, int* psep, getsoundpos_t getpos)
{
	int	vol, sep;
	player_t* player = &players[consoleplayer];
	player_t* player2 = &players[consoleplayer ^ 1];

	vol = sfxvolume;
	sep = 128;

	if (mobj)
	{
		fixed_t origin[2];

		if (getpos)
		{
			getpos(mobj, origin);
		}
		else
		{
			origin[0] = mobj->x, origin[1] = mobj->y;
		}

		if (mobj != player->mo)
			S_SpatializeAt(origin, player->mo, &vol, &sep);

		if (splitscreen && player2->mo)
		{
			int vol2, sep2;

			vol2 = sfxvolume;
			sep2 = 255;

			if (mobj != player2->mo)
				S_SpatializeAt(origin, player2->mo, &vol2, &sep2);

			sep = 128 + (vol2 - vol) * 2;
			vol = vol2 > vol ? vol2 : vol;
			if (sep < 0) sep = 0;
			else if (sep > 255) sep = 255;
		}
	}

	*pvol = vol;
	*psep = sep;
}

static void S_Update(int16_t* buffer)
{
	int i;
	int32_t* b2;
	int c, l, h;
	mobj_t* mo;
	boolean spatialize;

	S_UpdatePCM();

	Mars_ClearCacheLine(&sfxvolume);
	Mars_ClearCacheLine(&musicvolume);

	i = 0;
	if (!pcmchannel.data)
	{
		for (i = 0; i < SFXCHANNELS; i++)
		{
			if (sfxchannels[i].data)
				break;
		}
	}

	if (i == SFXCHANNELS)
		snd_nopaintcount++;
	else
		snd_nopaintcount = 0;
	if (snd_nopaintcount > 2)
	{
		// we haven't painted the sound channels to the output buffer
		// for over two frames, both buffers are cleared and safe to
		// point the DMA to
		return;
	}

	b2 = (int32_t*)buffer;
	for (i = 0; i < MAX_SAMPLES / 8; i++)
	{
		*b2++ = 0, * b2++ = 0, * b2++ = 0, * b2++ = 0;
		*b2++ = 0, * b2++ = 0, * b2++ = 0, * b2++ = 0;
	}
	for (i *= 8; i < MAX_SAMPLES; i++)
	{
		*b2++ = 0;
	}

	// keep updating the channel until done 
	S_PaintChannel(&pcmchannel, buffer);

	spatialize = samplecount >= SPATIALIZATION_SRATE;
	if (samplecount >= SPATIALIZATION_SRATE)
		samplecount -= SPATIALIZATION_SRATE;
	samplecount += MAX_SAMPLES;

	if (spatialize)
	{
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t* player = &players[i];
			if (!playeringame[i])
				continue;

			Mars_ClearCacheLine(&player->mo);

			mo = player->mo;
			Mars_ClearCacheLine(&mo->x);
			Mars_ClearCacheLine(&mo->y);
			Mars_ClearCacheLine(&mo->angle);
		}
	}

	for (i = 0; i < SFXCHANNELS; i++)
	{
		sfxchannel_t* ch = &sfxchannels[i];

		if (!ch->data)
			continue;

		mo = ch->mobj;
		if (mo && spatialize)
		{
			int vol, sep;

			//
			//spatialize
			//
			if (!ch->getpos)
			{
				Mars_ClearCacheLine(&mo->x);
				Mars_ClearCacheLine(&mo->y);
			}

			S_Spatialize(mo, &vol, &sep, ch->getpos);

			if (!vol)
			{
				// inaudible
				ch->data = NULL;
				continue;
			}

			ch->volume = vol;
			ch->pan = sep;
		}

		S_PaintChannel(ch, buffer);
	}

#ifdef MARS
	// force GCC into keeping constants in registers as it 
	// is stupid enough to reload them on each loop iteration
	__asm volatile("mov %1,%0\n\t" : "=&r" (c) : "r"(SAMPLE_CENTER));
	__asm volatile("mov %1,%0\n\t" : "=&r" (l) : "r"(SAMPLE_MIN));
	__asm volatile("mov %1,%0\n\t" : "=&r" (h) : "r"(SAMPLE_MAX));
#else
	c = SAMPLE_CENTER, l = SAMPLE_MIN, h = SAMPLE_MAX;
#endif

	// convert buffer from s16 pcm samples to u16 pwm samples
	b2 = (int32_t*)buffer;
	for (i = 0; i < MAX_SAMPLES; i++)
	{
		int s, s1, s2;
		s = (int16_t)(*b2 >> 16) + c;
		s1 = (s < l) ? l : (s > h) ? h : s;
		s = (int16_t)(*b2) + c;
		s2 = (s < l) ? l : (s > h) ? h : s;
		*b2++ = (s1 << 16) | s2;
	}
}