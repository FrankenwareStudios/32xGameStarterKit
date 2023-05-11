#include <stdint.h>
#include <stddef.h>

#define ATTR_DATA_OPTIMIZE_NONE __attribute__((section(".data"), aligned(16), optimize("O1")))
#define ATTR_DATA_CACHE_ALIGN __attribute__((section(".data"), aligned(16), optimize("Os")))
#define ATTR_OPTIMIZE_SIZE __attribute__((optimize("Os")))
#define ATTR_OPTIMIZE_EXTREME __attribute__((optimize("O3", "no-align-loops", "no-align-functions", "no-align-jumps", "no-align-labels")))

//used within drawing
#define SCREEN_HEIGHT 224
#define SCREEN_WIDTH 320

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum { false, true } boolean;
typedef unsigned char byte;
#endif

#define MAXPLAYERS	2
#define TICRATE		15				/* number of tics / second */

#define MINTICSPERFRAME		2
#define MAXTICSPERFRAME		4

#define	VINT	short
#define	FRACBITS		16
typedef int fixed_t;
typedef unsigned angle_t;

typedef unsigned short pixel_t;

typedef void (*latecall_t) (struct mobj_s* mo); //May not be needed

/*================= */
/*TLS */
/*================= */

#define DOOMTLS_BANKPAGE 		0
#define DOOMTLS_SETBANKPAGEPTR 	4
#define DOOMTLS_VALIDCOUNT 		8

#define	AF_OPTIONSACTIVE	128			/* options screen running */

#define	ANG45	0x20000000
#define	ANG90	0x40000000
#define	ANG180	0x80000000
#define	ANG270	0xc0000000
typedef unsigned angle_t;

#define	FINEANGLES			8192
#define	ANGLETOFINESHIFT	19	/* 0x100000000 to 0x2000 */

#define	FRACBITS		16
#define	FRACUNIT		(1<<FRACBITS)

//used in marssound under s_update()
extern	boolean		playeringame[MAXPLAYERS];

//was in fixed.h from yatssd
#define FIXED_SHIFT_BITS    16
#define FIXED_UNIT          (1<<FIXED_SHIFT_BITS)
fixed_t	FixedMul(fixed_t a, fixed_t b);
fixed_t	FixedDiv(fixed_t a, fixed_t b);
fixed_t IDiv(fixed_t a, fixed_t b);

#define FixedMul2(c,a,b) do { \
        int32_t t; \
       __asm volatile ( \
            "dmuls.l %2, %3\n\t" \
            "sts mach, %1\n\t" \
            "sts macl, %0\n\t" \
            "xtrct %1, %0\n\t" \
            : "=r" (c), "=&r" (t) \
            : "r" (a), "r" (b) \
            : "mach", "macl"); \
        } while (0)

typedef enum
{
	ga_nothing,
	ga_died,
	ga_completed,
	ga_secretexit,
	ga_warped,
	ga_exitdemo,
	ga_startnew
} gameaction_t;

typedef enum
{
	gt_single,
	gt_coop,
	gt_deathmatch
} gametype_t;

typedef enum
{
	sk_baby,
	sk_easy,
	sk_medium,
	sk_hard,
	sk_nightmare
} skill_t;

typedef struct
{
	VINT	state;		/* a S_NULL state means not active */
	VINT	tics;
	fixed_t	sx, sy;
} pspdef_t;

typedef struct mobj_s
{
	fixed_t			x, y, z;
	struct	mobj_s* prev, * next;

	unsigned char	movedir;		/* 0-7 */
	char			movecount;		/* when 0, select a new dir */
	unsigned char		reactiontime;	/* if non 0, don't attack yet */
									/* used by player to freeze a bit after */
									/* teleporting */
	unsigned char		threshold;		/* if >0, the target will be chased */
									/* no matter what (even if shot) */
	unsigned char		sprite;				/* used to find patch_t and flip value */
	unsigned char		player;		/* only valid if type == MT_PLAYER */

	VINT			health;
	VINT			tics;				/* state tic counter	 */
	VINT 			state;
	VINT			frame;				/* might be ord with FF_FULLBRIGHT */

	unsigned short		thingid;

	unsigned short		type;

	/* info for drawing */
	struct	mobj_s* snext, * sprev;		/* links in sector (if needed) */
	angle_t			angle;

	/* interaction info */
	struct mobj_s* bnext, * bprev;		/* links in blocks (if needed) */
	struct subsector_s* subsector;
	fixed_t			floorz, ceilingz;	/* closest together of contacted secs */
	fixed_t			radius, height;		/* for movement checking */
	fixed_t			momx, momy, momz;	/* momentums */

	unsigned 		speed;			/* mobjinfo[mobj->type].speed */
	int			flags;
	struct mobj_s* target;		/* thing being chased/attacked (or NULL) */
									/* also the originator for missiles */
	latecall_t		latecall;		/* set in p_base if more work needed */
	intptr_t		extradata;		/* for latecall functions */
} mobj_t;

