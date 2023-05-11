#include <stdlib.h>
#include "GameDef.h"
#include "mars.h"
#include "32x.h"
#include "yatssd/types.h"


void Draw_Sprite_From_SpriteSheet(int x, int y, unsigned char* spriteBuffer, int crop_x, int crop_y, int crop_width, int crop_height, int src_width, int flags)
{
	//unsigned char* src = (unsigned char*)(spriteBuffer + crop_y * src_width + crop_x);
	unsigned char* src = (unsigned char*)spriteBuffer + crop_y * crop_height * src_width + crop_x * crop_width;

	//draw_sprite(x, y, crop_width - crop_x, crop_height - crop_y, src_width, src, flags); //test
	draw_sprite(x, y, crop_width, crop_height, src_width, src, flags);
}