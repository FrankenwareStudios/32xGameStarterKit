.align 4

.text
.globl _music
.globl _sfx1
.globl _sfx2
.globl _sfx3
.globl _spritesheettest
.globl _spritesheettest2
.globl _spritesheettest3
.globl _spritesheettest4
.globl _background1
.globl _foreground1
.globl _splashscreen
.globl _splashscreen2
.globl _blackbg
.globl _title
.globl _newfont
.globl _dialogBox
.globl _dialogBoxShort
.globl _musicBattle

.globl _heroBattleSprite

_dialogBoxShort:
.incbin "assets/graphics/DialogBoxShort.bmp"
_dialogBox:
.incbin "assets/graphics/DialogBox.bmp"
_newfont:
.incbin "assets/graphics/NewFont2.bmp"
_title:
.incbin "assets/graphics/GameTitle.bmp"
_blackbg:
.incbin "assets/graphics/black.bmp"
_splashscreen2:
.incbin "assets/graphics/StudioTitle.bmp"
_splashscreen:
.incbin "assets/graphics/Chilly.bmp"
_background1:
.incbin "assets/graphics/background1.bmp"
_foreground1:
.incbin "assets/graphics/foreground1.bmp"
_spritesheettest4:
.incbin "assets/graphics/spritesheet4.bmp"
_spritesheettest3:
.incbin "assets/graphics/spritesheet3.bmp"
_spritesheettest2:
.incbin "assets/graphics/spritesheet2.bmp"
_spritesheettest:
.incbin "assets/graphics/spritesheet1.bmp"

_heroBattleSprite:
.incbin "assets/graphics/Test_Hero.bmp"

_musicBattle:
.incbin "assets/music/Bunny.zgm"
_music:
.incbin "assets/music/intro.zgm"

_sfx1:
.incbin "assets/sound/11.wav"
_sfx2:
.incbin "assets/sound/22.wav"
_sfx3:
.incbin "assets/sound/33.wav"
