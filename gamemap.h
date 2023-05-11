
//
//Needed for organizing GameMap/World assets such as sprites, maps, etc
///

typedef struct {
	int x, y;
	int dest_x, dest_y;
	int dest_mapID;

} gm_warp_point;

// in-memory game map houses information about the map itself, such as npcs, items, events, etc.
typedef struct {

	int mapID; //used for warping
	char* name;
	uint8_t* reslist; //Tileset used with map
	dtilemap_t tilemap;
	gm_warp_point* warps; //adjust max number of warps later
	npc_t* npcs; //These are the npcs that are visibly walking on the map
	npc_t* mobs; //These are the npcs that are for random encounters on a map
	int numMobs;
	int numNPCs;
	int numWarps;

	//TODO: collection of npcs
	//TODO: collection of battlezones (what monsters can randomly appear)
	//TODO: collection of events/effects in map
	//TODO: map music
} gamemap_t;

// in-memory gameworld contains collections of the entire world such as gamemaps
typedef struct {

	gamemap_t* gamemaps;
	main_character_t currentPlayer;
} gameworld_t;
