/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects for rent and crash-save           *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5

extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int rent_file_timeout, crash_file_timeout;
extern int free_rent;
extern int min_rent_cost;
extern int max_obj_save;	/* change in config.c */
extern int xap_objs;

/* Extern functions */
ACMD(do_action);
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
int invalid_class(struct char_data *ch, struct obj_data *obj);
void add_llog_entry(struct char_data *ch, int type);

/* local functions */
void Crash_extract_norent_eq(struct char_data *ch);
void auto_equip(struct char_data *ch, struct obj_data *obj, int location);
int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist, int display, int factor);
int Crash_report_unrentables(struct char_data * ch, struct char_data * recep, struct obj_data * obj);
void Crash_report_rent(struct char_data * ch, struct char_data * recep, struct obj_data * obj, long *cost, long *nitems, int display, int factor);
struct obj_data *Obj_from_store(struct obj_file_elem object, int *location);
int Obj_to_store(struct obj_data * obj, FILE * fl, int location);
void update_obj_file(void);
int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent);
int gen_receptionist(struct char_data * ch, struct char_data * recep, int cmd, char *arg, int mode);
int Crash_save(struct obj_data * obj, FILE * fp, int location);
void Crash_rent_deadline(struct char_data * ch, struct char_data * recep, long cost);
void Crash_restore_weight(struct obj_data * obj);
void Crash_extract_objs(struct obj_data * obj);
int Crash_is_unrentable(struct obj_data * obj);
void Crash_extract_norents(struct obj_data * obj);
void Crash_extract_expensive(struct obj_data * obj);
void Crash_calculate_rent(struct obj_data * obj, int *cost);
void Crash_rentsave(struct char_data * ch, int cost);
void Crash_cryosave(struct char_data * ch, int cost);


struct obj_data *Obj_from_store(struct obj_file_elem object, int *location)
{
  struct obj_data *obj;
  int j;

  *location = 0;
  if (real_object(object.item_number) >= 0) {
    obj = read_object(object.item_number, VIRTUAL);
#if USE_AUTOEQ
    *location = object.location;
#endif
    GET_OBJ_VAL(obj, 0) = object.value[0];
    GET_OBJ_VAL(obj, 1) = object.value[1];
    GET_OBJ_VAL(obj, 2) = object.value[2];
    GET_OBJ_VAL(obj, 3) = object.value[3];
    GET_OBJ_EXTRA(obj) = object.extra_flags;
    GET_OBJ_WEIGHT(obj) = object.weight;
    GET_OBJ_TIMER(obj) = object.timer;
    GET_OBJ_PERM(obj) = object.bitvector;
    obj->obj_flags.bitvector = object.bitvector;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = object.affected[j];

    return (obj);
  } else
    return (NULL);
}



int Obj_to_store(struct obj_data * obj, FILE * fl, int location)
{
  int j;
  struct obj_file_elem object;

  if(!xap_objs) {
    object.item_number = GET_OBJ_VNUM(obj);
#if USE_AUTOEQ
    object.location = location;
#endif
  object.value[0] = GET_OBJ_VAL(obj, 0);
  object.value[1] = GET_OBJ_VAL(obj, 1);
  object.value[2] = GET_OBJ_VAL(obj, 2);
  object.value[3] = GET_OBJ_VAL(obj, 3);
  object.extra_flags = GET_OBJ_EXTRA(obj);
  object.weight = GET_OBJ_WEIGHT(obj);
  object.timer = GET_OBJ_TIMER(obj);
  object.bitvector = GET_OBJ_PERM(obj);
  object.bitvector = obj->obj_flags.bitvector;
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    object.affected[j] = obj->affected[j];

  if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1) {
    perror("SYSERR: error writing object in Obj_to_store");
    return (0);
   }
  } else {
    my_obj_save_to_disk(fl, obj, location);
   }
  return (1);
}

