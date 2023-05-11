
/// <summary>
/// enum for sprite states tied to animation frames
/// </summary>
enum {
	FACING_DOWN_STATE,
	FACING_RIGHT_STATE,
	FACING_UP_STATE,
	FACING_LEFT_STATE,
	ATTACKING_STATE,
	DEFEND_STATE,
	HIT_STATE,
	HURT_STATE
};

/// <summary>
/// Enums for AI types
/// </summary>
enum {
	NO_AI,
	RANDOM_WALK_AI,
	SCRIPTED_AI
};

/// <summary>
/// Used by AI to determine direction currently walking
/// </summary>
enum {
	NOT_WALKING,
	WALKING_LEFT,
	WALKING_RIGHT,
	WALKING_UP,
	WALKING_DOWN
};

typedef struct
{
	int numFrames;
	int state_type; //This not only defines the name, but also the Row location on the sprite sheet:  i.e.  IDLE_STATE = 0 which would be the top row
	int speed; //higher is slower
	int current_frame;
} sprite_state_t;

typedef struct
{
	int numStates;
	sprite_state_t* states;
	unsigned char* sprite_sheet;
	int spriteheight;
	int spriteWidth;
	int spriteSheetWidth;
	int npc_tics;
	int npc_tic_start;
	int current_state;
} sprite_data_t;

typedef struct
{
	int hp;
	int exp_given;
	int lvl;
	//TODO: add items structs for drops
	//TODO: add battle AI
	sprite_data_t battleSprite;

} npc_battle_data;

typedef struct
{
	char* name;
	char* dialog;
	sprite_data_t sprite;
	int x, y; 
	npc_battle_data battleData;
	int ai_type;
	int ai_tics;
	int ai_start_tic;
	int ai_walking_direction;
} npc_t;