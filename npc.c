#include "GameDef.h"
#include "mars.h"
#include "yatssd/types.h"
#include "npc.h"
#include "main_character.h"
#include "gamemap.h"

//used for local use of current frame for performance and accuracy
int cf_npc_battle = 0;
int cf_npc = 0;

//Header for npc collisions
int npc_collisions(int x, int y, int player_x, int player_y, tilemap_t* tm, gamemap_t* gm, dtilelayer_t* collisionLayer);


///Used for random AI movements
int ai_random_tic_counter, ai_random_pause_time;

/// <summary>
/// Within animating a sprite.  I'm assuming that the Y coordinate will be the state (or animatation) that you want your sprite to be.
/// And, let x be the frame within that state.  Some states may have less/more frames than others.  I'm also going to make it so that
/// you can adjust the rate of animate per state
/// 
/// NOTE:  Since we have pulled the sprite data directly from the bmp, everything is upside down.  So within here if we want to 0 position state, it will be the last state
/// </summary>
void animate_sprite(npc_t *npc, int state_to_animate, int currentMapX_position, int currentMapY_position, int flags)
{

	int current_state;
	int s;
	for (s = 0; s < npc->sprite.numStates; s++)
	{
		if (npc->sprite.states[s].state_type == state_to_animate)
			current_state = s;
	}

	//Adjust state since everything is upsidedown from .bmp
	int adjusted_state = npc->sprite.numStates - 1 - current_state;

	npc->sprite.npc_tics = Mars_GetTicCount();
	if ((npc->sprite.npc_tics - npc->sprite.npc_tic_start) > npc->sprite.states[current_state].speed)
	{
		//update frame if we have another one
		if (npc->sprite.states[current_state].current_frame < npc->sprite.states[current_state].numFrames)
			cf_npc = npc->sprite.states[current_state].current_frame++;
		else
		{
			npc->sprite.states[current_state].current_frame = 0;
			cf_npc = 0;
		}

		//reset start tick
		npc->sprite.npc_tic_start = Mars_GetTicCount();
	}

	//draw sprite at frame
	Draw_Sprite_From_SpriteSheet(npc->x - currentMapX_position, npc->y - currentMapY_position, npc->sprite.sprite_sheet, 
		cf_npc, adjusted_state, npc->sprite.spriteWidth, npc->sprite.spriteheight,
		npc->sprite.spriteSheetWidth, flags);

}

/// <summary>
/// Within animating a sprite.  I'm assuming that the Y coordinate will be the state (or animatation) that you want your sprite to be.
/// And, let x be the frame within that state.  Some states may have less/more frames than others.  I'm also going to make it so that
/// you can adjust the rate of animate per state
/// 
/// NOTE:  Since we have pulled the sprite data directly from the bmp, everything is upside down.  So within here if we want to 0 position state, it will be the last state
/// </summary>
void animate_battle_sprite(npc_t* npc, int state_to_animate, int currentMapX_position, int currentMapY_position, int flags)
{

	int current_state;
	int s;
	for (s = 0; s < npc->battleData.battleSprite.numStates; s++)
	{
		if (npc->battleData.battleSprite.states[s].state_type == state_to_animate)
			current_state = s;
	}

	//Adjust state since everything is upsidedown from .bmp
	int adjusted_state = npc->battleData.battleSprite.numStates - 1 - current_state;

	npc->battleData.battleSprite.npc_tics = Mars_GetTicCount();
	if ((npc->battleData.battleSprite.npc_tics - npc->battleData.battleSprite.npc_tic_start) > npc->battleData.battleSprite.states[current_state].speed)
	{
		//update frame if we have another one
		if (npc->battleData.battleSprite.states[current_state].current_frame < npc->battleData.battleSprite.states[current_state].numFrames)
			cf_npc_battle = npc->battleData.battleSprite.states[current_state].current_frame++;
		else
		{
			if (state_to_animate == ATTACKING_STATE || state_to_animate == DEFEND_STATE)
				npc->battleData.battleSprite.current_state = FACING_DOWN_STATE;

			npc->battleData.battleSprite.states[current_state].current_frame = 0;
			cf_npc_battle = 0;
		}
			

		//reset start tick
		npc->battleData.battleSprite.npc_tic_start = Mars_GetTicCount();
	}

	//draw sprite at frame
	Draw_Sprite_From_SpriteSheet(npc->x - currentMapX_position, npc->y - currentMapY_position, npc->battleData.battleSprite.sprite_sheet,
		cf_npc_battle, adjusted_state, npc->battleData.battleSprite.spriteWidth, npc->battleData.battleSprite.spriteheight,
		npc->battleData.battleSprite.spriteSheetWidth, flags);

}