typedef enum
{
	PST_LIVE,			/* playing */
	PST_DEAD,			/* dead on the ground */
	PST_REBORN			/* ready to restart */
} playerstate_t;

/* psprites are scaled shapes directly on the view screen */
/* coordinates are given for a 320*200 view screen */
typedef enum
{
	ps_weapon,
	ps_flash,
	NUMPSPRITES
} psprnum_t;

typedef enum
{
	pw_invulnerability,
	pw_strength,
	pw_ironfeet,
	pw_allmap,
	NUMPOWERS
} powertype_t;

typedef enum
{
	it_bluecard,
	it_yellowcard,
	it_redcard,
	it_blueskull,
	it_yellowskull,
	it_redskull,
	NUMCARDS
} card_t;

typedef enum
{
	am_clip,		/* pistol / chaingun */
	am_shell,		/* shotgun */
	am_cell,		/* BFG */
	am_misl,		/* missile launcher */
	NUMAMMO,
	am_noammo		/* chainsaw / fist */
} ammotype_t;

typedef enum
{
	wp_fist,
	wp_pistol,
	wp_shotgun,
	wp_chaingun,
	wp_missile,
	wp_plasma,
	wp_bfg,
	wp_chainsaw,
	NUMWEAPONS,
	wp_nochange
} weapontype_t;

/*
================
=
= player_t
=
================
*/

typedef struct player_s
{
	mobj_t* mo;
	playerstate_t	playerstate;

	fixed_t		forwardmove, sidemove;	/* built from ticbuttons */
	angle_t		angleturn;				/* built from ticbuttons */

	fixed_t		viewz;					/* focal origin above r.z */
	fixed_t		viewheight;				/* base height above floor for viewz */
	fixed_t		deltaviewheight;		/* squat speed */
	fixed_t		bob;					/* bounded/scaled total momentum */

	VINT		health;					/* only used between levels, mo->health */
										/* is used during levels	 */
	VINT		armorpoints, armortype;	/* armor type is 0-2 */

	VINT		powers[NUMPOWERS];		/* invinc and invis are tic counters	 */
	char		cards[NUMCARDS];
	char		backpack;
	VINT		frags;					/* kills of other player */
	VINT		readyweapon;
	VINT		pendingweapon;		/* wp_nochange if not changing */
	char		weaponowned[NUMWEAPONS];
	VINT		ammo[NUMAMMO];
	VINT		maxammo[NUMAMMO];
	VINT		attackdown, usedown;	/* true if button down last tic */
	VINT		cheats;					/* bit flags */

	VINT		refire;					/* refired shots are less accurate */

	VINT		killcount, itemcount, secretcount;		/* for intermission */
	char* message;				/* hint messages */
	VINT		damagecount, bonuscount;/* for screen flashing */
	mobj_t* attacker;				/* who did damage (NULL for floors) */
	VINT		extralight;				/* so gun flashes light up areas */
	VINT		colormap;				/* 0-3 for which color to draw player */
	pspdef_t	psprites[NUMPSPRITES];	/* view sprites (gun, etc) */
	boolean		didsecret;				/* true if secret level has been done */
	void* lastsoundsector;		/* don't flood noise every time */

	int			automapx, automapy, automapscale, automapflags;
	int			turnheld;				/* for accelerative turning */
} player_t;

typedef enum
{
	DEBUGMODE_NONE,

	DEBUGMODE_FPSCOUNT,
	DEBUGMODE_ALLCOUNT,
	DEBUGMODE_NOTEXCACHE,
	DEBUGMODE_NODRAW,
	DEBUGMODE_TEXCACHE,

	DEBUGMODE_NUM_MODES
} debugmode_t;

//May not need this?  Maybe for jaguar only?
typedef struct
{
	short	width;		/* in pixels */
	short	height;
	short	depth;		/* 1-5 */
	short	index;		/* location in palette of color 0 */
	short	pad1, pad2, pad3, pad4;	/* future expansion */
	byte	data[8];		/* as much as needed */
} jagobj_t;

enum
{
	// hardware-agnostic game button actions
	// transmitted over network
	// should fit into a single word
	BT_RIGHT = 0x1,
	BT_LEFT = 0x2,
	BT_UP = 0x4,
	BT_DOWN = 0x8,

	BT_ATTACK = 0x10,
	BT_USE = 0x20,
	BT_STRAFE = 0x40,
	BT_SPEED = 0x80,

	BT_START = 0x100,
	BT_AUTOMAP = 0x200,
	BT_MODE = 0x400,

