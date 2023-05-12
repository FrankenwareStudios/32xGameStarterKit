#include "GameDef.h"
#include "mars.h"
#include "yatssd/draw.h" //Used for tilemap
#include "yatssd/Tiled/ts1.h" //used for tilemap
//#include "yatssd/Tiled/RPGMap.h" //used for tilemap
#include "gameworld.h"

//for collision.c file (may move to separate collision.h file)
int player_collisions(int x, int y, tilemap_t* tm, gamemap_t* gm, dtilelayer_t* collisionLayer);




//TEST
int flags = DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP ;

//Get's assets
extern char spritesheettest[], background1[], foreground1[], splashscreen[], blackbg[], splashscreen2[], title[], newfont[], dialogBox[], dialogBoxShort[];
unsigned char* fade_palette;
unsigned char* myFonts = (unsigned char*)((int)newfont + 0x36 + 1024);

//fading tics
int start_fade_tic;
int fade_tics;

//game tics
int start_tic;
int tics;

//input handling
int buttons, old_buttons;

//Used as Player's current position.  this is needed to keep all
//NPC sprites drawing in their map position as you walk/scroll through the map
int currentMapX_position, currentMapY_position;
int currentPlayerPos_x;
int currentPlayerPos_y;

//Music
extern uint8_t* musicBattle, music;


//Game States
enum
{
	GAME_TITLE_SCREEN    = 0,
	GAME_SPLASH_SCREEN1  = 1,
	GAME_SPLASH_SCREEN2  = 2,
	GAME_DIALOG          = 4,
	GAME_OVER            = 8,
	GAME_WIN             = 16,
	GAME_BATTLE          = 32,
	GAME_MENU            = 64,
	GAME_MAP_VIEW        = 128
};

int GAME_STATE;

///
//Supports Displaying Tilemaps
///
tilemap_t tm;
gamemap_t currentGameMap;
dtilelayer_t collisionLayer;
int fpcamera_x, fpcamera_y;
int fpmoveinc_x = 1 << 16, fpmoveinc_y = 1 << 16; // in 16.16 fixed point
int window_canvas_x = 0, window_canvas_y = 0;
///
//Supports Displaying tilemaps
///

//for fade
boolean isFading = false;
boolean fadeToggle = false; //test, could be deleted later
boolean resetFading = false;

//for battles
npc_t currentMob;


/// <summary>
/// Initializes the game engine
/// </summary>
void game_engine_init()
{
	//Get the first map from the game world and set the current map
	//Will need to change this for saved state when I introduce that
	currentGameMap = GAME_WORLD.gamemaps[0];

	//inits yatssd tilemaps
	init_tilemap(&tm, &currentGameMap.tilemap, (uint8_t**)currentGameMap.reslist);
	Hw32xScreenFlip(0);

	//Set the game state to startup splash screen
	GAME_STATE = GAME_SPLASH_SCREEN1;

	//set the fade palette for later
	fade_palette = (unsigned short*)((int)spritesheettest + 0x36); //gets the palette data

	//Init fade tics
	start_fade_tic = Mars_GetTicCount();
	fade_tics = 0;

	//Init game tics
	start_tic = Mars_GetTicCount();
	tics = 0;

	//Init for tilemap NPC and player positions
	currentMapX_position = 0;
	currentMapY_position = 0;
	currentPlayerPos_x = currentMapX_position + 128;
	currentPlayerPos_y = currentMapY_position + 112;
	
	//lets get the current map collision layer
	int tl;
	for (tl = 0; tl < tm.numlayers ; tl++)
	{
		//if layer has collison then thats our layer
		if (tm.layers[tl].collisionLayer == 1)
			collisionLayer = tm.layers[tl];
	}

	//TESTING init sprites on current map
	currentGameMap.npcs[0].sprite.npc_tic_start = Mars_GetTicCount();
	currentGameMap.npcs[0].ai_start_tic = Mars_GetTicCount();
}

