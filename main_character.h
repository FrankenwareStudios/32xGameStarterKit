
typedef struct
{
	char* name;
	int lvl;
	int hp;
	int exp;
	int x, y;
	int currentMapID;
	sprite_data_t sprite;
	sprite_data_t battleSprite;
	//TODO: add inventory
	//TODO: add equipment
} main_character_t;