	BT_PWEAPN = 0x800,
	BT_NWEAPN = 0x1000,

	BT_STRAFELEFT = 0x2000,
	BT_STRAFERIGHT = 0x4000,

	BT_A = 0x8000,
	BT_B = 0x10000,
	BT_C = 0x20000,
	BT_LMBTN = 0x40000,
	BT_RMBTN = 0x80000,
	BT_MMBTN = 0x100000,
	BT_PAUSE = 0x200000,
	BT_OPTION = 0x400000,
	BT_X = 0x800000,
	BT_Y = 0x1000000,
	BT_Z = 0x2000000,
	BT_FASTTURN = 0x4000000
};

typedef enum
{
	SFU,
	SUF,
	FSU,
	FUS,
	USF,
	UFS,
	NUMCONTROLOPTIONS
} control_t;

extern unsigned configuration[NUMCONTROLOPTIONS][3];
extern	VINT	controltype;				/* 0 to 5 */
extern	VINT	alwaysrun;
extern	boolean		mousepresent;
extern	boolean	splitscreen;
extern	VINT	ticsperframe;		/* 2 - 4 */
extern	player_t	players[MAXPLAYERS];
extern	int			consoleplayer;		/* player taking events and displaying */
extern	int		ticrealbuttons, oldticrealbuttons; /* buttons for the console player before reading the demo file */
extern	skill_t		startskill;
extern	int			startmap;
extern	gametype_t	starttype;
extern	int		vblsinframe;			/* range from 4 to 8 */
extern	gameaction_t	gameaction;
extern	int		ticbuttons[MAXPLAYERS];
extern	int		oldticbuttons[MAXPLAYERS];
extern	int 	ticrate;	/* 4 for NTSC, 3 for PAL */
extern VINT COLOR_WHITE;

/* */
/* library replacements */
/* */

#define D_abs(x) ((x < 0) ? -(x) : x)
void D_memset(void* dest, int val, size_t count) __attribute__((nonnull));

//
//Used within dsprite_extensions.c
//
void Draw_Sprite_From_SpriteSheet(int x, int y, unsigned char* spriteBuffer, int crop_x, int crop_y, int crop_width, int crop_height, int src_width, int flags);


//
//was in r_local.h
//
#define	SLOPERANGE	2048


typedef enum
{
	detmode_potato = -1,
	detmode_medium,
	detmode_high,

	MAXDETAILMODES
} detailmode_t;

typedef struct
{
	unsigned short* pixcount; /* capped at 0xffff */
	VINT* framecount;

	int maxobjects;
	int objectsize;

	int bestobj;
	int bestcount;
	int reqcount_le;
	int reqfreed;

	int zonesize;
	void* zone;
} r_texcache_t;

typedef struct
#ifdef MARS
__attribute__((aligned(16)))
#endif
{
	fixed_t		viewx, viewy, viewz;
	angle_t		viewangle;
	fixed_t		viewcos, viewsin;
	player_t* viewplayer;
	VINT		lightlevel;
	VINT		extralight;
	VINT		displayplayer;
	VINT		fixedcolormap;
	VINT		fuzzcolormap;
} viewdef_t;

/* A vissprite_t is a thing that will be drawn during a refresh */
typedef struct vissprite_s
{
	VINT		x1, x2;			/* clipped to screen edges column range */
	fixed_t		startfrac;		/* horizontal position of x1 */
	fixed_t		xscale;
	fixed_t		xiscale;		/* negative if flipped */
	fixed_t		yscale;
	fixed_t		yiscale;
	fixed_t		texturemid;
	VINT 		patchnum;
	VINT		colormap;		/* < 0 = shadow draw */
	fixed_t		gx, gy;	/* global coordinates */
#ifndef MARS
	pixel_t* pixels;		/* data patch header references */
#endif
} vissprite_t;

typedef struct
{
	fixed_t		x, y;
} vertex_t;

struct line_s;

typedef	struct
{
	fixed_t		floorheight, ceilingheight;
	VINT		floorpic, ceilingpic;	/* if ceilingpic == -1,draw sky */

	uint8_t		lightlevel, special;

	VINT		validcount;			/* if == validcount, already checked */
	VINT		linecount;

	VINT		tag;

	mobj_t* soundtarget;		/* thing that made a sound (or null) */

	VINT		blockbox[4];		/* mapblock bounding box for height changes */
	VINT		soundorg[2];		/* for any sounds played by the sector */

	mobj_t* thinglist;			/* list of mobjs in sector */
	void* specialdata;		/* thinker_t for reversable actions */
	struct line_s** lines;			/* [linecount] size */
} sector_t;

typedef struct subsector_s
{
	VINT		numlines;
	VINT		firstline;
	sector_t* sector;
} subsector_t;

