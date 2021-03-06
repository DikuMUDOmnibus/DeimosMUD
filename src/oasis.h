/************************************************************************
 * OasisOLC - General / oasis.h					v2.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#define _OASISOLC	0x200   /* 2.0.0 */

/*
 * Used to determine what version of OasisOLC is installed.
 *
 * Ex: #if _OASISOLC >= OASIS_VERSION(2,0,0)
 */
#define OASIS_VERSION(x,y,z)	(((x) << 8 | (y) << 4 | (z))

/*
 * Set this to 1 to enable MobProg support.  MobProgs are available on
 * the CircleMUD FTP site in the contrib/code directory.
 */
#define CONFIG_OASIS_MPROG	0

void trigedit_save(struct descriptor_data *d);
int can_edit_zone(struct char_data *ch, int number);
#define ZCMD(zon, cmds) zone_table[(zon)].cmd[(cmds)] 
/* -------------------------------------------------------------------------- */

/*
 * Macros, defines, structs and globals for the OLC suite.  You will need
 * to adjust these numbers if you ever add more.
 */
#define NUM_ROOM_FLAGS 		TOPOFROOMFLAGS
#define NUM_ROOM_SECTORS	12

#define NUM_MOB_FLAGS		23
#define NUM_ATTACK_TYPES	15

#define NUM_ITEM_TYPES		33
#define NUM_ITEM_FLAGS		30
#define NUM_ITEM_WEARS 		15
#define NUM_APPLIES		25
#define NUM_LIQ_TYPES 		16
#define NUM_SPELLS		TOP_OF_SPELLS

#define NUM_GENDERS		3
#define NUM_SHOP_FLAGS 		2
#define NUM_TRADERS 		7

#if CONFIG_OASIS_MPROG
/*
 * Define this to how many MobProg scripts you have.
 */
#define NUM_PROGS		12
#endif

#define AEDIT_PERMISSION 999

/* -------------------------------------------------------------------------- */

/*
 * Limit information.
 */
#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	MAX_STRING_LENGTH - 100
#define MAX_EXIT_DESC	256
#define MAX_EXTRA_DESC  512
#define MAX_MOB_DESC	512
#define MAX_OBJ_DESC	512
#define MAX_HELP_KEYWORDS	75
#define MAX_HELP_ENTRY		1024

#define HEDIT_MAIN_MENU			1
#define HEDIT_ENTRY			2
#define HEDIT_MIN_LEVEL			3
#define HEDIT_KEYWORDS			4
#define HEDIT_CONFIRM_SAVESTRING 	5

#define HEDIT_PERMISSION	666	/* set people's olc_zone to	*
 					 * this to allow them to edit 	*
					 * help entries			*/
/* #define HEDIT_LIST		1 */	/* define to log saves		*/

/* -------------------------------------------------------------------------- */

extern int list_top;

/*
 * Utilities exported from olc.c.
 */
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_colors(struct char_data *ch);

/*
 * OLC structures.
 */

struct oasis_olc_data {
  int mode;
  int zone_num;
  int number;
  int value;
  int total_mprogs;
  struct char_data *mob;
  struct room_data *room;
  struct obj_data *obj;
  struct zone_data *zone;
  struct shop_data *shop;
  struct social_messg *action;
  struct obj_data *iobj;
  struct extra_descr_data *desc;
  struct help_index_element *help;
  struct clan_type *clan;
#if CONFIG_OASIS_MPROG
  struct mob_prog_data *mprog;
  struct mob_prog_data *mprogl;
#endif
  struct trig_data *trig;
  int script_mode;
  int trigger_position;
  int trigger_type;
  int item_type;
  struct trig_proto_list *script;
  char *storage;
  struct assembly_data *OlcAssembly;
  int spec_index;
};

/*
 * Exported globals.
 */
extern const char *nrm, *grn, *cyn, *yel;

/*
 * Descriptor access macros.
 */
