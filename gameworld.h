#include "mars.h"
#include "npc.h"
#include "main_character.h"
#include "gamemap.h"

#include "yatssd/Tiled/ts1.h"
#include "yatssd/Tiled/RPGMap.h" //used for tilemap/
#include "yatssd/Tiled/ts2.h"
#include "yatssd/Tiled/RPGMap2.h" //used for tilemap/

extern char spritesheettest[], spritesheettest2[], spritesheettest3[], spritesheettest4[], heroBattleSprite[];


////////////////////////
/// <summary>
/// WORLD implementations down below
/// </summary>
/// /////////////////////

/// <summary>
/// Listing of all mobs/random encounters possible within this map
/// </summary>

sprite_state_t DwarfLord_Battle_States[] = { {2,FACING_DOWN_STATE,10, 0},{2,ATTACKING_STATE, 10, 0} };

//TEST Player (delete later)
sprite_state_t Bad_Player_Battle_States[] = { {2,FACING_DOWN_STATE,10, 0},{2,ATTACKING_STATE, 10, 0} };
sprite_state_t Player_Battle_States[] = { {2,FACING_DOWN_STATE,10, 0},{2,DEFEND_STATE, 10, 0},{2,ATTACKING_STATE, 5, 0} };
npc_t map1_mobs[] = {
	{"Dwarf Lord",
	"",
	{},
	30,
	90,
	{10,3,1,{2,(sprite_state_t*)DwarfLord_Battle_States,(unsigned char*)((int)spritesheettest3 + 0x36 + 1024),100,100,200,0,0, FACING_DOWN_STATE}},
	NO_AI, 0, 0,
	NOT_WALKING},
	{"Player",
	"",
	{},
	220,
	120,
	{10,3,1,{2,(sprite_state_t*)Bad_Player_Battle_States,(unsigned char*)((int)spritesheettest4 + 0x36 + 1024),100,100,200,0,0, FACING_DOWN_STATE}},
	NO_AI, 0, 0,
	NOT_WALKING}
};

//test map sprites
sprite_state_t bunky_npc_states[] = { {2,FACING_DOWN_STATE,10, 0},{2,FACING_LEFT_STATE, 10, 0}, {2,FACING_UP_STATE, 10, 0}, {2,FACING_RIGHT_STATE, 10, 0} };
npc_t map1_npcs[] = {
	{
	"Bunky",
	"Hello!  You should used Yatssd in your next game!",
	{4,(sprite_state_t*)bunky_npc_states,(unsigned char*)((int)spritesheettest2 + 0x36 + 1024),32,32,64,0,0, FACING_DOWN_STATE},
	200,
	130,
	{},
	RANDOM_WALK_AI, 0, 0,
	NOT_WALKING}
};

/// <summary>
/// Maps Implementations
/// </summary>
gm_warp_point map1_warps[] = { {160,120,0,0,1},{147,21,0,0,2} };
gm_warp_point map2_warps[] = { {516,149,0,0,1},{200,100,1,1,2} };
gamemap_t game_maps[] = {
	{1, "map 1 test", ts1_Reslist, RPGMap_Map,map1_warps, map1_npcs, map1_mobs, 2, 1, 2},
	{2, "map 2 test", ts2_Reslist, RPGMap2_Map,map2_warps, NULL, NULL, 0, 0, 2}
};

/// <summary>
/// Gameworld, collection of all maps, etc
/// </summary>

//Player states
sprite_state_t player_states[] = { {2,FACING_DOWN_STATE,10, 0},{2,FACING_LEFT_STATE, 10, 0}, {2,FACING_UP_STATE, 10, 0}, {2,FACING_RIGHT_STATE, 10, 0} };
gameworld_t GAME_WORLD = {
	(gamemap_t*)game_maps,
	{"Victor", 1,10,0,0,0,0,
	{4,(sprite_state_t*)player_states,(unsigned char*)((int)spritesheettest + 0x36 + 1024),32,32,64,0,0, FACING_DOWN_STATE},
	{3,(sprite_state_t*)Player_Battle_States,(unsigned char*)((int)heroBattleSprite + 0x36 + 1024),90,90,180,0,0, FACING_DOWN_STATE}}
};

//////////////////////
//
/////////////////////