void auto_equip(struct char_data *ch, struct obj_data *obj, int location)
{
  int j;

  /* Lots of checks... */
  if (location > 0) {	/* Was wearing it. */
    switch (j = (location - 1)) {
    case WEAR_LIGHT:
      break;
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
        location = LOC_INVENTORY;
      break;
    case WEAR_NECK_1:
    case WEAR_NECK_2:
      if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
        location = LOC_INVENTORY;
      break;
    case WEAR_BODY:
      if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
        location = LOC_INVENTORY;
      break;
    case WEAR_HEAD:
      if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
        location = LOC_INVENTORY;
      break;
    case WEAR_LEGS:
      if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
        location = LOC_INVENTORY;
      break;
    case WEAR_FEET:
      if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
        location = LOC_INVENTORY;
      break;
    case WEAR_HANDS:
      if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
        location = LOC_INVENTORY;
      break;
    case WEAR_ARMS:
      if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
        location = LOC_INVENTORY;
      break;
    case WEAR_SHIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_ABOUT:
      if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
        location = LOC_INVENTORY;
      break;
    case WEAR_WAIST:
      if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WRIST_R:
    case WEAR_WRIST_L:
      if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
        location = LOC_INVENTORY;
      break;
    case WEAR_WIELD:
      if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
        location = LOC_INVENTORY;
      break;
    case WEAR_HOLD:
      if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
	break;
      if (IS_WARRIOR(ch) && CAN_WEAR(obj, ITEM_WEAR_WIELD) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	break;
      location = LOC_INVENTORY;
      break;
    default:
      location = LOC_INVENTORY;
    }

    if (location > 0) {	    /* Wearable. */
      if (!GET_EQ(ch,j)) {
	/*
	 * Check the characters's alignment to prevent them from being
	 * zapped through the auto-equipping.
         */
         if (invalid_align(ch, obj) || invalid_class(ch, obj))
          location = LOC_INVENTORY;
        else
          equip_char(ch, obj, j);
      } else {	/* Oops, saved a player with double equipment? */
        char aeq[128];
        sprintf(aeq, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
        mudlog(aeq, BRF, LVL_IMMORT, TRUE);
        location = LOC_INVENTORY;
      }
    }
  }
  if (location <= 0)	/* Inventory */
    obj_to_char(obj, ch);
}


int Crash_delete_file(char *name)
{
  return (1);
  /*  
  char filename[50];
  FILE *fl;
  if(!xap_objs) {
  if (!get_filename(name, filename, CRASH_FILE))
    return (0);
  } else {
    if(!get_filename(name,filename,NEW_OBJ_FILES))
      return (0);
  }
  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT)	// if it fails but NOT because of no file 
      log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
    return (0);
  }
  fclose(fl);

 //  if it fails, NOT because of no file 
  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));

  return (1);
  */
}


int Crash_delete_crashfile(struct char_data * ch)
{
  char fname[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fl;
  int rentcode,timed,netcost,gold,account,nitems;
  char line[MAX_INPUT_LENGTH];

  if(!xap_objs) {
    if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
      return (0);
  } else {
    if(!get_filename(GET_NAME(ch), fname,NEW_OBJ_FILES))
      return (0);
  }

  if (!(fl = fopen(fname, "rb"))) {
    if (errno != ENOENT)	/* if it fails, NOT because of no file */
      log("SYSERR: checking for crash file %s (3): %s", fname, strerror(errno));
    return (0);
  }

  if(!xap_objs) {
    if (!feof(fl))
      fread(&rent, sizeof(struct rent_info), 1, fl);
    fclose(fl);
  } else {
    if (!feof(fl))
      get_line(fl,line);
    sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,&gold,
        &account,&nitems);
    fclose(fl);
  }

  if(!xap_objs) {
    if (rent.rentcode == RENT_CRASH)
      Crash_delete_file(GET_NAME(ch));
  } else {
    if (rentcode == RENT_CRASH)
      Crash_delete_file(GET_NAME(ch));
  }

  return (1);
}


int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH], filetype[20];
  struct rent_info rent;
  FILE *fl;
  int rentcode,timed,netcost,gold,account,nitems;
  char line[MAX_STRING_LENGTH];

  if(!xap_objs) {
    if (!get_filename(name, fname, CRASH_FILE))
      return (0);
  } else {
    if (!get_filename(name, fname, NEW_OBJ_FILES))
      return (0);
  }
 
  /*
   * open for write so that permission problems will be flagged now, at boot
   * time.
   */
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT)	/* if it fails, NOT because of no file */
      log("SYSERR: OPENING OBJECT FILE %s (4): %s", fname, strerror(errno));
    return (0);
  }

  if(!xap_objs) {
    if (!feof(fl))
      fread(&rent, sizeof(struct rent_info), 1, fl);
    fclose(fl);
  } else {
    if (!feof(fl))
      get_line(fl,line);
      sscanf(line, "%d %d %d %d %d %d",&rentcode,&timed,&netcost,
        &gold,&account,&nitems);
    fclose(fl);
  }

  if(!xap_objs) {
    rentcode=rent.rentcode;
    timed=rent.time;
  }

  if ((rentcode == RENT_CRASH) ||
      (rentcode == RENT_FORCED) || (rentcode == RENT_TIMEDOUT)) {
    if (timed < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      switch (rentcode) {
      case RENT_CRASH:
	strcpy(filetype, "crash");
	break;
      case RENT_FORCED:
	strcpy(filetype, "forced rent");
	break;
      case RENT_TIMEDOUT:
	strcpy(filetype, "idlesave");
	break;
      default:
	strcpy(filetype, "UNKNOWN!");
	break;
      }
      log("    Deleting %s's %s file.", name, filetype);
      return (1);
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (rentcode == RENT_RENTED)
    if (timed < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_file(name);
      log("    Deleting %s's rent file.", name);
      return (1);
    }
  return (0);
}



void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name)
      Crash_clean_file(player_table[i].name);
}