/// <summary>
/// Executes an NPC's AI function(s)
/// </summary>
void run_ai(npc_t* npc, int currentMapX_position, int currentMapY_position, int flags, int player_x, int player_y, tilemap_t* tm, gamemap_t* gm, dtilelayer_t* collisionLayer)
{
	//Call Random walking AI
	if (npc->ai_type == RANDOM_WALK_AI)
	{
		ai_random_walk(npc, currentMapX_position, currentMapY_position, flags, player_x, player_y,tm,gm,collisionLayer);
	}

	if (npc->ai_type == NO_AI)
	{
		//don't draw if not on the screen
		if ((npc->x + npc->sprite.spriteWidth - 1 - currentMapX_position) > 0)
			animate_sprite(npc, npc->sprite.current_state, currentMapX_position, currentMapY_position, flags);
	}
}

/// <summary>
/// Basic AI function that allows NPC to randomly walk around the map
/// Respecting collisions as the NPC moves such as blocking and other NPCs as well as players
/// </summary>
void ai_random_walk(npc_t* npc, int currentMapX_position, int currentMapY_position, int flags, int player_x, int player_y, tilemap_t* tm, gamemap_t* gm, dtilelayer_t* collisionLayer)
{
	//Update tics
	npc->ai_tics = Mars_GetTicCount();

	//This section sets two things:  First it sets how long the AI actions go for with tic_counter and the second we determine
	//How long the next pause is between next set of tic_counters
	if ((npc->ai_tics - npc->ai_start_tic) > ai_random_pause_time && ai_random_tic_counter <= 0)
	{
		//Get new tic count amount
		srand(Mars_GetTicCount());
		ai_random_tic_counter = rand() % 100 + 1;

		//Set new pause time amount
		ai_random_pause_time = rand() % 300 + 10;

		//set the new direction
		npc->ai_walking_direction = rand() % 5;

		npc->ai_start_tic = Mars_GetTicCount();
	}

	//Time for some random AI movements below
	if ((npc->ai_tics - npc->ai_start_tic) > 1 && ai_random_tic_counter > 0)
	{
		//Decrement tic counter
		ai_random_tic_counter--;


		switch (npc->ai_walking_direction)
		{
		case NOT_WALKING:
			//Just animate no movement
			break;
		case WALKING_LEFT:

			//Change state
			if (npc->sprite.current_state != FACING_LEFT_STATE)
				npc->sprite.current_state = FACING_LEFT_STATE;

			//Make sure npc is in bounds
			if ((npc->x) > 0)
			{
				//making sure the sprite can walk there
				if (npc_collisions(npc->x, npc->y, player_x, player_y, tm, gm, collisionLayer) != COLLISION_BLOCK)
				{
					npc->x--;
				}
			}

			break;
		case WALKING_RIGHT:

			//Change state
			if (npc->sprite.current_state != FACING_RIGHT_STATE)
				npc->sprite.current_state = FACING_RIGHT_STATE;

			//Make sure npc is in bounds
			if ((npc->x) < (tm->tiles_hor * tm->tw) - npc->sprite.spriteWidth)
			{
				//making sure the sprite can walk there
				if (npc_collisions(npc->x, npc->y, player_x, player_y, tm, gm, collisionLayer) != COLLISION_BLOCK)
				{
					npc->x++;
				}
			}

			break;
		case WALKING_UP:

			//Change state
			if (npc->sprite.current_state != FACING_UP_STATE)
				npc->sprite.current_state = FACING_UP_STATE;

			//Make sure npc is in bounds
			if ((npc->y) > 0)
			{
				//making sure the sprite can walk there
				if (npc_collisions(npc->x, npc->y, player_x, player_y, tm, gm, collisionLayer) != COLLISION_BLOCK)
				{
					npc->y--;
				}
			}

			break;
		case WALKING_DOWN:

			//Change state
			if (npc->sprite.current_state != FACING_DOWN_STATE)
				npc->sprite.current_state = FACING_DOWN_STATE;

			//Make sure npc is in bounds
			if ((npc->y) < (tm->tiles_ver * tm->th) - npc->sprite.spriteheight)
			{
				//making sure the sprite can walk there
				if (npc_collisions(npc->x, npc->y, player_x, player_y, tm, gm, collisionLayer) != COLLISION_BLOCK)
				{
					npc->y++;
				}
			}

			break;
		default:
			break;
		}


		npc->ai_start_tic = Mars_GetTicCount();
	}

	//don't draw if not on the screen
	if ((npc->x + npc->sprite.spriteWidth - 1 - currentMapX_position) > 0)
		animate_sprite(npc, npc->sprite.current_state, currentMapX_position, currentMapY_position, flags);
}