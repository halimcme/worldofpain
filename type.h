/**************************************************************************
*   File: type.h                                    Part of World of Pain *
*  Usage: Definition of all types used by the game                        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1997 by Daniel Jacobs                                    *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
***************************************************************************/

#ifndef TYPE_H
#define TYPE_H

#include <list>
#include <memory>
#include <vector>

// typedefs
typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;

typedef int room_num;
typedef int obj_num;
typedef int mob_num;   /* A mob's vnum type */
typedef int shop_num;  // shop vnum type
typedef int zone_num;  // zone vnum type
typedef int quest_num; // quest vnum type


//  Pointer Types!
typedef struct char_data* chPtr;      // char_data
typedef struct obj_data* oPtr;        // obj_data
typedef struct room_data* rPtr;       // room_data
typedef struct descriptor_data* dPtr; // descriptor_data
typedef struct zone_data* zPtr;       // zone_data
typedef struct affected_type* affPtr; // affected_type
typedef struct index_data* idxPtr;    // index_data
typedef struct shop_data* sPtr;       // shop_data
typedef struct aq_data* qPtr;         // aq_data (autoquest)

//
//  Miscellaneous numbers for crunching
//
#define QUEST_ARRAY 4
#define MAX_OBJ_AFFECT 6 /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH 30
#define MAX_TONGUE 17
#define MAX_NAME_LENGTH 20
#define HOST_LENGTH 30
#define GARBAGE_SPACE 32
#define MAX_MESSAGES 60
#define MAX_TITLE_LENGTH 80
#define POOF_LENGTH 80
#define MAX_CAN_LEARN 80 /* For skills/spells           */
#define MAX_PROMPT_LENGTH 256
#define MAX_ALIAS_LENGTH 126 /* Crashes if longer!!!!!!!!!!!!!!!!!! */
#define EXDSCR_LENGTH 4096
#define MAX_INPUT_LENGTH 512
#define MAX_SKILLS 270
#define MAX_RAW_INPUT_LENGTH (12 * 1024)
#define SMALL_BUFSIZE 102
#define MAX_COMPLETED_QUESTS 1024
#define MAX_BKGROUND_LENGTH 3072
#define MAX_STRING_LENGTH 49152
#define MAX_SOCK_BUF (24 * 1024)
#define LARGE_BUFSIZE (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)


//
//  Cardinal directions, for use by game logic
//
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define UP 4
#define DOWN 5
#define NUM_OF_DIRS 6 // number of directions in a room (nsewud)


//
//  GOD LEVELS
//
#define LVL_IMPL 1
#define LVL_CODER 2
#define LVL_BUILDER 3
#define LVL_WRITER 4
#define LVL_AVATAR 5
#define LVL_J_CODER 6
#define LVL_J_BUILDER 7
#define LVL_J_WRITER 8
#define LVL_STAFF 9
#define LVL_IMMORT 10
#define LVL_GOD LVL_IMMORT
#define LVL_GRGOD LVL_BUILDER
#define LVL_FREEZE LVL_AVATAR
#define LVL_LOWEST_GOD LVL_IMMORT
#define LVL_NEWBIE 49 // max level of newbie
#define MAX_LEVELS 420000000


//
//  Rent codes
//
#define RENT_UNDEF 0
#define RENT_CRASH 1
#define RENT_RENTED 2
#define RENT_CRYO 3
#define RENT_FORCED 4
#define RENT_TIMEDOUT 5


//
//  Some different kind of liquids for use
//  in values of drink containers
//
#define LIQ_WATER 0
#define LIQ_BEER 1
#define LIQ_WINE 2
#define LIQ_ALE 3
#define LIQ_DARKALE 4
#define LIQ_WHISKY 5
#define LIQ_LEMONADE 6
#define LIQ_FIREBRT 7
#define LIQ_LOCALSPC 8
#define LIQ_SLIME 9
#define LIQ_MILK 10
#define LIQ_TEA 11
#define LIQ_COFFE 12
#define LIQ_BLOOD 13
#define LIQ_SALTWATER 14
#define LIQ_CLEARWATER 15
#define LIQ_NECTAR 16