void load_new_map(int mapID, int player_x, int player_y)
{
	//Get the map from gameworld
	int numMaps = sizeof(game_maps) / sizeof(game_maps[0]);
	int i;
	for (i = 0; i < numMaps; i++)
	{
		//found our map
		if (GAME_WORLD.gamemaps[i].mapID == mapID)
			currentGameMap = GAME_WORLD.gamemaps[i];
	}

	//Init new map
	init_tilemap(&tm, &currentGameMap.tilemap, (uint8_t**)currentGameMap.reslist);
	Hw32xScreenFlip(0);

	//fade out
	if (!isFading)
	{
		fade_display(fade_palette, false);
		Mars_WaitTicks(60); //gives me a little delay before just fading back in
	}

	//set new position for player
	fpcamera_x = player_x;
	fpcamera_y = player_y;
	currentMapX_position = player_x;
	currentMapY_position = player_y;
	currentPlayerPos_x = currentMapX_position + 128;
	currentPlayerPos_y = currentMapY_position + 112;

	//lets get the current map collision layer
	int tl;
	for (tl = 0; tl < tm.numlayers; tl++)
	{
		//if layer has collison then thats our layer
		if (tm.layers[tl].collisionLayer == 1)
			collisionLayer = tm.layers[tl];
	}
}

void run_game_mainloop()
{

	while (1)
	{
		//reset fading
		reset_fading();

		//read buttons pushed
		old_buttons = buttons;
		buttons = I_ReadControls();







		




		//Test sound with hitting a button
		if (buttons & BT_A && !(GAME_STATE & GAME_BATTLE) && !(old_buttons & BT_A))
		{
			//Starts the simple sound.
			//Currently hard coded single sound effect.  Will need to implement Sound array
			S_StartSound(NULL, 1);
		}


		if (buttons & BT_B)
		{
			//Test end battle
			if (GAME_STATE & GAME_BATTLE)
			{
				fade_display(fade_palette, false);
				GAME_STATE &= ~GAME_BATTLE; //this removes

				//set current map music
				Mars_PlayTrack(0, 1, &music, (int)sizeof(&music), 1);
			}
		}


		//Test
		if (buttons & BT_C && !(GAME_STATE & GAME_BATTLE))
		{
			//Testing showing dialog screen
			if (!(GAME_STATE & GAME_DIALOG))
			{
				GAME_STATE &= ~GAME_MAP_VIEW; //this removes
				GAME_STATE |= GAME_DIALOG;
			}

		}

		//check for map collisions
		check_map_collisions();

		//Check for map/world view
		map_screen();

		////Collison Testing (a.left < b.right) && (a.right > b.left) && (a.top < b.bottom) && (a.bottom > b.top)
		//if ((currentPlayerPos_x < testnpc.x + 16) && (currentPlayerPos_x + 16 > testnpc.x) &&
		//	(currentPlayerPos_y < testnpc.y + 16) && (currentPlayerPos_y + 16 > testnpc.y) )
		//	Display_Battle();

		//Check for dialgo screen
		dialog_screen();

		//Check for battle screen
		battle_screen();

		//Check for title menu
		title_screen();

		//Check for splash screen
		splash_screen2();

		//Check for boot screen
		splash_screen();

		Hw32xScreenFlip(0);

	}
}