void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *obj;
  struct rent_info rent;
  int rentcode,timed,netcost,gold,account,nitems;
  int t[10],nr;
  char line[MAX_STRING_LENGTH]; 
  char *sdesc;

  if(!xap_objs) {
    if (!get_filename(name, fname, CRASH_FILE))
      return;
  } else {
     if (!get_filename(name, fname, NEW_OBJ_FILES))
      return;
  }

  if (!(fl = fopen(fname, "rb"))) {
    sprintf(buf, "%s has no rent file.\r\n", name);
    send_to_char(buf, ch);
    return;
  }
  sprintf(buf, "%s\r\n", fname);
 
  if (!feof(fl)) {
    if(!xap_objs) {
      fread(&rent, sizeof(struct rent_info), 1, fl);
    } else {
      get_line(fl,line);
      sscanf(line,"%d %d %d %d %d %d",&rentcode,&timed,&netcost,
                 &gold,&account,&nitems);
    }
  }

  if(!xap_objs) {
    rentcode=rent.rentcode;
  }

  switch (rent.rentcode) {
  case RENT_RENTED:
    strcat(buf, "Rent\r\n");
    break;
  case RENT_CRASH:
    strcat(buf, "Crash\r\n");
    break;
  case RENT_CRYO:
    strcat(buf, "Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    strcat(buf, "TimedOut\r\n");
    break;
  default:
    strcat(buf, "Undef\r\n");
    break;
  }

 if(!xap_objs) {
  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      fclose(fl);
      return;
    }
    if (!feof(fl))
      if (real_object(object.item_number) > -1) {
	obj = read_object(object.item_number, VIRTUAL);
#if USE_AUTOEQ
	sprintf(buf + strlen(buf), " [%5d] (%5dau) <%2d> %-20s\r\n",
		object.item_number, GET_OBJ_RENT(obj),
		object.location, obj->short_description);
#else
	sprintf(buf + strlen(buf), " [%5d] (%5dau) %-20s\r\n",
		object.item_number, GET_OBJ_RENT(obj),
		obj->short_description);
#endif
	extract_obj(obj);
	if (strlen(buf) > MAX_STRING_LENGTH - 80) {
	  strcat(buf, "** Excessive rent listing. **\r\n");
	  break;
	}
        }
    }
  } else {      /* else we have xap objs */
    while(!feof(fl)) {
      get_line(fl,line);
      if(*line == '#') { /* swell - its an item */
        sscanf(line,"#%d",&nr);
        if(nr != NOTHING) {  /* then we can dispense with it easily */
          obj=read_object(nr,VIRTUAL);
          sprintf(buf,"%s[%5d] (%5dau) %-20s\r\n",buf,
                  nr, GET_OBJ_RENT(obj),
                  obj->short_description);
          extract_obj(obj);
        } else { /* its nothing, and a unique item. bleh. partial parse.*/
          get_line(fl,line);    /* this is obj+val */
          get_line(fl,line);    /* this is XAP */
          sprintf(buf2,"xap objects in listrent");
          fread_string(fl,buf2);  /* screw the name */
          sdesc=fread_string(fl,buf2);
          fread_string(fl,buf2); /* screw the long desc */
          fread_string(fl,buf2); /* screw the action desc. */
          get_line(fl,line);    /* this is an important line.rent..*/
          sscanf(line,"%d %d %d %d %d",t,t+1,t+2,t+3,t+4);
          /* great we got it all, make the buf */
          sprintf(buf,"%s[%5d] (%5dau) %-20s\r\n",buf,
                  nr, t[4],sdesc);
         /* best of all, we don't care if there's descs, or stuff..*/
         /* since we're only doing operations on lines beginning in # */
        /* i suppose you don't want to make exdescs start with # .:) */
        }
      }
    }
  }
        /* why would you have send_to_char here anyway?!?! */
  page_string(ch->desc,buf,0);

  fclose(fl);
}



int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent)
{
  if(!xap_objs) {
    if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1) {
      perror("SYSERR: writing rent code");
      return 0;
    }
  } else {
    if(fprintf(fl,"%d %d %d %d %d %d\r\n",rent->rentcode, rent->time,
        rent->net_cost_per_diem,rent->gold,rent->account,rent->nitems) < 1) {
      perror("Syserr: Writing rent code");
      return 0;
    }
  }
  return (1);
}



/*
 * Return values:
 *  0 - successful load, keep char in rent room.
 *  1 - load failure or load of crash items -- put char in temple.
 *  2 - rented equipment lost (no $)
 */
