#include "GameDef.h"
#include "yatssd/types.h"
#include "npc.h"
#include "main_character.h"
#include "gamemap.h"


/// <summary>
/// Get tile info at x/y coordinates.
/// Still need to determine values for non zero
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns></returns>
uint16_t get_tile_at(int x, int y, tilemap_t* tm, dtilelayer_t* collisionLayer)
{
	//test sprite size being a factor here
	//x >>= 3; //divide by 8 since sprites i'm using are 32x32
	//y >>= 3;

	return collisionLayer->tiles[y * tm->tiles_hor + x];
}

/// <summary>
/// Evaluates x/y coordinates from left through right and top through bottom of sprite against tilemap collision layer
/// Currently using a + 16 since my sprites are 32x32 but don't take up the full 32x32 square against 8x8 tiles
/// </summary>
int sprite_map_collision(int x, int y, tilemap_t* tm, dtilelayer_t* collisionLayer)
{
	int left_tile = x;
	int right_tile = x + 32; //May need to determine the value for 32.  This is based off 32x32 sprite against 8x8 tilemap tiles
	int top_tile = y;
	int bottom_tile = y + 32;
	left_tile /= tm->tw;
	right_tile /= tm->tw;
	top_tile /= tm->th;
	bottom_tile /= tm->th;

	if (left_tile < 0) left_tile = 0;
	if (right_tile > tm->tiles_hor) right_tile = tm->tiles_hor;
	if (top_tile < 0) top_tile = 0;
	if (bottom_tile > tm->tiles_ver) bottom_tile = tm->tiles_ver;

	int any_collision = 0;
	int i, j;
	for (i = left_tile; i <= right_tile; i++)
	{
		for (j = top_tile; j <= bottom_tile; j++)
		{
			uint16_t result = get_tile_at(i, j, tm, collisionLayer);
			if (result > 0)
			{
				return result;
			}
		}
	}

	return any_collision;
}

/// <summary>
/// In this function we are checking for any kind of npc collision
/// </summary>
/// <returns></returns>
int npc_collisions(int x, int y, int player_x, int player_y, tilemap_t* tm, gamemap_t* gm, dtilelayer_t* collisionLayer)
{
	//First, lets check to see if any map collision
	if (sprite_map_collision(x, y, tm, collisionLayer) == COLLISION_BLOCK)
	{
		return COLLISION_BLOCK;
	}

	/////
	//if no map collision let's check player and other sprite collisions
	/////
	
	//check other sprites
	int s;
	for (s = 0; s < gm->numNPCs; s++)
	{
		//we skip the npc that's act that exact location, because its the current npc checking for collisions
		if (x != gm->npcs[s].x && y != gm->npcs[s].y)
		{
			//Now we check since its a different npc
			if ((x < gm->npcs[s].x + 16) && (x + 16 > gm->npcs[s].x) &&
				(y < gm->npcs[s].y + 16) && (y + 16 > gm->npcs[s].y))
				return COLLISION_BLOCK;
		}
	}

	//Check of player is blocking sprite
	if ((x < player_x + 16) && (x + 16 > player_x) &&
		(y < player_y + 16) && (y + 16 > player_y))
		return COLLISION_BLOCK;

	//else we are turning no blocking
	return COLLISION_NONE;
}

/// <summary>
/// Handles all player collision (map and other sprites)
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="tm"></param>
/// <param name="gm"></param>
/// <param name="collisionLayer"></param>
/// <returns></returns>
int player_collisions(int x, int y, tilemap_t* tm, gamemap_t* gm, dtilelayer_t* collisionLayer)
{
	//First, lets check to see if any map collision
	int collisionType = sprite_map_collision(x, y, tm, collisionLayer);
	if (collisionType != COLLISION_NONE)
	{
		return collisionType;
	}

	//check other sprites
	int s;
	for (s = 0; s < gm->numNPCs; s++)
	{
		if ((x < gm->npcs[s].x + 16) && (x + 16 > gm->npcs[s].x) &&
			(y < gm->npcs[s].y + 16) && (y + 16 > gm->npcs[s].y))
			return COLLISION_BLOCK;
	}

	//else we return no blocking
	return COLLISION_NONE;
}