//
//  Item types:
//    used by obj_data.obj_flags.type_flag
#define ITEM_LIGHT 1      /* Item is a light source	*/
#define ITEM_SCROLL 2     /* Item is a scroll		*/
#define ITEM_WAND 3       /* Item is a wand		*/
#define ITEM_STAFF 4      /* Item is a staff		*/
#define ITEM_WEAPON 5     /* Item is a weapon		*/
#define ITEM_FIREWEAPON 6 /* Unimplemented		*/
#define ITEM_MISSILE 7    /* Unimplemented		*/
#define ITEM_TREASURE 8   /* Item is a treasure, not gold	*/
#define ITEM_ARMOR 9      /* Item is armor		*/
#define ITEM_POTION 10    /* Item is a potion		*/
#define ITEM_WORN 11      /* Unimplemented		*/
#define ITEM_OTHER 12     /* Misc object			*/
#define ITEM_TRASH 13     /* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP 14      /* Unimplemented		*/
#define ITEM_CONTAINER 15 /* Item is a container		*/
#define ITEM_NOTE 16      /* Item is note 		*/
#define ITEM_DRINKCON 17  /* Item is a drink container	*/
#define ITEM_KEY 18       /* Item is a key		*/
#define ITEM_FOOD 19      /* Item is food			*/
#define ITEM_MONEY 20     /* Item is money (gold)		*/
#define ITEM_PEN 21       /* Item is a pen		*/
#define ITEM_BOAT 22      /* Item is a boat		*/
#define ITEM_FOUNTAIN 23  /* Item is a fountain		*/
#define ITEM_THROW 24
#define ITEM_GRENADE 25
#define ITEM_BOW 26
#define ITEM_SLING 27
#define ITEM_CROSSBOW 28
#define ITEM_BOLT 29
#define ITEM_ARROW 30
#define ITEM_ROCK 31
#define ITEM_TABLE 32
#define ITEM_PORTAL 33 // Item is a portal
#define ITEM_PIPE 34
#define ITEM_HERB 35
#define ITEM_SCUBA 36
#define ITEM_TEMP 37
#define ITEM_BOOK 38
#define ITEM_QUESTPOINT 39
#define ITEM_POUCH 40
#define ITEM_AQUIVER 41
#define ITEM_CQUIVER 42
#define ITEM_BLUEPRINT 43
#define ITEM_TOOL 44
#define ITEM_BUTCHERED 45
#define ITEM_WOOD 46
#define ITEM_CRYSTAL 47
#define ITEM_ORE 48
#define ITEM_METAL 49
#define ITEM_MINERAL 50

#define NUM_ITEM_TYPES 51 // number of ITEM_ types

// meta item types used to group item types into categories
const std::vector<int> ITEM_TYPE_WEAPON = {ITEM_WEAPON, ITEM_FIREWEAPON, ITEM_BOW, ITEM_CROSSBOW, ITEM_SLING};
const std::vector<int> ITEM_TYPE_AMMO = {ITEM_ARROW,   ITEM_BOLT,  ITEM_ROCK,   ITEM_AQUIVER,
                                         ITEM_CQUIVER, ITEM_POUCH, ITEM_MISSILE};
const std::vector<int> ITEM_TYPE_ARMOR = {ITEM_ARMOR};
const std::vector<int> ITEM_TYPE_MAGIC = {ITEM_POTION, ITEM_SCROLL, ITEM_STAFF, ITEM_WAND};