int Crash_load(struct char_data * ch)
{
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct rent_info rent;
//  int cost;
  int orig_rent_code, num_objs = 0, j;
//  float num_of_days;
  /* AutoEQ addition. */
  struct obj_data *obj, *obj2, *cont_row[MAX_BAG_ROWS];
  int location;

  if(xap_objs) {
    return (Crash_load_xapobjs(ch));
  }

  /* Empty all of the container lists (you never know ...) */
  for (j = 0; j < MAX_BAG_ROWS; j++)
    cont_row[j] = NULL;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE))
    return (1);
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      log("SYSERR: READING OBJECT FILE %s (5): %s", fname, strerror(errno));
      send_to_char("\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n", ch);
    }
    sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    return (1);
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  else {
    log("SYSERR: Crash_load: %s's rent file was empty!", GET_NAME(ch));
    return (1);
  }

  switch (orig_rent_code = rent.rentcode) {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "SYSERR: %s entering game with undefined rent code %d.", GET_NAME(ch), rent.rentcode);
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }

  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      perror("SYSERR: Reading crash file: Crash_load");
      fclose(fl);
      return (1);
    }
    if (feof(fl))
      break;
    ++num_objs;
    if ((obj = Obj_from_store(object, &location)) == NULL)
      continue;

    auto_equip(ch, obj, location);
    /*
     * What to do with a new loaded item:
     *
     * If there's a list with location less than 1 below this, then its
     * container has disappeared from the file so we put the list back into
     * the character's inventory. (Equipped items are 0 here.)
     *
     * If there's a list of contents with location of 1 below this, then we
     * check if it is a container:
     *   - Yes: Get it from the character, fill it, and give it back so we
     *          have the correct weight.
     *   -  No: The container is missing so we put everything back into the
     *          character's inventory.
     *
     * For items with negative location, we check if there is already a list
     * of contents with the same location.  If so, we put it there and if not,
     * we start a new list.
     *
     * Since location for contents is < 0, the list indices are switched to
     * non-negative.
     *
     * This looks ugly, but it works.
     */
    if (location > 0) {		/* Equipped */
      for (j = MAX_BAG_ROWS - 1; j > 0; j--) {
        if (cont_row[j]) {	/* No container, back to inventory. */
          for (; cont_row[j]; cont_row[j] = obj2) {
            obj2 = cont_row[j]->next_content;
            obj_to_char(cont_row[j], ch);
          }
          cont_row[j] = NULL;
        }
      }
      if (cont_row[0]) {	/* Content list existing. */
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
	/* Remove object, fill it, equip again. */
          obj = unequip_char(ch, location - 1);
          obj->contains = NULL;	/* Should be NULL anyway, but just in case. */
          for (; cont_row[0]; cont_row[0] = obj2) {
            obj2 = cont_row[0]->next_content;
            obj_to_obj(cont_row[0], obj);
          }
          equip_char(ch, obj, location - 1);
        } else {			/* Object isn't container, empty the list. */
          for (; cont_row[0]; cont_row[0] = obj2) {
            obj2 = cont_row[0]->next_content;
            obj_to_char(cont_row[0], ch);
          }
          cont_row[0] = NULL;
        }
      }
    } else {	/* location <= 0 */
      for (j = MAX_BAG_ROWS - 1; j > -location; j--) {
        if (cont_row[j]) {	/* No container, back to inventory. */
          for (; cont_row[j]; cont_row[j] = obj2) {
            obj2 = cont_row[j]->next_content;
            obj_to_char(cont_row[j], ch);
          }
          cont_row[j] = NULL;
        }
      }
      if (j == -location && cont_row[j]) {	/* Content list exists. */
        if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
		/* Take the item, fill it, and give it back. */
          obj_from_char(obj);
          obj->contains = NULL;
          for (; cont_row[j]; cont_row[j] = obj2) {
            obj2 = cont_row[j]->next_content;
            obj_to_obj(cont_row[j], obj);
          }
          obj_to_char(obj, ch);	/* Add to inventory first. */
        } else {	/* Object isn't container, empty content list. */
          for (; cont_row[j]; cont_row[j] = obj2) {
            obj2 = cont_row[j]->next_content;
            obj_to_char(cont_row[j], ch);
          }
          cont_row[j] = NULL;
        }
      }
      if (location < 0 && location >= -MAX_BAG_ROWS) {
        /*
         * Let the object be part of the content list but put it at the
         * list's end.  Thus having the items in the same order as before
         * the character rented.
         */
        obj_from_char(obj);
        if ((obj2 = cont_row[-location - 1]) != NULL) {
          while (obj2->next_content)
            obj2 = obj2->next_content;
          obj2->next_content = obj;
        } else
          cont_row[-location - 1] = obj;
      }
    }
  }

  /* Little hoarding check. -gg 3/1/98 */
  sprintf(fname, "%s (level %d) has %d objects (max %d).",
	GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);
  mudlog(fname, NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);

  sprintf(fname, "&R%s has connected to DeimosMUD.&n", GET_NAME(ch));
 if (!PRF_FLAGGED(ch, PRF_WHOINVIS))
  showplayer(fname, BRF, MAX(0, GET_INVIS_LEV(ch)), TRUE);


  /* turn this into a crash file by re-writing the control block */
  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  rewind(fl);
  Crash_write_rentcode(ch, fl, &rent);

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return (0);
  else
    return (1);
}



