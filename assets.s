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

.align 4
_dialogBoxShort:
.incbin "assets/graphics/DialogBoxShort.bmp"

.align 4
_dialogBox:
.incbin "assets/graphics/DialogBox.bmp"

.align 4
_newfont:
.incbin "assets/graphics/NewFont2.bmp"

.align 4
_title:
.incbin "assets/graphics/GameTitle.bmp"

.align 4
_blackbg:
.incbin "assets/graphics/black.bmp"

.align 4
_splashscreen2:
.incbin "assets/graphics/StudioTitle.bmp"

.align 4
_splashscreen:
.incbin "assets/graphics/Chilly.bmp"

.align 4
_background1:
.incbin "assets/graphics/background1.bmp"

.align 4
_foreground1:
.incbin "assets/graphics/foreground1.bmp"

.align 4
_spritesheettest4:
.incbin "assets/graphics/spritesheet4.bmp"

.align 4
_spritesheettest3:
.incbin "assets/graphics/spritesheet3.bmp"

.align 4
_spritesheettest2:
.incbin "assets/graphics/spritesheet2.bmp"

.align 4
_spritesheettest:
.incbin "assets/graphics/spritesheet1.bmp"

.align 4
_heroBattleSprite:
.incbin "assets/graphics/Test_Hero.bmp"

.align 4
_musicBattle:
.incbin "assets/music/Bunny.zgm"

.align 4
_music:
.incbin "assets/music/intro.zgm"

.align 4
_sfx1:
.incbin "assets/sound/11.wav"

.align 4
_sfx2:
.incbin "assets/sound/22.wav"

.align 4
_sfx3:
.incbin "assets/sound/33.wav"