/// <summary>
/// All logic associated to the world/town map screen presented
/// </summary>
void map_screen()
{
	//camera for tilemap
	int old_fpcamera_x, old_fpcamera_y;
	int clip;

	if (GAME_STATE & GAME_MAP_VIEW)
	{
		if (!(GAME_STATE & GAME_BATTLE))
		{
			if (!resetFading)
				resetFading = true;
		}
		

		//for tilemap
		old_fpcamera_x = fpcamera_x;
		old_fpcamera_y = fpcamera_y;

		//Main player walks down
		if (buttons & BT_DOWN && !(GAME_STATE & GAME_BATTLE))
		{
			//Only advance map position if you are still in the vertical lower bound of the map
			int tempy = currentPlayerPos_y + 1;
			int tempx = currentPlayerPos_x;
			if (player_collisions(tempx,tempy, &tm, &currentGameMap, &collisionLayer) != COLLISION_BLOCK)
			{
				if (currentMapY_position < ((tm.tiles_ver * tm.th) - 224))
				{
					currentMapY_position++;

					//updates player postion used for collision testing
					currentPlayerPos_y = currentMapY_position + 112;

					//Check for random battle encounter
					check_for_battle();
				}

				fpcamera_y += fpmoveinc_y;
			}
		}
		else if (buttons & BT_UP && !(GAME_STATE & GAME_BATTLE))
		{
			int tempy = currentPlayerPos_y - 1;
			int tempx = currentPlayerPos_x;
			if (player_collisions(tempx, tempy, &tm, &currentGameMap, &collisionLayer) != COLLISION_BLOCK)
			{
				//Only advance map position if you are still in the vertical upper bound of the map
				if (currentMapY_position > 0)
				{
					currentMapY_position--;

					//updates player postion used for collision testing
					currentPlayerPos_y = currentMapY_position + 112;

					//Check for random battle encounter
					check_for_battle();
				}

				fpcamera_y -= fpmoveinc_y;
			}
		}

		if (buttons & BT_RIGHT && !(GAME_STATE & GAME_BATTLE))
		{
			int tempy = currentPlayerPos_y;
			int tempx = currentPlayerPos_x + 1;
			if (player_collisions(tempx, tempy, &tm, &currentGameMap, &collisionLayer) != COLLISION_BLOCK)
			{
				//Only advance map position if you are still in the horizontal right bound of the map
				if (currentMapX_position < (tm.tiles_hor * tm.tw) - 320)
				{
					currentMapX_position++;

					//updates player postion used for collision testing
					currentPlayerPos_x = currentMapX_position + 128;

					//Check for random battle encounter
					check_for_battle();
				}

				fpcamera_x += fpmoveinc_x;
			}
		}
		else if (buttons & BT_LEFT && !(GAME_STATE & GAME_BATTLE))
		{
			int tempy = currentPlayerPos_y;
			int tempx = currentPlayerPos_x - 1;
			if (player_collisions(tempx, tempy, &tm, &currentGameMap, &collisionLayer) != COLLISION_BLOCK)
			{
				//Only advance map position if you are still in the horizontal left bound of the map
				if (currentMapX_position > 0)
				{
					currentMapX_position--;

					//updates player postion used for collision testing
					currentPlayerPos_x = currentMapX_position + 128;

					//Check for random battle encounter
					check_for_battle();
				}

				fpcamera_x -= fpmoveinc_x;
			}
		}

		//testing for tilemap camera
		if (fpcamera_x < 0) fpcamera_x = 0;
		if (fpcamera_y < 0) fpcamera_y = 0;
	}
		
		//Stilll need to flip and call display without map drawn for everything else
		//may try to refactor this further
		Hw32xFlipWait();
		clip = display();

	if (GAME_STATE & GAME_MAP_VIEW)
	{
		//Used for camera scrolling tilemap
		if (clip & 2)
		{
			// clipped to the right
			fpcamera_x = old_fpcamera_x;
		}
		if (clip & 8)
		{
			// clipped to the right
			fpcamera_y = old_fpcamera_y;
		}
	}
}

/// <summary>
/// We check if the current map has any mobs that can be fought.
/// If so, then we randomly determine if player encounters that mob in a battle
/// </summary>
void check_for_battle()
{
	//Only if map has mobs to fight
	if (currentGameMap.mobs != NULL)
	{
		//simple random gen between 1 - 1000
		srand(Mars_GetTicCount());
		int random_number = rand() % 1000 + 1;

		//testing a hit
		if (random_number < 10)
		{
			//simple randomly select npc from list to battle
			int mob = 1;
			if (currentGameMap.numMobs > 1)
			{
				mob = rand() % currentGameMap.numMobs + 1;
			}

			//got our mob now show battle screen and animate him
			currentMob = currentGameMap.mobs[mob - 1];
			fade_display(fade_palette, false);
			GAME_STATE |= GAME_BATTLE; //Adds

			//set battle music
			Mars_PlayTrack(0, 1, &musicBattle, (int)sizeof(&musicBattle), 1);
		}
	}
}

