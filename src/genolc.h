/************************************************************************
 * Generic OLC Library - General / genolc.h			v1.0	*
 * Original author: Levork						*
 * Copyright 1996 by Harvey Gilpin					*
 * Copyright 1997-1999 by George Greer (greerga@circlemud.org)		*
 ************************************************************************/

#define STRING_TERMINATOR       '~'

#define CONFIG_GENOLC_MOBPROG	0

#define LVL_BUILDER	LVL_IMMORT

int genolc_checkstring(struct descriptor_data *d, const char *arg);
int remove_from_save_list(zone_vnum, int type);
int add_to_save_list(zone_vnum, int type);
void strip_cr(char *);
void do_show_save_list(struct char_data *);
int save_all(void);
char *str_udup(const char *);
void copy_ex_descriptions(struct extra_descr_data **to, struct extra_descr_data *from);
void free_ex_descriptions(struct extra_descr_data *head);

struct save_list_data {
  int zone;
  int type;
  struct save_list_data *next;
};

extern struct save_list_data *save_list;

/* save_list_data.type */
#define SL_MOB	0
#define SL_OBJ	1
#define SL_SHP	2
#define SL_WLD	3
#define SL_ZON	4
#define SL_TRG  5
#define SL_HLP  6
#define SL_ACT  7
#define SL_MAX	7

#define ZCMD(zon, cmds)	zone_table[(zon)].cmd[(cmds)]