int Crash_save(struct obj_data * obj, FILE * fp, int location)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->next_content, fp, location);
    Crash_save(obj->contains, fp, MIN(0, location) - 1);
    result = Obj_to_store(obj, fp, location);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return (0);
  }
  return (TRUE);
}


void Crash_restore_weight(struct obj_data * obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/*
 * Get !RENT items from equipment to inventory and
 * extract !RENT out of worn containers.
 */
void Crash_extract_norent_eq(struct char_data *ch)
{
  int j;

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j) == NULL)
      continue;

    if (Crash_is_unrentable(GET_EQ(ch, j)))
      obj_to_char(unequip_char(ch, j), ch);
    else
      Crash_extract_norents(GET_EQ(ch, j));
  }
}

void Crash_extract_objs(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}


int Crash_is_unrentable(struct obj_data * obj)
{
  if (!obj)
    return (0);

  if (IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0 ||
      GET_OBJ_RNUM(obj) <= NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY ||
         (GET_OBJ_RNUM(obj) <= NOTHING && 
         !IS_SET(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE))) {
    return (1);
   }

  return (0);
}


void Crash_extract_norents(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}


void Crash_extract_expensive(struct obj_data * obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}



void Crash_calculate_rent(struct obj_data * obj, int *cost)
{
  if (obj) {
    *cost += MAX(0, GET_OBJ_RENT(obj));
    Crash_calculate_rent(obj->contains, cost);
    Crash_calculate_rent(obj->next_content, cost);
  }
}


void Crash_crashsave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

   if(!xap_objs) {
    if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
      return;
  } else {
    if (!get_filename(GET_NAME(ch), buf, NEW_OBJ_FILES))
      return;
  }

  if (!(fp = fopen(buf, "wb")))
    return;

  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
 
  if(!xap_objs) {
    if (!Crash_write_rentcode(ch, fp, &rent)) {
      fclose(fp);
      return;
    }
  } else {
    fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
    rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);
  }

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
	fclose(fp);
	return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
    }

  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  Crash_restore_weight(ch->carrying);

  fclose(fp);
  REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}


void Crash_idlesave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  int cost, cost_eq;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if(!xap_objs) {
    if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
      return;
  } else {
    if (!get_filename(GET_NAME(ch), buf, NEW_OBJ_FILES))
      return;
  }

  if (!(fp = fopen(buf, "wb")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  cost = 0;
  Crash_calculate_rent(ch->carrying, &cost);

  cost_eq = 0;
  for (j = 0; j < NUM_WEARS; j++)
    Crash_calculate_rent(GET_EQ(ch, j), &cost_eq);

  cost += cost_eq;
  cost *= 2;			/* forcerent cost is 2x normal rent */

  if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
    for (j = 0; j < NUM_WEARS; j++)	/* Unequip players with low gold. */
      if (GET_EQ(ch, j))
        obj_to_char(unequip_char(ch, j), ch);

    while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
      Crash_extract_expensive(ch->carrying);
      cost = 0;
      Crash_calculate_rent(ch->carrying, &cost);
      cost *= 2;
    }
  }

  if (ch->carrying == NULL) {
    for (j = 0; j < NUM_WEARS && GET_EQ(ch, j) == NULL; j++) /* Nothing */ ;
    if (j == NUM_WEARS) {	/* No equipment or inventory. */
      fclose(fp);
      Crash_delete_file(GET_NAME(ch));
      return;
    }
  }
  rent.net_cost_per_diem = cost;

  rent.rentcode = RENT_TIMEDOUT;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if(!xap_objs) {
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
    }
  } else {
    fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
        rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);
   }

  for (j = 0; j < NUM_WEARS; j++) {
    if (GET_EQ(ch, j)) {
      if (!Crash_save(ch->carrying, fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_rentsave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if(!xap_objs) {
    if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
      return;
  } else {
    if (!get_filename(GET_NAME(ch), buf, NEW_OBJ_FILES))
      return;
  }

  if (!(fp = fopen(buf, "wb")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  rent.net_cost_per_diem = cost;
  rent.rentcode = RENT_RENTED;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
   if(!xap_objs) {
    if (!Crash_write_rentcode(ch, fp, &rent)) {
      fclose(fp);
      return;
    }
  } else {
    fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
        rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);
  }
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch,j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
}


void Crash_cryosave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

   if(!xap_objs) {
    if (!get_filename(GET_NAME(ch), buf, CRASH_FILE))
      return;
  } else {
    if (!get_filename(GET_NAME(ch), buf, NEW_OBJ_FILES))
      return;
  }

  if (!(fp = fopen(buf, "wb")))
    return;

  Crash_extract_norent_eq(ch);
  Crash_extract_norents(ch->carrying);

  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  rent.rentcode = RENT_CRYO;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  rent.net_cost_per_diem = 0;
  if(!xap_objs) {
    if (!Crash_write_rentcode(ch, fp, &rent)) {
      fclose(fp);
      return;
    }
  } else {
    fprintf(fp,"%d %d %d %d %d %d\r\n",rent.rentcode,rent.time,
        rent.net_cost_per_diem,rent.gold,rent.account,rent.nitems);

  }
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j)) {
      if (!Crash_save(GET_EQ(ch, j), fp, j + 1)) {
        fclose(fp);
        return;
      }
      Crash_restore_weight(GET_EQ(ch, j));
      Crash_extract_objs(GET_EQ(ch, j));
    }
  if (!Crash_save(ch->carrying, fp, 0)) {
    fclose(fp);
    return;
  }
  fclose(fp);

  Crash_extract_objs(ch->carrying);
  SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data * ch, struct char_data * recep,
			      long cost)
{
  long rent_deadline;

  if (!cost)
    return;

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  sprintf(buf,
      "$n tells you, 'You can rent for %ld day%s with the gold you have\r\n"
	  "on hand and in the bank.'\r\n",
	  rent_deadline, (rent_deadline > 1) ? "s" : "");
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
			         struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      has_norents = 1;
      sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
      act(buf, FALSE, recep, 0, ch, TO_VICT);
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}



void Crash_report_rent(struct char_data * ch, struct char_data * recep,
		            struct obj_data * obj, long *cost, long *nitems, int display, int factor)
{
  static char buf[256];

  if (obj) {
    if (!Crash_is_unrentable(obj)) {
      (*nitems)++;
      *cost += MAX(0, (GET_OBJ_RENT(obj) * factor));
      if (display) {
	sprintf(buf, "$n tells you, '%5d coins for %s..'",
		(GET_OBJ_RENT(obj) * factor), OBJS(obj, ch));
	act(buf, FALSE, recep, 0, ch, TO_VICT);
      }
    }
    Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
    Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
  }
}



int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist,
		         int display, int factor)
{
  char buf[MAX_INPUT_LENGTH];
  int i;
  long totalcost = 0, numitems = 0, norent;

  norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

  if (norent)
    return (0);

  totalcost = min_rent_cost * factor;

  Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < NUM_WEARS; i++)
    Crash_report_rent(ch, receptionist, GET_EQ(ch, i), &totalcost, &numitems, display, factor);

  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
	FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (numitems > max_obj_save) {
    sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
	    max_obj_save);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (display) {
    sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
	    min_rent_cost * factor);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
	    totalcost, (factor == RENT_FACTOR ? " per day" : ""));
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    if (totalcost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, receptionist, 0, ch, TO_VICT);
      return (0);
    } else if (factor == RENT_FACTOR)
      Crash_rent_deadline(ch, receptionist, totalcost);
  }
  return (totalcost);
}