/// <summary>
/// Checks the current maps collisions as the player moves
/// </summary>
void check_map_collisions()
{
	//Need to make sure that the tilemap is currently being drawn for map collisions to occur
	if (GAME_STATE & GAME_MAP_VIEW)
	{
		////
		//Check for warp
		///
		if (player_collisions(currentPlayerPos_x, currentPlayerPos_y, &tm, &currentGameMap, &collisionLayer) == COLLISION_WARP)
		{
			//Let's see which warp to use (testing location to warp array in map
			int i;
			for (i = 0; i < currentGameMap.numWarps; i++)
			{
				//Found correct warp information now perform warp
				if (currentMapX_position >= currentGameMap.warps[i].x && currentMapX_position < currentGameMap.warps[i].x + 16 && currentMapY_position >= currentGameMap.warps[i].y && currentMapY_position < currentGameMap.warps[i].y + 16)
				{
					//Still testing, but I would get the map ID from warp information and pull that map from my gamemaps array when i build it, but for the time
					//I'm testing to warp to 2nd map
					load_new_map(currentGameMap.warps[i].dest_mapID, currentGameMap.warps[i].dest_x, currentGameMap.warps[i].dest_x);
				}
			}
		}

		////
		//check for special battle
		////

		////
		//check for treasure if they search
		///
		
		////
		//Check for damage
		////

		////
		//check for special event
		////
	}
}

/// <summary>
/// Show's screen, sometimes a portait maybe shown as well
/// There will be no fading done when this is visible.  Tilemapping will be paused during dialog shown
/// </summary>
void dialog_screen()
{
	if (GAME_STATE & GAME_DIALOG)
	{
		//test drawing dialog
		unsigned char* dialogB = (unsigned char*)((int)dialogBox + 0x36 + 1024); //gets the bitmap data
		draw_sprite(10, 140, 300, 74, 0, dialogB, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP); //test

		//test write something to the screen
		char temp[0];
		gm_warp_point wp = currentGameMap.warps[1];
		sprintf(temp, "warp1: %d", wp.dest_mapID);
		display_text(temp, 10, 150);

		//Test start button while in this screen
		if (buttons & BT_START)
		{
			if (!(GAME_STATE & GAME_MAP_VIEW))
			{
				GAME_STATE &= ~GAME_DIALOG; //this removes
				GAME_STATE |= GAME_MAP_VIEW;
			}
		}
	}
}