//
//  Character equipment positions:
//    used as index for char_data.equipment[]
//
//    NOTE: Don't confuse these constants with the ITEM_ bitvectors
//    which control the valid places you can wear a piece of equipment
//
#define WEAR_LIGHT 0
#define WEAR_OVERALL 1
#define WEAR_HEAD 2
#define WEAR_FACE 3
#define WEAR_EAR_R 4
#define WEAR_EAR_L 5
#define WEAR_EYES 6
#define WEAR_NOSE 7
#define WEAR_MOUTH 8
#define WEAR_NECK_1 9
#define WEAR_NECK_2 10
#define WEAR_BODY_1 11
#define WEAR_BODY_2 12
#define WEAR_ABOUT 13
#define WEAR_ARMS 14
#define WEAR_ELBOW_R 15
#define WEAR_ELBOW_L 16
#define WEAR_SHIELD_R 17
#define WEAR_SHIELD_L 18
#define WEAR_WRIST_R1 19
#define WEAR_WRIST_L1 20
#define WEAR_WRIST_R2 21
#define WEAR_WRIST_L2 22
#define WEAR_FINGER_R1 23
#define WEAR_FINGER_L1 24
#define WEAR_FINGER_R2 25
#define WEAR_FINGER_L2 26
#define WEAR_FINGER_R3 27
#define WEAR_FINGER_L3 28
#define WEAR_HANDS 29
#define WEAR_HOLD 30
#define WEAR_WIELD 31
#define WEAR_DUALWIELD 32
#define WEAR_WAIST 33
#define WEAR_BELOW_WAIST 34
#define WEAR_THIGH_R 35
#define WEAR_THIGH_L 36
#define WEAR_LEGS 37
#define WEAR_POCKET 38
#define WEAR_ANKLE_R 39
#define WEAR_ANKLE_L 40
#define WEAR_TOE_R1 41
#define WEAR_TOE_R2 42
#define WEAR_TOE_L1 43
#define WEAR_TOE_L2 44
#define WEAR_FEET 45
#define WEAR_HIDDEN_1 46
#define WEAR_HIDDEN_2 47
#define WEAR_QUIVER 48
#define WEAR_SHEATH 49
#define WEAR_SHEATH2 50
#define NUM_WEARS 51 /* This must be the # of eq positions!! */


//
//  Modifier constants used with obj affects ('A' fields)
//
#define APPLY_NONE 0           // No effect
#define APPLY_STR 1            // Apply to strength
#define APPLY_DEX 2            // Apply to dexterity
#define APPLY_INT 3            // Apply to constitution
#define APPLY_WIS 4            // Apply to wisdom
#define APPLY_CON 5            // Apply to constitution
#define APPLY_CHA 6            // Apply to charisma
#define APPLY_CLASS 7          // Reserved
#define APPLY_LEVEL 8          // Reserved
#define APPLY_AGE 9            // Apply to age
#define APPLY_CHAR_WEIGHT 10   // Apply to weight
#define APPLY_CHAR_HEIGHT 11   // Apply to height
#define APPLY_MANA 12          // Apply to max mana
#define APPLY_HIT 13           // Apply to max hit points
#define APPLY_MOVE 14          // Apply to max move points
#define APPLY_GOLD 15          // Reserved
#define APPLY_EXP 16           // Reserved
#define APPLY_AC 17            // Apply to Armor Class
#define APPLY_HITROLL 18       // Apply to hitroll
#define APPLY_DAMROLL 19       // Apply to damage roll
#define APPLY_SAVING_PARA 20   // Apply to save throw: paralz
#define APPLY_SAVING_ROD 21    // Apply to save throw: rods
#define APPLY_SAVING_PETRI 22  // Apply to save throw: petrif
#define APPLY_SAVING_BREATH 23 // Apply to save throw: breath
#define APPLY_SAVING_SPELL 24  // Apply to save throw: spells
#define APPLY_RACE 25          // Apply to race variable
#define APPLY_REGEN 26         // Apply to regeneration factor
#define MAX_APPLY 27           // Used by oedit to display applys


//
// Herb types for use with pipe object
//
#define HERB_HEALING 1
#define HERB_POT 2
#define HERB_POISON 3
#define HERB_STR 4
#define HERB_INTEL 5
#define HERB_WIS 6
#define HERB_CON 7
#define HERB_DEX 8
#define HERB_CHA 9
#define HERB_BLINDNESS 10
#define HERB_GOOD 11
#define HERB_EVIL 12
#define HERB_FLY 13


//
//  Player conditions
//
#define DRUNK 0     // Player drunk
#define FULL 1      // Player hunger
#define THIRST 2    // Player thirsty
#define STONED 3    // Player stoned (not fully imped)
#define STRESSED 4  // Player stressed (not imped)
#define DEPRESSED 5 // Player depressed (not imped)
#define ANGRY 6     // Player angry (not imped)
#define FATIGUED 7  // Player fatigued (not imped)
#define MENTAL 8    // Player mental state (not imped)
#define MAX_COND 8  // Last condition

//
//  Sex
//
#define SEX_NEUTRAL 0
#define SEX_MALE 1
#define SEX_FEMALE 2