extern	vissprite_t* vissprites/*[MAXVISSPRITES]*/, * lastsprite_p, * vissprite_p;
extern boolean lowResMode;
extern int16_t viewportWidth, viewportHeight;
extern r_texcache_t r_texcache;
extern detailmode_t detailmode;
extern	viewdef_t	vd;
extern char clearscreen;

//
//was in soundst.h file
//
#define	SFXCHANNELS		6

typedef struct
{
	int     samples;
	int     loop_start;
	int     loop_end;
	int     info;
	int     unity;
	int		pitch_correction;
	int		decay_step;
	unsigned char data[1];
} sfx_t;

typedef void (*getsoundpos_t)(mobj_t*, fixed_t*);

typedef struct sfxinfo_s
{
	char singularity;	/* Sfx singularity (only one at a time) */
	unsigned char priority;		/* Sfx priority */
#ifdef MARS
	short lump;
#else
	int	pitch;		/* pitch if a link */
	int	volume;		/* volume if a link */
	struct sfxinfo_s* link;	/* referenced sound if a link */
	sfx_t* md_data;	/* machine-dependent sound data */
#endif
} sfxinfo_t;

typedef struct
{
#ifdef MARS
	uint8_t* data;
	int			position;
	int			increment;
	int			length;
	int			loop_length;
	int			prev_pos;			/* for adpcm decoding */
	uint8_t		volume;
	uint8_t		pan;
#else
	unsigned* source;				/* work in groups of 4 8 bit samples */
	int			startquad;
	int			stopquad;
	int			volume;				/* range from 0-32k */
#endif
	uint16_t	width;
	uint16_t	block_size; 		/* size of data block in bytes */
	int			remaining_bytes; 	/* WAV chunk */

	sfxinfo_t* sfx;
	mobj_t* mobj;
	getsoundpos_t getpos;
} sfxchannel_t;

//
//was in sounds.h
//
typedef enum
{
	sfx_None,
	sfx_pistol,
	sfx_shotgn,
	sfx_sgcock,
	sfx_plasma,
	sfx_bfg,
	sfx_sawup,
	sfx_sawidl,
	sfx_sawful,
	sfx_sawhit,
	sfx_rlaunc,
	sfx_rfly,
	sfx_rxplod,
	sfx_firsht,
	sfx_firbal,
	sfx_firxpl,
	sfx_pstart,
	sfx_pstop,
	sfx_doropn,
	sfx_dorcls,
	sfx_stnmov,
	sfx_swtchn,
	sfx_swtchx,
	sfx_plpain,
	sfx_dmpain,
	sfx_popain,
	sfx_slop,
	sfx_itemup,
	sfx_wpnup,
	sfx_oof,
	sfx_telept,
	sfx_posit1,
	sfx_posit2,
	sfx_posit3,
	sfx_bgsit1,
	sfx_bgsit2,
	sfx_sgtsit,
	sfx_cacsit,
	sfx_brssit,
	sfx_cybsit,
	sfx_spisit,
	sfx_sklatk,
	sfx_sgtatk,
	sfx_claw,
	sfx_pldeth,
	sfx_podth1,
	sfx_podth2,
	sfx_podth3,
	sfx_bgdth1,
	sfx_bgdth2,
	sfx_sgtdth,
	sfx_cacdth,
	sfx_skldth,
	sfx_brsdth,
	sfx_cybdth,
	sfx_spidth,
	sfx_posact,
	sfx_bgact,
	sfx_dmact,
	sfx_noway,
	sfx_barexp,
	sfx_punch,
	sfx_hoof,
	sfx_metal,
	sfx_itmbk,
	sfx_bdopn,
	sfx_bdcls,
	NUMSFX
} sfxenum_t;

//needed these to find them
static void S_ClearPCM(void);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
extern	sfxchannel_t	sfxchannels[SFXCHANNELS];
extern	const fixed_t		finesine_[5 * FINEANGLES / 4];
static void S_Update(int16_t* buffer);
void S_StartSound(mobj_t* mobj, int sound_id); //needed to call for sound
void S_StartPositionedSound(mobj_t* mobj, int sound_id, getsoundpos_t getpos); //needed to call for sound

//Needed for text.c/h
#define FBF_WIDTH 320


//For collison.c

//Collision types
//I'm using a collision tileset that is 1 row of 10 tiles that are 8x8.  These values maybe different depending on tile set
enum
{
	COLLISION_NONE = 0,
	COLLISION_BLOCK = 4,
	COLLISION_WARP = 8,
	COLLISION_DAMAGE = 16,
	COLLISION_BATTLE = 32,
	COLLISION_TREASTURE = 64,
	COLLISION_EVENT = 128 //Thinking aliens
};

//test for sound init
void S_Init(void);