void battle_screen()
{
	if (GAME_STATE & GAME_BATTLE)
	{
		//Need to fade back in
		if (!resetFading)
			resetFading = true;

		unsigned char* bg_src = (unsigned char*)((int)background1 + 0x36 + 1024); //gets the bitmap data
		unsigned char* fg_src = (unsigned char*)((int)foreground1 + 0x36 + 1024); //gets the bitmap data

		draw_sprite(0, 0, 320, 224, 0, bg_src, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP); //test
		draw_sprite(320 - 96, 224 - 32, 96, 32, 0, fg_src, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP | DRAWSPR_HFLIP); //test

		//simple test attack animation
		if (buttons & BT_A && !(old_buttons & BT_A))
		{
			//Change state of mob for testing
			//if (currentMob.battleData.battleSprite.current_state != ATTACKING_STATE)
			//	currentMob.battleData.battleSprite.current_state = ATTACKING_STATE;

			//Change state to defend
			if (GAME_WORLD.currentPlayer.battleSprite.current_state != DEFEND_STATE)
			{
				GAME_WORLD.currentPlayer.battleSprite.current_state = DEFEND_STATE;

				//test sound effect
				S_StartSound(NULL, 2);
			}
		}

		//simple test attack animation
		if (buttons & BT_C && !(old_buttons & BT_C))
		{
			//Change state
			if (GAME_WORLD.currentPlayer.battleSprite.current_state != ATTACKING_STATE)
			{
				GAME_WORLD.currentPlayer.battleSprite.current_state = ATTACKING_STATE;

				//Simple reduce HP for testing
				currentMob.battleData.hp -= 1;

				//test sound effect
				S_StartSound(NULL, 3);
			}
		}
		
		//test mob animation
		animate_battle_sprite(&currentMob, currentMob.battleData.battleSprite.current_state, 0, 0, flags);

		//test player animation
		animate_player_battle_sprite(&GAME_WORLD.currentPlayer, GAME_WORLD.currentPlayer.battleSprite.current_state, 220, 120, flags);

		//draw text panels
		unsigned char* shortDB = (unsigned char*)((int)dialogBoxShort + 0x36 + 1024); //gets the bitmap data
		draw_sprite(10, 180, 124, 45, 0, shortDB, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP); //test

		//draw text panels
		draw_sprite(190, 5, 124, 45, 0, shortDB, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP); //test

		//test write something to the screen
		char temp[0];
		sprintf(temp, "%s", currentMob.name);
		display_text(temp, 10, 184);
		sprintf(temp, "HP: %d\\10", currentMob.battleData.hp);
		display_text(temp, 10, 198);

		//player info
		sprintf(temp, "%s", GAME_WORLD.currentPlayer.name);
		display_text(temp, 190, 9);
		sprintf(temp, "HP: %d\\10", GAME_WORLD.currentPlayer.hp);
		display_text(temp, 190, 23);

	}
}


/// <summary>
/// All logic associated to title screen presented
/// </summary>
void title_screen()
{
	if (GAME_STATE == GAME_TITLE_SCREEN)
	{
		if (!resetFading)
			resetFading = true;

		display_screen(32, 10, 256, 135, title);

		//Start text drawn
		display_text("Press Start", 110, 150);

		//Start button only currently testing
		if (buttons & BT_START)
		{
			//fade out
			if (!isFading)
			{
				fade_display(fade_palette, false);
				GAME_STATE &= ~GAME_TITLE_SCREEN; //this removes Game_splash_screen1 rom Game_state
				GAME_STATE |= GAME_MAP_VIEW; //Adds the title screen
			}
		}

	}
}

/// <summary>
/// All logic associated to the 2nd splash screen
/// </summary>
void splash_screen2()
{
	if (GAME_STATE & GAME_SPLASH_SCREEN2)
	{
		if (!resetFading)
			resetFading = true;

		display_screen(32, 60, 256, 88, splashscreen2);

		//after about a second then move on
		tics = Mars_GetTicCount();
		if (((tics - start_tic) > 300) || buttons & BT_START)
		{
			//fade out
			if (!isFading)
			{
				fade_display(fade_palette, false);
				GAME_STATE &= ~GAME_SPLASH_SCREEN2; //this removes Game_splash_screen1 rom Game_state
				GAME_STATE |= GAME_TITLE_SCREEN; //Adds the title screen
				start_tic = Mars_GetTicCount();
			}
		}
	}
}

/// <summary>
/// All logic associated to the first splash screen or boot screen
/// </summary>
void splash_screen()
{
	if (GAME_STATE & GAME_SPLASH_SCREEN1)
	{
		display_screen(69, 87, 192, 64, splashscreen);

		//after about a second then move on
		tics = Mars_GetTicCount();
		if (((tics - start_tic) > 300) || buttons & BT_START)
		{
			//fade out
			if (!isFading)
			{
				fade_display(fade_palette, false);
				GAME_STATE &= ~GAME_SPLASH_SCREEN1; //this removes Game_splash_screen1 from Game_state
				GAME_STATE |= GAME_SPLASH_SCREEN2; //Adds the next splash screen
				start_tic = Mars_GetTicCount();
			}
		}
	}
}