//
//  PC classes
//
#define CLASS_UNDEFINED -1
#define CLASS_MAGIC_USER 0
#define CLASS_CLERIC 1
#define CLASS_THIEF 2
#define CLASS_WARRIOR 3
#define CLASS_ALCHEMIST 4
#define CLASS_MATRICIST 5
#define CLASS_SENTINEL 6
#define CLASS_SPARE3 7
#define CLASS_SPARE4 8
#define CLASS_SPARE5 9
#define CLASS_PSIONIST 10
#define CLASS_SHAMAN 11
#define CLASS_NOMAD 12
#define CLASS_FIRESWORD 13
#define CLASS_DIABOLIST 14
#define CLASS_DEVIANT 15
#define CLASS_SHRIKE 16
#define CLASS_WANDERER 17
#define CLASS_MAGE_SPARE4 18
#define CLASS_MAGE_SPARE5 19
#define CLASS_EXORCIST 20
#define CLASS_PRIEST 21
#define CLASS_DRUID 22
#define CLASS_SCOUT 23
#define CLASS_BISHOP 24
#define CLASS_PROTECTOR 25
#define CLASS_ACCOLYTE 26
#define CLASS_BARD 27
#define CLASS_CLER_SPARE4 28
#define CLASS_CLER_SPARE5 29
#define CLASS_GYPSY 30
#define CLASS_CULTIST 31
#define CLASS_SILENCER 32
#define CLASS_PIRATE 33
#define CLASS_TERRORIST 34
#define CLASS_HITMAN 35
#define CLASS_SHARPER 36
#define CLASS_SHADOW 37
#define CLASS_THIEF_SPARE4 38
#define CLASS_THIEF_SPARE5 39
#define CLASS_DARKSABRE 40
#define CLASS_PALADIN 41
#define CLASS_GUERILLA 42
#define CLASS_BARBARIAN 43
#define CLASS_SLAYER 44
#define CLASS_KICKBOXER 45
#define CLASS_HUNTER 46
#define CLASS_WOODSMAN 47
#define CLASS_WARR_SPARE4 48
#define CLASS_WARR_SPARE5 49
#define CLASS_CONJURER 50
#define CLASS_HEALER 51
#define CLASS_ARSONIST 52
#define CLASS_BERSERKER 53
#define CLASS_MYSTIC 54
#define CLASS_RONIN 55
#define CLASS_LURKER 56
#define CLASS_GEOMANCER 57
#define CLASS_ALCH_SPARE4 58
#define CLASS_ALCH_SPARE5 59
#define CLASS_WARLOCK 60
#define CLASS_GUARDIAN 61
#define CLASS_NINJA 62
#define CLASS_MERCENARY 63
#define CLASS_SAMURAI 64
#define CLASS_SAVIOR 65
#define CLASS_ROGUE 66
#define CLASS_INITIATE 67
#define CLASS_SPARE1_SPR4 68
#define CLASS_SPARE1_SPR5 69
#define CLASS_WIZARD 70
#define CLASS_PROPHET 71
#define CLASS_STALKER 72
#define CLASS_DRAGOON 73
#define CLASS_ZEALOT 74
#define CLASS_CONVERT 75
#define CLASS_AGENT 76
#define CLASS_DETECTIVE 77
#define CLASS_SPARE2_SPR4 78
#define CLASS_SPARE2_SPR5 79
#define CLASS_SAGE 80
#define CLASS_ARCHDRUID 81
#define CLASS_PHANTOM 82
#define CLASS_TEMPLAR 83
#define CLASS_CHEMIST 84
#define CLASS_ORACLE 85
#define CLASS_OFFICER 86
#define CLASS_ELEMENT 87
#define CLASS_SPARE3_SPR4 88
#define CLASS_SPARE3_SPR5 89
#define CLASS_SPARE4_MAGE 90
#define CLASS_SPARE4_CLER 91
#define CLASS_SPARE4_THIEF 92
#define CLASS_SPARE4_WARR 93
#define CLASS_SPARE4_ALCH 94
#define CLASS_SPARE4_SPR1 95
#define CLASS_SPARE4_SPR2 96
#define CLASS_SPARE4_SPR3 97
#define CLASS_SPARE4_SPR4 98
#define CLASS_SPARE4_SPR5 99
#define CLASS_SPARE5_MAGE 100
#define CLASS_SPARE5_CLER 101
#define CLASS_SPARE5_THIEF 102
#define CLASS_SPARE5_WARR 103
#define CLASS_SPARE5_ALCH 104
#define CLASS_SPARE5_SPR1 105
#define CLASS_SPARE5_SPR2 106
#define CLASS_SPARE5_SPR3 107
#define CLASS_SPARE5_SPR4 108
#define CLASS_SPARE5_SPR5 109
#define NUM_CLASSES 110 // This must be the number of classes!!


