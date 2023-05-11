#include "GameDef.h"
#include "mars.h"
#include "npc.h"
#include "main_character.h"

//used for local use of current frame for performance and accuracy
int cf_battle = 0;
int cf = 0;

/// <summary>
/// Within animating a sprite.  I'm assuming that the Y coordinate will be the state (or animatation) that you want your sprite to be.
/// And, let x be the frame within that state.  Some states may have less/more frames than others.  I'm also going to make it so that
/// you can adjust the rate of animate per state
/// 
/// NOTE:  Since we have pulled the sprite data directly from the bmp, everything is upside down.  So within here if we want to 0 position state, it will be the last state
/// </summary>
void animate_player_sprite(main_character_t* player, int state_to_animate, int playerPosition_x, int playerPosition_y, int flags)
{

	int current_state;
	int s;
	for (s = 0; s < player->sprite.numStates; s++)
	{
		if (player->sprite.states[s].state_type == state_to_animate)
			current_state = s;
	}

	//Adjust state since everything is upsidedown from .bmp
	int adjusted_state = player->sprite.numStates - 1 - current_state;

	player->sprite.npc_tics = Mars_GetTicCount();
	if ((player->sprite.npc_tics - player->sprite.npc_tic_start) > player->sprite.states[current_state].speed)
	{
		//update frame if we have another one
		if (player->sprite.states[current_state].current_frame < player->sprite.states[current_state].numFrames)
			cf = player->sprite.states[current_state].current_frame++;
		else
		{
			player->sprite.states[current_state].current_frame = 0;
			cf = 0;
		}

		//reset start tick
		player->sprite.npc_tic_start = Mars_GetTicCount();
	}

	//draw sprite at frame
	Draw_Sprite_From_SpriteSheet(playerPosition_x, playerPosition_y, player->sprite.sprite_sheet,
		cf, adjusted_state, player->sprite.spriteWidth, player->sprite.spriteheight,
		player->sprite.spriteSheetWidth, flags);

}

/// <summary>
/// Within animating a sprite.  I'm assuming that the Y coordinate will be the state (or animatation) that you want your sprite to be.
/// And, let x be the frame within that state.  Some states may have less/more frames than others.  I'm also going to make it so that
/// you can adjust the rate of animate per state
/// 
/// NOTE:  Since we have pulled the sprite data directly from the bmp, everything is upside down.  So within here if we want to 0 position state, it will be the last state
/// </summary>
void animate_player_battle_sprite(main_character_t* player, int state_to_animate, int battleScreenPos_x, int battleScreenPos_y, int flags)
{

	int current_state;
	int s;

	//use local int as it's faster
	//int cf = 0;

	for (s = 0; s < player->battleSprite.numStates; s++)
	{
		if (player->battleSprite.states[s].state_type == state_to_animate)
			current_state = s;
	}

	//Adjust state since everything is upsidedown from .bmp
	int adjusted_state = player->battleSprite.numStates - 1 - current_state;

	player->battleSprite.npc_tics = Mars_GetTicCount();
	if ((player->battleSprite.npc_tics - player->battleSprite.npc_tic_start) > player->battleSprite.states[current_state].speed)
	{
		//update frame if we have another one
		if (player->battleSprite.states[current_state].current_frame < player->battleSprite.states[current_state].numFrames)
		{
			cf_battle = player->battleSprite.states[current_state].current_frame++;
		}
		else
		{
			if (state_to_animate == ATTACKING_STATE || state_to_animate == DEFEND_STATE)
				player->battleSprite.current_state = FACING_DOWN_STATE;

			player->battleSprite.states[current_state].current_frame = 0;
			cf_battle = 0;
		}


		//reset start tick
		player->battleSprite.npc_tic_start = Mars_GetTicCount();
	}

	//draw sprite at frame
	Draw_Sprite_From_SpriteSheet(battleScreenPos_x, battleScreenPos_y, player->battleSprite.sprite_sheet,
		cf_battle, adjusted_state, player->battleSprite.spriteWidth, player->battleSprite.spriteheight,
		player->battleSprite.spriteSheetWidth, flags);


}