int gen_receptionist(struct char_data * ch, struct char_data * recep,
		         int cmd, char *arg, int mode)
{
  int cost;
  room_rnum save_room;
  const char *action_table[] = { "smile", "dance", "sigh", "blush", "burp",
	  "cough", "fart", "twiddle", "yawn" };

  if (!ch->desc || IS_NPC(ch))
    return (FALSE);

  if (!cmd && !number(0, 5)) {
    do_action(recep, NULL, find_command(action_table[number(0, 8)]), 0);
    return (FALSE);
  }
  if (!CMD_IS("offer") && !CMD_IS("rent"))
    return (FALSE);
  if (!AWAKE(recep)) {
    sprintf(buf, "%s is unable to talk to you...\r\n", HESH(recep));
    send_to_char(buf, ch);
    return (TRUE);
  }
  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return (TRUE);
  }
  if (free_rent) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return (1);
  }
  if (CMD_IS("rent")) {
    if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
      return (TRUE);
    if (mode == RENT_FACTOR)
      sprintf(buf, "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
    else if (mode == CRYO_FACTOR)
      sprintf(buf, "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return (TRUE);
    }
    if (cost && (mode == RENT_FACTOR))
      Crash_rent_deadline(ch, recep, cost);

    if (mode == RENT_FACTOR) {
      act("$n stores your belongings and helps you into your private chamber.",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_rentsave(ch, cost);
      sprintf(buf, "%s has rented (%d/day, %d tot.)", GET_NAME(ch),
	      cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
    } else {			/* cryo */
      act("$n stores your belongings and helps you into your private chamber.\r\n"
	  "A white mist appears in the room, chilling you to the bone...\r\n"
	  "You begin to lose consciousness...",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_cryosave(ch, cost);
      sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
      SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
    }

    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);
    save_room = ch->in_room;
    extract_char(ch);
    save_char(ch, save_room);
  } else {
    Crash_offer_rent(ch, recep, TRUE, mode);
    act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
  }
  return (TRUE);
}


SPECIAL(receptionist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, CRYO_FACTOR));
}