//
//  NPC classes
//    (currently unused - feel free to implement!)
//
#define CLASS_OTHER 0
#define CLASS_UNDEAD 1
#define CLASS_HUMANOID 2
#define CLASS_ANIMAL 3
#define CLASS_DRAGON 4
#define CLASS_GIANT 5
#define CLASS_VAMPIRE 6
#define CLASS_WITCH 7
#define CLASS_DEMON_MOB 8


//
//  Race types
//
#define RACE_UNDEFINED -1
#define RACE_HUMAN 0
#define RACE_ELF 1
#define RACE_HALFELF 2
#define RACE_DWARF 3
#define RACE_GOBLIN 4
#define RACE_HOBGOBLIN 5
#define RACE_KOBOLD 6
#define RACE_ORC 7
#define RACE_OGRE 8
#define RACE_TROLL 9
#define RACE_TROGLODYLE 10
#define RACE_CHANGELING 11
#define RACE_GNOME 12
#define RACE_WOLFEN 13
#define RACE_FAIRY 14
#define RACE_HALFLING 15
#define RACE_WRAITHEN 16
#define RACE_DEMON 17
#define RACE_ANGEL 18
#define RACE_SPHINXIAN 19
#define RACE_GHOST 20
#define RACE_GHOUL 21
#define RACE_VAMPIRE 22
#define RACE_NESSIAN 23
#define RACE_UNDEF5 24
#define RACE_UNDEF6 25
#define RACE_UNDEF7 26
#define RACE_UNDEF8 27
#define RACE_UNDEF9 28
#define RACE_UNDEF10 29
#define RACE_UNDEF11 30
#define RACE_UNDEF12 31
//
//  end pc races
//
//  begin mob races
//
#define RACE_MINOTAUR 32
#define RACE_HYDRA 33
#define RACE_GOD 34
#define RACE_TITAN 35
#define RACE_ETTIN 36
#define RACE_ALLIGATOR 37
#define RACE_CROCODILE 38
#define RACE_SEA_MONSTER 39
#define RACE_HORSE 40
#define RACE_PEGASUS 41
#define RACE_DROW 42
#define RACE_ANIMAL 43
#define RACE_ELEMENTAL 44
#define RACE_ALIEN 45
#define RACE_INSECT 46
#define RACE_DRAGON 47
#define RACE_DINOSAUR 48
#define RACE_ROBOT 49
#define RACE_FISH 50
#define RACE_GIANT 51
#define RACE_OTHER 52
#define RACE_PLANT 53
#define RACE_UNDEAD 54
#define RACE_AMPHIBIAN 55
#define RACE_BAT 56
#define RACE_BEAR 57
#define RACE_BIRD 58
#define RACE_CANINE 59
#define RACE_CRAB 60
#define RACE_DOLPHIN 61
#define RACE_GOAT 62
#define RACE_FELINE 63
#define RACE_HIPPO 64
#define RACE_MARSUPIAL 65
#define RACE_PRIMATE 66
#define RACE_RABBIT 67
#define RACE_REPTILE 68
#define RACE_RHINO 69
#define RACE_RODENT 70
#define RACE_SEAL 71
#define RACE_SHEEP 72
#define RACE_SPIDER 73
#define RACE_TURTLE 74
#define RACE_WHALE 75
#define NUM_RACES 76
#define NUM_PC_RACES 32


//
//  Zone Race Alignments
//
#define ZRA_NO_HATE 0    // can't live here, we hate you
#define ZRA_NO_NEUTRAL 1 // can't live here, neutral
#define ZRA_YES_FRIEND 2 // can live here, neutral or friend
#define ZRA_YES_OWNER 3  // can live here, we own the town