#define OLC(d)		((struct oasis_olc_data *)(d)->olc)
#define OLC_MODE(d) 	(OLC(d)->mode)		/* Parse input mode.	*/
#define OLC_NUM(d) 	(OLC(d)->number)	/* Room/Obj VNUM.	*/
#define OLC_VAL(d) 	(OLC(d)->value)		/* Scratch variable.	*/
#define OLC_ZNUM(d) 	(OLC(d)->zone_num)	/* Real zone number.	*/
#define OLC_ROOM(d) 	(OLC(d)->room)		/* Room structure.	*/
#define OLC_OBJ(d) 	(OLC(d)->obj)		/* Object structure.	*/
#define OLC_IEDIT(d)    (OLC(d)->iobj)	 	/* iObject structure    */
#define OLC_ZONE(d)     (OLC(d)->zone)		/* Zone structure.	*/
#define OLC_MOB(d)	(OLC(d)->mob)		/* Mob structure.	*/
#define OLC_SHOP(d) 	(OLC(d)->shop)		/* Shop structure.	*/
#define OLC_DESC(d) 	(OLC(d)->desc)		/* Extra description.	*/
#define OLC_HELP(d)	(OLC(d)->help)   	/* help entries		*/
#define OLC_CLAN(d)     (OLC(d)->clan)          /* Clan structures      */
#if CONFIG_OASIS_MPROG
#define OLC_MPROG(d)	(OLC(d)->mprog)		/* Temporary MobProg.	*/
#define OLC_MPROGL(d)	(OLC(d)->mprogl)	/* MobProg list.	*/
#define OLC_MTOTAL(d)	(OLC(d)->total_mprogs)	/* Total mprog number.	*/
#endif
#define OLC_TRIG(d)     (OLC(d)->trig)        /* Trigger structure.   */
#define OLC_STORAGE(d)  (OLC(d)->storage)    /* For command storage  */
#define OLC_ACTION(d)   (OLC(d)->action) /* Action structure     */
#define OLC_ASSEDIT(d)  (OLC(d)->OlcAssembly)   /* assembly olc 	*/
#define OLC_SPEC(d)     (OLC(d)->spec_index)

/*
 * Other macros.
 */
#define OLC_EXIT(d)		(OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define GET_OLC_ZONE(c)	        ((c)->player_specials->saved.olc_zone)

/*
 * Cleanup types.
 */
#define CLEANUP_ALL		1	/* Free the whole lot.	*/
#define CLEANUP_STRUCTS 	2	/* Don't free strings.	*/

/*
 * Submodes of OEDIT connectedness.
 */
#define OEDIT_MAIN_MENU              	1
#define OEDIT_EDIT_NAMELIST          	2
#define OEDIT_SHORTDESC              	3
#define OEDIT_LONGDESC               	4
#define OEDIT_ACTDESC                	5
#define OEDIT_TYPE                   	6
#define OEDIT_EXTRAS                 	7
#define OEDIT_WEAR                  	8
#define OEDIT_WEIGHT                	9
#define OEDIT_COST                  	10
#define OEDIT_COSTPERDAY            	11
#define OEDIT_TIMER                 	12
#define OEDIT_VALUE_1               	13
#define OEDIT_VALUE_2               	14
#define OEDIT_VALUE_3               	15
#define OEDIT_VALUE_4               	16
#define OEDIT_APPLY                 	17
#define OEDIT_APPLYMOD              	18
#define OEDIT_EXTRADESC_KEY         	19
#define OEDIT_CONFIRM_SAVEDB        	20
#define OEDIT_CONFIRM_SAVESTRING    	21
#define OEDIT_PROMPT_APPLY          	22
#define OEDIT_EXTRADESC_DESCRIPTION 	23
#define OEDIT_EXTRADESC_MENU        	24
#define OEDIT_LEVEL                 	25
#define OEDIT_PERM			26
#define OEDIT_SPEC_PROC                 27


/*
 * Submodes of REDIT connectedness.
 */