///Draws all sprites within the map including the player
void display_sprites(int l, void* p)
{
	
	//Setup
	draw_setScissor(0, 0, 320, 224);

	//No need to draw if battle screen or if map view isn't present
	if (GAME_STATE & GAME_MAP_VIEW && !(GAME_STATE & GAME_BATTLE))
	{
		//Simple NPC sprite drawn in the world.  Probably need to place all sprites within a single draw_sprite routine like the one with TileMap
		//if ((npc_sprite_currentX_position - currentMapX_position) > 0)//test only draw if on screen?

		//Runs all map sprites
		int m;
		for (m = 0; m < currentGameMap.numNPCs; m++)
		{
			run_ai(&currentGameMap.npcs[m], currentMapX_position, currentMapY_position, flags,currentPlayerPos_x, currentPlayerPos_y, &tm, &currentGameMap, &collisionLayer);
		}

		//Draws the main player
		animate_player_sprite(&GAME_WORLD.currentPlayer, GAME_WORLD.currentPlayer.sprite.current_state, 128, 112, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP);
	}
}

//For game_engine
int display()
{
	int cameraclip = 0;

	draw_setScissor(0, 0, 320, 224);

	//Onlly draw if we are showing mapview
	if(GAME_STATE & GAME_MAP_VIEW)
		draw_tilemap(&tm, fpcamera_x, fpcamera_y, &cameraclip, &display_sprites, NULL);

	return cameraclip;
}

//For game_engine
void Display_Battle()
{
	unsigned char* bg_src = (unsigned char*)((int)background1 + 0x36 + 1024); //gets the bitmap data
	unsigned char* fg_src = (unsigned char*)((int)foreground1 + 0x36 + 1024); //gets the bitmap data

	draw_sprite(0, 0, 320, 224, 0, bg_src, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP); //test
	draw_sprite(320 - 96, 224 - 32, 96, 32, 0, fg_src, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP | DRAWSPR_HFLIP); //test
}

/// <summary>
/// helper function that resets any faded screen back to full color if triggered
/// </summary>
void reset_fading()
{
	if (resetFading && isFading)
	{
		fade_tics = Mars_GetTicCount();
		if ((fade_tics - start_fade_tic) > 60)
		{
			//Need to fade back in
			fade_display(fade_palette, true);
			start_fade_tic = Mars_GetTicCount();
		}
	}
}

//For game_engine
void fade_display(unsigned short* p, boolean fadeIn)
{
	isFading = true;
	int fadeCnt = 0;

	//fade to black
	if (!fadeIn)
	{
		fadeCnt = 255;
		while (fadeCnt--)
		{
			Mars_SetBrightness(fadeCnt - 255);
			I_SetPalette(p);

		}
	}
	else
	{
		fadeCnt = 0;
		while (fadeCnt <= 256)
		{
			Mars_SetBrightness(fadeCnt - 255);
			I_SetPalette(p);
			fadeCnt++;
		}

		//No longer fading
		isFading = false;
	}
	
	resetFading = false;
	
}

//For game_engine
void display_screen(int x, int y, int w, int h, char screen[])
{
	unsigned char* splash = (unsigned char*)((int)screen + 0x36 + 1024); //gets the bitmap data
	unsigned char* bg_src = (unsigned char*)((int)blackbg + 0x36 + 1024); //gets the bitmap data

	draw_sprite(0, 0, 320, 224, 0, bg_src, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP); //test
	draw_sprite(x, y, w, h, 0, splash, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP | DRAWSPR_HFLIP); //test
}