//
//  Modes of connectedness:
//    used by descriptor_data.state
//
#define CON_PLAYING 0       // Playing - Nominal state
#define CON_CLOSE 1         // Disconnecting
#define CON_GET_NAME 2      // By what name ..?
#define CON_NAME_CNFRM 3    // Did I get that right, x?
#define CON_PASSWORD 4      // Password:
#define CON_NEWPASSWD 5     // Give me a password for x
#define CON_CNFPASSWD 6     // Please retype password:
#define CON_QSEX 7          // Sex?
#define CON_QCLASS 8        // Class?
#define CON_RMOTD 9         // PRESS RETURN after MOTD
#define CON_MENU 10         // Your choice: (main menu)
#define CON_EXDESC 11       // Enter a new description:
#define CON_CHPWD_GETOLD 12 // Changing passwd: get old
#define CON_CHPWD_GETNEW 13 // Changing passwd: get new
#define CON_CHPWD_VRFY 14   // Verify new password
#define CON_DELCNF1 15      // Delete confirmation 1
#define CON_DELCNF2 16      // Delete confirmation 2
#define CON_OEDIT 17        // Object editor
#define CON_REDIT 18        // Room editor
#define CON_ZEDIT 19        // Zone editor
#define CON_MEDIT 20        // Mobile editor
#define CON_SEDIT 21        // Shop editor
#define CON_CRAFTEDIT 22    // Craft editor
#define CON_AEDIT 23        // Social editor
#define CON_QEDIT 24        // Quest editor
#define CON_QRACE 25        // Entering Race
#define CON_GET_EMAIL 26    // Entering eMail
#define CON_ACCEPTRACE 27   // Accepting RP of race
#define CON_FINISHRACE 28   // Finishing information for race
#define CON_OTP 29          // One Time Password MFA prompt
#define CON_GET_PROTOCOL 30 // Used at log-in while attempting to get protocols
#define CON_VI_MODE 31      // Choose visually impaired mode

/* OLC States range - used by IS_IN_OLC and IS_PLAYING */
#define FIRST_OLC_STATE CON_OEDIT /**< The first CON_ state that is an OLC */
#define LAST_OLC_STATE CON_QEDIT  /**< The last CON_ state that is an OLC  */

//
//  Positions
//
#define POS_DEAD 0
#define POS_MORTALLYW 1
#define POS_INCAP 2
#define POS_STUNNED 3
#define POS_SLEEPING 4
#define POS_RESTING 5
#define POS_SITTING 6
#define POS_FIGHTING 7
#define POS_STANDING 8


//
//  Sector types:
//    used in room_data.sector_type
//
#define SECT_INSIDE 0       // Indoors
#define SECT_CITY 1         // In a city
#define SECT_FIELD 2        // In a field
#define SECT_FOREST 3       // In a forest
#define SECT_HILLS 4        // In the hills
#define SECT_MOUNTAIN 5     // On a mountain
#define SECT_WATER_SWIM 6   // Swimmable water
#define SECT_WATER_NOSWIM 7 // Water - need a boat
#define SECT_UNDERWATER 8   // Underwater
#define SECT_FLYING 9       // Wheee!
#define SECT_CAVE 10        // ! No magic
#define SECT_DESERT 11      // ! Thirst faster
#define SECT_ARCTIC 12      // ! Lose mana
#define SECT_SPACE 13       // ! Need oxygen suit
#define SECT_QUICKSAND 14   // ! Need to move fast
#define SECT_SWAMP 15       // Top / underwater zone


//
//  Area's PK type
//
#define PK_NONE 0   // Default for no set type, will be lawful
#define PK_LAWFUL 1 // Lawful area, pk results in KILLER flag
#define PK_TYPE1 2  // Type 1 pk, level ranges, loser returned
#define PK_TYPE2 3  // Type 2 pk, level ranges, loser dies
#define PK_TYPE3 4  // Type 3 pk, no level ranges, loser dies
#define NUM_PK_TYPES 5


//
//  Sky conditions for weather_data
//
#define SKY_CLOUDLESS 0
#define SKY_CLOUDY 1
#define SKY_RAINING 2
#define SKY_LIGHTNING 3


//
//  Sun state for weather_data
//
#define SUN_DARK 0
#define SUN_RISE 1
#define SUN_LIGHT 2
#define SUN_SET 3
#define SUN_ECLIPSE 4

typedef enum { eNULL, eCHAR, eOBJ, eDESC, eROOM, ePROTOCOLS, eCAST, eCRAFT, eHARVEST } event_t;

// types of sounds used in const char* media_types[]
#define SOUND_ACTION 0
#define SOUND_COMBAT 1
#define SOUND_ENVIRONMENT 2
#define SOUND_MUSIC 3
#define SOUND_WEATHER 4

#endif // TYPE_H
// End - type.h