#define REDIT_MAIN_MENU 		1
#define REDIT_NAME 			2
#define REDIT_DESC 			3
#define REDIT_FLAGS 			4
#define REDIT_SECTOR 			5
#define REDIT_EXIT_MENU 		6
#define REDIT_CONFIRM_SAVEDB 		7
#define REDIT_CONFIRM_SAVESTRING 	8
#define REDIT_EXIT_NUMBER 		9
#define REDIT_EXIT_DESCRIPTION 		10
#define REDIT_EXIT_KEYWORD 		11
#define REDIT_EXIT_KEY 			12
#define REDIT_EXIT_DOORFLAGS 		13
#define REDIT_EXTRADESC_MENU 		14
#define REDIT_EXTRADESC_KEY 		15
#define REDIT_EXTRADESC_DESCRIPTION 	16
#define REDIT_SPEC_PROC			17

/*
 * Submodes of ZEDIT connectedness.
 */
#define ZEDIT_MAIN_MENU              	0
#define ZEDIT_DELETE_ENTRY		1
#define ZEDIT_NEW_ENTRY			2
#define ZEDIT_CHANGE_ENTRY		3
#define ZEDIT_COMMAND_TYPE		4
#define ZEDIT_IF_FLAG			5
#define ZEDIT_ARG1			6
#define ZEDIT_ARG2			7
#define ZEDIT_ARG3			8
#define ZEDIT_ZONE_NAME			9
#define ZEDIT_ZONE_LIFE			10
#define ZEDIT_ZONE_TOP			11
#define ZEDIT_ZONE_RESET		12
#define ZEDIT_CONFIRM_SAVESTRING	13
#define ZEDIT_SARG1			14
#define ZEDIT_SARG2			15
#define ZEDIT_PROB			16
#define ZEDIT_PROB2			17
#define ZEDIT_ZONE_BUILDERS             18

/*
 * Submodes of MEDIT connectedness.
 */
#define MEDIT_MAIN_MENU              	0
#define MEDIT_ALIAS			1
#define MEDIT_S_DESC			2
#define MEDIT_L_DESC			3
#define MEDIT_D_DESC			4
#define MEDIT_NPC_FLAGS			5
#define MEDIT_AFF_FLAGS			6
#define MEDIT_CONFIRM_SAVESTRING	7

/*
 * Numerical responses.
 */
#define MEDIT_NUMERICAL_RESPONSE	10
#define MEDIT_SETTER                    11
#define MEDIT_SEX			12
#define MEDIT_HITROLL			13
#define MEDIT_DAMROLL			14
#define MEDIT_NDD			15
#define MEDIT_SDD			16
#define MEDIT_NUM_HP_DICE		17
#define MEDIT_SIZE_HP_DICE		18
#define MEDIT_ADD_HP			19
#define MEDIT_AC			20
#define MEDIT_EXP			21
#define MEDIT_GOLD			22
#define MEDIT_POS			23
#define MEDIT_DEFAULT_POS		24
#define MEDIT_ATTACK			25
#define MEDIT_LEVEL			26
#define MEDIT_ALIGNMENT			27
#if CONFIG_OASIS_MPROG
#define MEDIT_MPROG                     28
#define MEDIT_CHANGE_MPROG              29
#define MEDIT_MPROG_COMLIST             30
#define MEDIT_MPROG_ARGS                31
#define MEDIT_MPROG_TYPE                32
#define MEDIT_PURGE_MPROG               33
#endif
#define MEDIT_SPEC_PROC                 32

/*
 * Submodes of SEDIT connectedness.
 */
#define SEDIT_MAIN_MENU              	0
#define SEDIT_CONFIRM_SAVESTRING	1
#define SEDIT_NOITEM1			2
#define SEDIT_NOITEM2			3
#define SEDIT_NOCASH1			4
#define SEDIT_NOCASH2			5
#define SEDIT_NOBUY			6
#define SEDIT_BUY			7
#define SEDIT_SELL			8
#define SEDIT_PRODUCTS_MENU		11
#define SEDIT_ROOMS_MENU		12
#define SEDIT_NAMELIST_MENU		13
#define SEDIT_NAMELIST			14
/*
 * Numerical responses.
 */
