/* marsonly.c */


//#include "doomdef.h"
//#include "r_local.h"
#include "GameDef.h"
#include "mars.h"

//test pulling binarys from .s file with .incbin
//with music, sound, graphics, etc.
extern uint8_t* music;
extern char spritesheettest[];


//void ReadEEProm (void);





/* 
================ 
= 
= Mars_main  
= 
================ 
*/ 
 
int main(void)
{
	Mars_CommSlaveClearCache();

/* */
/* load defaults */
/* */
	//ReadEEProm();

	/* use letter-boxed 240p mode */
	if (Mars_IsPAL())
	{
		Mars_InitVideo(-240);
		Mars_SetMDColor(1, 0);
	}

	//Simplified Init process
	Mars_Init();

	//adjusts the brightness by 1 from 255
	Mars_SetBrightness(1);

	//Loading palette data directly from .bmp file
	unsigned short* p = (unsigned short*)((int)spritesheettest + 0x36); //gets the palette data

	//Loads up the palette
	I_SetPalette(p);
	
	Mars_CommSlaveClearCache();

	//testing music/sound
	//Mars_InitSoundDMA();
	S_Init();
	Mars_PlayTrack(0, 1, &music, (int)sizeof(&music), 1);

	//Init game engine
	game_engine_init();


	//call into game_engine loop below
	run_game_mainloop();

	return 0;
}

void secondary()
{
	Mars_Secondary();
}