void Crash_save_all(void)
{
  struct descriptor_data *d;
  extern int circle_shutdown;
  extern int circle_reboot;

  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
	Crash_crashsave(d->character);
	save_char(d->character, NOWHERE);
	if(circle_shutdown) {
	  if(circle_reboot) {
	     add_llog_entry(d->character,LAST_REBOOT);
          } else {
             add_llog_entry(d->character,LAST_SHUTDOWN);
	  }
        }
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}

int Crash_load_xapobjs(struct char_data *ch) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char line[256];
  int t[10],danger,zwei=0;
  int orig_rent_code;
  struct obj_data *temp;
  int locate=0, j, nr,k,num_objs=0, i;
  struct obj_data *obj1;
  struct obj_data *cont_row[MAX_BAG_ROWS];
  struct extra_descr_data *new_descr;
  int rentcode,timed,netcost,gold,account,nitems;

  if (!get_filename(GET_NAME(ch), fname, NEW_OBJ_FILES))
    return 1;
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING OBJECT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n", ch);
    }
    sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
 if (PLR_FLAGGED(ch, PLR_DEAD) || 
     PLR_FLAGGED(ch, PLR_DEADI) || 
     PLR_FLAGGED(ch, PLR_DEADII) || 
     PLR_FLAGGED(ch, PLR_DEADIII)) {
    send_to_char("\r\nYour death has caused you to corpse, look for your eq at your corpse.\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
     unequip_char(ch, i);
    }
    ch->carrying = NULL;
    IS_CARRYING_N(ch) = 0;
    IS_CARRYING_W(ch) = 0;
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEAD);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEADI);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEADII);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEADIII);
   }
    return 1;
  }
  if (!feof(fl))
    get_line(fl,line);

  sscanf(line,"%d %d %d %d %d %d",&rentcode, &timed,
        &netcost,&gold,&account,&nitems);

  switch (orig_rent_code = rentcode) {
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }

  for (j = 0;j < MAX_BAG_ROWS;j++)
    cont_row[j] = NULL; /* empty all cont lists (you never know ...) */

  if(!feof(fl))
    get_line(fl, line);
  while (!feof(fl)) {
        temp=NULL;
        /* first, we get the number. Not too hard. */
    if(*line == '#') {
      if (sscanf(line, "#%d", &nr) != 1) {
        continue;
      }
      /* we have the number, check it, load obj. */
      if (nr == NOTHING) {   /* then it is unique */
        temp = create_obj();
        temp->item_number=NOTHING;
      } else if (nr < 0) {
        continue;
      } else {
        if(nr >= 999999) 
          continue;
        temp=read_object(nr,VIRTUAL);
        if (!temp) {
          continue;
        }
      }

      get_line(fl,line);
      sscanf(line,"%d %d %d %d %d %d",t, t + 1, t+2, t + 3, t + 4,t+5);
      locate=t[0];
      GET_OBJ_VAL(temp,0) = t[1];
      GET_OBJ_VAL(temp,1) = t[2];
      GET_OBJ_VAL(temp,2) = t[3];
      GET_OBJ_VAL(temp,3) = t[4];
      GET_OBJ_EXTRA(temp) = t[5];

     get_line(fl,line);
       /* read line check for xap. */
      if(!strcasecmp("XAP",line)) {  /* then this is a Xap Obj, requires
                                       special care */
        if ((temp->name = fread_string(fl, buf2)) == NULL) {
          temp->name = "undefined";
        }

        if ((temp->short_description = fread_string(fl, buf2)) == NULL) {
          temp->short_description = "undefined";
        }

        if ((temp->description = fread_string(fl, buf2)) == NULL) {
          temp->description = "undefined";
        }

        if ((temp->action_description = fread_string(fl, buf2)) == NULL) {
          temp->action_description=0;
        }
        if (!get_line(fl, line) ||
           (sscanf(line, "%d %d %d %d %d %d", t,t+1,t+2,t+3,t+4,t+5) != 6)) {
          fprintf(stderr, "Format error in first numeric line (expecting _x_ args)");
          return 0;
        }
       temp->obj_flags.type_flag = t[0];
        temp->obj_flags.wear_flags = t[1];
        temp->obj_flags.weight = t[2];
        temp->obj_flags.cost = t[3];
        temp->obj_flags.cost_per_day = t[4];
	/* Delete */
        temp->obj_flags.level = t[5];

        /* buf2 is error codes pretty much */
        strcat(buf2, ", after numeric constants (expecting E/#xxx)");

        /* we're clearing these for good luck */

        for (j = 0; j < MAX_OBJ_AFFECT; j++) {
          temp->affected[j].location = APPLY_NONE;
          temp->affected[j].modifier = 0;
        }

        if (temp->ex_description) {
          temp->ex_description = NULL;
        }

        get_line(fl,line);
        for (k=j=zwei=0;!zwei && !feof(fl);) {
          switch (*line) {
            case 'E':
              CREATE(new_descr, struct extra_descr_data, 1);
             new_descr->keyword = fread_string(fl, buf2);
              new_descr->description = fread_string(fl, buf2);
             new_descr->next = temp->ex_description;
              temp->ex_description = new_descr;
              get_line(fl,line);
              break;
            case 'A':
              if (j >= MAX_OBJ_AFFECT) {
                log("SYSERR: Too many object affectations in loading rent file");
                danger=1;
              }
              get_line(fl, line);
              sscanf(line, "%d %d", t, t + 1);

              temp->affected[j].location = t[0];
              temp->affected[j].modifier = t[1];  
              j++;
              get_line(fl,line);
              break;

            case '$':
            case '#':
              zwei=1;
              break;
            default:
              zwei=1;
              break;
          }
        }      /* exit our for loop */
      }   /* exit our xap loop */
      if(temp != NULL) {
        num_objs++;
        auto_equip(ch, temp, locate);
      } else {
        continue;
      }
 /* 
  what to do with a new loaded item:

   if there's a list with <locate> less than 1 below this:
     (equipped items are assumed to have <locate>==0 here) then its
     container has disappeared from the file   *gasp*
      -> put all the list back to ch's inventory
   if there's a list of contents with <locate> 1 below this:
     check if it's a container
     - if so: get it from ch, fill it, and give it back to ch (this way the
         container has its correct weight before modifying ch)
     - if not: the container is missing -> put all the list to ch's inventory

for items with negative <locate>:
     if there's already a list of contents with the same <locate> put obj to it
     if not, start a new list

 Confused? Well maybe you can think of some better text to be put here ...

 since <locate> for contents is < 0 the list indices are switched to
 non-negative
 */

        if (locate > 0) { /* item equipped */
          for (j = MAX_BAG_ROWS-1;j > 0;j--)
            if (cont_row[j]) { /* no container -> back to ch's inventory */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_char(cont_row[j], ch);
              }
             cont_row[j] = NULL;
     }
          if (cont_row[0]) { /* content list existing */
            if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
              /* rem item ; fill ; equip again */
              temp = unequip_char(ch, locate-1);
              temp->contains = NULL; /* should be empty - but who knows */
              for (;cont_row[0];cont_row[0] = obj1) {
                obj1 = cont_row[0]->next_content;
               obj_to_obj(cont_row[0], temp);
         }
              equip_char(ch, temp, locate-1);
           } else { /* object isn't container -> empty content list */
              for (;cont_row[0];cont_row[0] = obj1) {
                obj1 = cont_row[0]->next_content;
        obj_to_char(cont_row[0], ch);
              }
              cont_row[0] = NULL;
            }
          }
     } else { /* locate <= 0 */
          for (j = MAX_BAG_ROWS-1;j > -locate;j--)
           if (cont_row[j]) { /* no container -> back to ch's inventory */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_char(cont_row[j], ch);
              } 
              cont_row[j] = NULL;
            }

         if (j == -locate && cont_row[j]) { /* content list existing */
            if (GET_OBJ_TYPE(temp) == ITEM_CONTAINER) {
              /* take item ; fill ; give to char again */
              obj_from_char(temp);
              temp->contains = NULL;
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_obj(cont_row[j], temp);
              }
              obj_to_char(temp, ch); /* add to inv first ... */
            } else { /* object isn't container -> empty content list */
              for (;cont_row[j];cont_row[j] = obj1) {
                obj1 = cont_row[j]->next_content;
                obj_to_char(cont_row[j], ch);
             }
              cont_row[j] = NULL;
            }
          }

          if (locate < 0 && locate >= -MAX_BAG_ROWS) {
               /* let obj be part of content list
                  but put it at the list's end thus having the items
                  in the same order as before renting */
            obj_from_char(temp);
            if ((obj1 = cont_row[-locate-1])) {
              while (obj1->next_content)
                obj1 = obj1->next_content;
              obj1->next_content = temp;
            } else
              cont_row[-locate-1] = temp;
          }
        } /* locate less than zero */
      }
    }

  /* Little hoarding check. -gg 3/1/98 */
  sprintf(fname, "%s (level %d) has %d objects (max %d).",
	GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);
  mudlog(fname, NRM, LVL_GOD, TRUE);

  sprintf(fname, "&R%s has connected to DeimosMUD.&n", GET_NAME(ch));
 if (!PRF_FLAGGED(ch, PRF_WHOINVIS))
  showplayer(fname, BRF, MAX(0, GET_INVIS_LEV(ch)), TRUE);
  
 if (PLR_FLAGGED(ch, PLR_DEAD) || 
     PLR_FLAGGED(ch, PLR_DEADI) || 
     PLR_FLAGGED(ch, PLR_DEADII) || 
     PLR_FLAGGED(ch, PLR_DEADIII)) {
    send_to_char("\r\nYour death has caused you to corpse, look for your eq at your corpse.\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
     unequip_char(ch, i);
    }
    ch->carrying = NULL;
    IS_CARRYING_N(ch) = 0;
    IS_CARRYING_W(ch) = 0;
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEAD);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEADI);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEADII);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DEADIII);
   }

  fclose(fl);

  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}