#define SEDIT_NUMERICAL_RESPONSE	20
#define SEDIT_OPEN1			21
#define SEDIT_OPEN2			22
#define SEDIT_CLOSE1			23
#define SEDIT_CLOSE2			24
#define SEDIT_KEEPER			25
#define SEDIT_BUY_PROFIT		26
#define SEDIT_SELL_PROFIT		27
#define SEDIT_TYPE_MENU			29
#define SEDIT_DELETE_TYPE		30
#define SEDIT_DELETE_PRODUCT		31
#define SEDIT_NEW_PRODUCT		32
#define SEDIT_DELETE_ROOM		33
#define SEDIT_NEW_ROOM			34
#define SEDIT_SHOP_FLAGS		35
#define SEDIT_NOTRADE			36


#define ASSEDIT_DO_NOT_USE              0
#define ASSEDIT_MAIN_MENU               1
#define ASSEDIT_ADD_COMPONENT           2
#define ASSEDIT_EDIT_COMPONENT          3
#define ASSEDIT_DELETE_COMPONENT        4
#define ASSEDIT_EDIT_EXTRACT            5
#define ASSEDIT_EDIT_INROOM             6
#define ASSEDIT_EDIT_TYPES              7

 /* Submodes of CEDIT connectedness */
#define CEDIT_MAIN_MENU      0
#define CEDIT_CONFIRM_SAVE   1
#define CEDIT_NAME           2
#define CEDIT_LEADERSNAME    3
#define CEDIT_SPONSORSNAME   4
#define CEDIT_MBR_LOOK_STR   5
#define CEDIT_ENTR_ROOM      6
#define CEDIT_GUARD          7
#define CEDIT_DIRECTION      8

/* Submodes of AEDIT connectedness	*/
#define AEDIT_CONFIRM_SAVESTRING	0
#define AEDIT_CONFIRM_EDIT		1
#define AEDIT_CONFIRM_ADD		2
#define AEDIT_MAIN_MENU			3
#define AEDIT_ACTION_NAME		4
#define AEDIT_SORT_AS			5
#define AEDIT_MIN_CHAR_POS		6
#define AEDIT_MIN_VICT_POS		7
#define AEDIT_HIDDEN_FLAG		8
#define AEDIT_MIN_CHAR_LEVEL		9
#define AEDIT_NOVICT_CHAR		10
#define AEDIT_NOVICT_OTHERS		11
#define AEDIT_VICT_CHAR_FOUND		12
#define AEDIT_VICT_OTHERS_FOUND		13
#define AEDIT_VICT_VICT_FOUND		14
#define AEDIT_VICT_NOT_FOUND		15
#define AEDIT_SELF_CHAR			16
#define AEDIT_SELF_OTHERS		17
#define AEDIT_VICT_CHAR_BODY_FOUND     	18
#define AEDIT_VICT_OTHERS_BODY_FOUND   	19
#define AEDIT_VICT_VICT_BODY_FOUND     	20
#define AEDIT_OBJ_CHAR_FOUND		21
#define AEDIT_OBJ_OTHERS_FOUND		22

/*
 * Prototypes to keep.
 */
void clear_screen(struct descriptor_data *);
/* ACMD(do_oasis); */

/*
 * Prototypes, to be moved later.
 */