///Used for dipslaying text on screen from loaded font
///May need to make adjustments with different sized fonts
void display_text(char* s, int x, int y)
{
	//test font
	char* o, ch;
	int charPos = 0;
	int charLocation;
	int textLines = 0;
	for (o = s; *o; o++) {
		charPos++;
		ch = *o;

		//testing return
		if (charPos > 35)
		{
			charPos = 0;
			textLines++;
		}

		//Mapping characters to corresponding images from .bmp spritesheet
		switch (ch)
		{
		case 'A':
			charLocation = 0;
			break;
		case 'a':
			charLocation = 1;
			break;
		case 'B':
			charLocation = 2;
			break;
		case 'b':
			charLocation = 3;
			break;
		case 'C':
			charLocation = 4;
			break;
		case 'c':
			charLocation = 5;
			break;
		case 'D':
			charLocation = 6;
			break;
		case 'd':
			charLocation = 7;
			break;
		case 'E':
			charLocation = 8;
			break;
		case 'e':
			charLocation = 9;
			break;
		case 'F':
			charLocation = 10;
			break;
		case 'f':
			charLocation = 11;
			break;
		case 'G':
			charLocation = 12;
			break;
		case 'g':
			charLocation = 13;
			break;
		case 'H':
			charLocation = 14;
			break;
		case 'h':
			charLocation = 15;
			break;
		case 'I':
			charLocation = 16;
			break;
		case 'i':
			charLocation = 17;
			break;
		case 'J':
			charLocation = 18;
			break;
		case 'j':
			charLocation = 19;
			break;
		case 'K':
			charLocation = 20;
			break;
		case 'k':
			charLocation = 21;
			break;
		case 'L':
			charLocation = 22;
			break;
		case 'l':
			charLocation = 23;
			break;
		case 'M':
			charLocation = 24;
			break;
		case 'm':
			charLocation = 25;
			break;
		case 'N':
			charLocation = 26;
			break;
		case 'n':
			charLocation = 27;
			break;
		case 'O':
			charLocation = 28;
			break;
		case 'o':
			charLocation = 29;
			break;
		case 'P':
			charLocation = 30;
			break;
		case 'p':
			charLocation = 31;
			break;
		case 'Q':
			charLocation = 32;
			break;
		case 'q':
			charLocation = 33;
			break;
		case 'R':
			charLocation = 34;
			break;
		case 'r':
			charLocation = 35;
			break;
		case 'S':
			charLocation = 36;
			break;
		case 's':
			charLocation = 37;
			break;
		case 'T':
			charLocation = 38;
			break;
		case 't':
			charLocation = 39;
			break;
		case 'U':
			charLocation = 40;
			break;
		case 'u':
			charLocation = 41;
			break;
		case 'V':
			charLocation = 42;
			break;
		case 'v':
			charLocation = 43;
			break;
		case 'W':
			charLocation = 44;
			break;
		case 'w':
			charLocation = 45;
			break;
		case 'X':
			charLocation = 46;
			break;
		case 'x':
			charLocation = 47;
			break;
		case 'Y':
			charLocation = 48;
			break;
		case 'y':
			charLocation = 49;
			break;
		case 'Z':
			charLocation = 50;
			break;
		case 'z':
			charLocation = 51;
			break;
		case '0':
			charLocation = 52;
			break;
		case '1':
			charLocation = 53;
			break;
		case '2':
			charLocation = 54;
			break;
		case '3':
			charLocation = 55;
			break;
		case '4':
			charLocation = 56;
			break;
		case '5':
			charLocation = 57;
			break;
		case '6':
			charLocation = 58;
			break;
		case '7':
			charLocation = 59;
			break;
		case '8':
			charLocation = 60;
			break;
		case '9':
			charLocation = 61;
			break;
		case '!':
			charLocation = 62;
			break;
		case '.':
			charLocation = 63;
			break;
		case '?':
			charLocation = 64;
			break;
		case ':':
			charLocation = 65;
			break;
		case '-':
			charLocation = 66;
			break;
		case '+':
			charLocation = 67;
			break;
		case '=':
			charLocation = 68;
			break;
		case '\\':
			charLocation = 85;
			break;
		case '\r':

			break;
		case '\n':

			break;
		case '\t':

			break;
		default:

		}

		//Draw character
		if (ch != ' ')
			Draw_Sprite_From_SpriteSheet(x + (charPos * 8), y + (textLines * 12), myFonts, charLocation, 0, 16, 16, 1376, DRAWSPR_OVERWRITE | DRAWSPR_PRECISE | DRAWSPR_VFLIP | DRAWSPR_HFLIP);
	}
}