void medit_free_mobile(struct char_data *mob);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void init_mobile(struct char_data *mob);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(zone_vnum zone_num);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
void medit_disp_mprog_types(struct descriptor_data *d);
void medit_disp_sex(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_menu(struct descriptor_data *d);
void medit_parse(struct descriptor_data *d, char *arg);
void medit_string_cleanup(struct descriptor_data *d, int terminator);

void hedit_string_cleanup(struct descriptor_data *d, int terminator);

void oedit_setup_new(struct descriptor_data *d);
void oedit_setup_existing(struct descriptor_data *d, int real_num);
void oedit_save_internally(struct descriptor_data *d);
void oedit_save_to_disk(int zone_num);
void oedit_disp_container_flags_menu(struct descriptor_data *d);
void oedit_disp_extradesc_menu(struct descriptor_data *d);
void oedit_disp_prompt_apply_menu(struct descriptor_data *d);
void oedit_liquid_type(struct descriptor_data *d);
void oedit_disp_apply_menu(struct descriptor_data *d);
void oedit_disp_weapon_menu(struct descriptor_data *d);
void oedit_disp_spells_menu(struct descriptor_data *d);
void oedit_disp_val1_menu(struct descriptor_data *d);
void oedit_disp_val2_menu(struct descriptor_data *d);
void oedit_disp_val3_menu(struct descriptor_data *d);
void oedit_disp_val4_menu(struct descriptor_data *d);
void oedit_disp_type_menu(struct descriptor_data *d);
void oedit_disp_extra_menu(struct descriptor_data *d);
void oedit_disp_wear_menu(struct descriptor_data *d);
void oedit_disp_menu(struct descriptor_data *d);
void oedit_parse(struct descriptor_data *d, char *arg);
void oedit_disp_perm_menu(struct descriptor_data *d);
void oedit_string_cleanup(struct descriptor_data *d, int terminator);

void iedit_setup_existing(struct descriptor_data *d, struct obj_data *obj);
void iedit_parse(struct descriptor_data *d, char *arg);

void redit_string_cleanup(struct descriptor_data *d, int terminator);
void redit_setup_new(struct descriptor_data *d);
void redit_setup_existing(struct descriptor_data *d, int real_num);
void redit_save_internally(struct descriptor_data *d);
void redit_save_to_disk(zone_vnum zone_num);
void redit_disp_extradesc_menu(struct descriptor_data *d);
void redit_disp_exit_menu(struct descriptor_data *d);
void redit_disp_exit_flag_menu(struct descriptor_data *d);
void redit_disp_flag_menu(struct descriptor_data *d);
void redit_disp_sector_menu(struct descriptor_data *d);
void redit_disp_menu(struct descriptor_data *d);
void redit_parse(struct descriptor_data *d, char *arg);
void free_room(struct room_data *room);

void sedit_setup_new(struct descriptor_data *d);
void sedit_setup_existing(struct descriptor_data *d, int rshop_num);
void sedit_save_internally(struct descriptor_data *d);
void sedit_save_to_disk(int zone_num);
void sedit_products_menu(struct descriptor_data *d);
void sedit_compact_rooms_menu(struct descriptor_data *d);
void sedit_rooms_menu(struct descriptor_data *d);
void sedit_namelist_menu(struct descriptor_data *d);
void sedit_shop_flags_menu(struct descriptor_data *d);
void sedit_no_trade_menu(struct descriptor_data *d);
void sedit_types_menu(struct descriptor_data *d);
void sedit_disp_menu(struct descriptor_data *d);
void sedit_parse(struct descriptor_data *d, char *arg);
void aedit_parse(struct descriptor_data *d, char *arg);

void zedit_setup(struct descriptor_data *d, int room_num);
void zedit_new_zone(struct char_data *ch, int vzone_num);
void zedit_create_index(int znum, char *type);
void zedit_save_internally(struct descriptor_data *d);
void zedit_save_to_disk(int zone_num);
void zedit_disp_menu(struct descriptor_data *d);
void zedit_disp_comtype(struct descriptor_data *d);
void zedit_disp_arg1(struct descriptor_data *d);
void zedit_disp_arg2(struct descriptor_data *d);
void zedit_disp_arg3(struct descriptor_data *d);
void zedit_parse(struct descriptor_data *d, char *arg);


 /* cedit prototypes */
void cedit_disp_menu(struct descriptor_data * d);
void cedit_disp_dir_menu(struct descriptor_data * d);
void cedit_free_clan(struct clan_type *cptr);
void cedit_setup_new(struct descriptor_data *d);
void cedit_parse(struct descriptor_data *d, char *arg);
void save_clans(void);
void load_clans(void);
