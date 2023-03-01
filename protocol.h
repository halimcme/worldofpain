/**************************************************************************
 *   File: protocol.h                                Part of World of Pain *
 *  Usage: Protocol negotiation and out of band data support               *
 *                                                                         *
 * Copyright (C) 2022 World of Pain                                        *
 * https://www.worldofpa.in                                                *
 *                                                                         *
 * Based upon protocol snippet by KaVir,                                   *
 * Released into the Public Domain in February 2011.                       *
***************************************************************************/
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "type.h"
#include <sys/types.h>
#include <unordered_set>
#include <zconf.h>

using namespace std;

/******************************************************************************
 Set your MUD_NAME, and change descriptor_t if necessary.
 ******************************************************************************/

#define MUD_NAME "The World of Pain"

typedef struct descriptor_data descriptor_t;

/******************************************************************************
 If your mud supports MCCP (compression), uncomment the next line.
 ******************************************************************************/

// buggy on copyover so disabled
//#define USING_MCCP true

extern void* z_alloc(void* opaque, uInt items, uInt size);
extern void z_free(void* opaque, void* address);

/******************************************************************************
 If your offer a Mudlet GUI for autoinstallation, put the path/filename here.
 ******************************************************************************/

#define MUDLET_PACKAGE "2.1.1\nhttps://www.worldofpa.in/WorldOfPain.mpackage"

/******************************************************************************
 Symbolic constants.
 ******************************************************************************/

#define SNIPPET_VERSION 6 /* Helpful for debugging */

#define MAX_PROTOCOL_BUFFER MAX_RAW_INPUT_LENGTH
#define MAX_VARIABLE_LENGTH 4096
#define MAX_OUTPUT_BUFFER LARGE_BUFSIZE
#define MAX_MSSP_BUFFER 4096

#define pSEND 1
#define pACCEPTED 2
#define pREJECTED 3

#define TELOPT_CHARSET 42
#define TELOPT_MSDP 69
#define TELOPT_MSSP 70
#define TELOPT_MCCP 86 /* This is MCCP version 2 */
#define TELOPT_MSP 90
#define TELOPT_MXP 91
#define TELOPT_ATCP 200
#define TELOPT_GMCP 201

#define OOB_VAR 1
#define OOB_VAL 2
#define OOB_TABLE_OPEN 3
#define OOB_TABLE_CLOSE 4
#define OOB_ARRAY_OPEN 5
#define OOB_ARRAY_CLOSE 6
#define MAX_OOB_SIZE 100

#define MSSP_VAR 1
#define MSSP_VAL 2

#define UNICODE_MALE 9794
#define UNICODE_FEMALE 9792
#define UNICODE_NEUTER 9791

/******************************************************************************
 Types.
 ******************************************************************************/

/*typedef enum
{
   FALSE,
   TRUE
} bool_t;*/

typedef enum { eUNKNOWN, eNO, eSOMETIMES, eYES } support_t;

typedef enum {
  eOOB_NONE = -1, /* This must always be first. */

  /* General */
  eOOB_CHARACTER_NAME,
  eOOB_CHAR_FULL_NAME,
  eOOB_SERVER_ID,
  eOOB_SERVER_TIME,
  eOOB_SNIPPET_VERSION,

  /* Character */
  eOOB_AFFECTS,
  eOOB_ALIGNMENT,
  eOOB_EXPERIENCE,
  eOOB_EXPERIENCE_MAX,
  eOOB_EXPERIENCE_TNL,
  eOOB_HEALTH,
  eOOB_HEALTH_MAX,
  eOOB_LEVEL,
  eOOB_RACE,
  eOOB_CLASS,
  eOOB_MANA,
  eOOB_MANA_MAX,
  eOOB_WIMPY,
  eOOB_PRACTICE,
  eOOB_MONEY,
  eOOB_BANK,
  eOOB_MOVEMENT,
  eOOB_MOVEMENT_MAX,
  eOOB_HITROLL,
  eOOB_DAMROLL,
  eOOB_AC,
  eOOB_STR,
  eOOB_INT,
  eOOB_WIS,
  eOOB_DEX,
  eOOB_CON,
  eOOB_CHA,
  eOOB_STR_PERM,
  eOOB_INT_PERM,
  eOOB_WIS_PERM,
  eOOB_DEX_PERM,
  eOOB_CON_PERM,
  eOOB_CHA_PERM,

  /* Combat */
  eOOB_OPPONENT_HEALTH,
  eOOB_OPPONENT_HEALTH_MAX,
  eOOB_OPPONENT_LEVEL,
  eOOB_OPPONENT_NAME,
  eOOB_COMBAT_STYLE,

  /* World */
  eOOB_AREA_NAME,
  eOOB_ROOM_EXITS,
  eOOB_ROOM_NAME,
  eOOB_ROOM_VNUM,
  eOOB_WORLD_TIME,

  /* Configuration */
  eOOB_CLIENT_ID,
  eOOB_CLIENT_VERSION,
  eOOB_PLUGIN_ID,
  eOOB_ANSI_COLORS,
  eOOB_XTERM_256_COLORS,
  eOOB_UTF_8,
  eOOB_SOUND,
  eOOB_MXP,

  eOOB_MAX /* This must always be last */
} variable_t;

// OOB data variables
typedef struct
{
  variable_t Variable;       /* The enum type of this variable */
  const char* pCategory;     // gmcp category
  const char* pFriendlyName; // player-facing variable names for GMCP
  const char* pKey;          // GMCP variable name
  const char* pName;         /* The string name of this variable */
  bool bString;              /* Is this variable a string or a number? */
  bool bConfigurable;        /* Can it be configured by the client? */
  bool bWriteOnce;           /* Can only set this variable once */
  bool bGUI;                 /* It's a special GUI configuration variable */
  int Min;                   /* The minimum valid value or string length */
  int Max;                   /* The maximum valid value or string length */
  int Default;               /* The default value for a number */
  const char* pDefault;      /* The default value for a string */
} variable_name_t;

typedef struct
{
  bool bReport;       /* Is this variable being reported? */
  bool bDirty;        /* Does this variable need to be sent again? */
  int ValueInt;       /* The numeric value of the variable */
  char* pValueString; /* The string value of the variable */
} OOB_t;

typedef struct
{
  const char* pName;              /* The name of the MSSP variable */
  const char* pValue;             /* The value of the MSSP variable */
  const char* (*pFunction)(void); /* Optional function to return the value */
} MSSP_t;

typedef struct
{
  int WriteOOB;          /* Used internally to indicate OOB data */
  bool bIACMode;         /* Current mode - deals with broken packets */
  bool bNegotiated;      /* Indicates client successfully negotiated */
  bool bBlockMXP;        /* Used internally based on MXP version */
  bool bTTYPE;           /* The client supports TTYPE */
  bool bNAWS;            /* The client supports NAWS */
  bool bCHARSET;         /* The client supports CHARSET */
  bool bMSDP;            /* The client supports MSDP */
  bool bMSP;             /* The client supports MSP */
  bool bMXP;             /* The client supports MXP */
  bool bGMCP;            // The client supports GMCP
  bool bMCCP;            /* The client supports MCCP */
  support_t b256Support; /* The client supports XTerm 256 colors */
  int ScreenWidth;       /* The client's screen width */
  int ScreenHeight;      /* The client's screen height */
  char* pMXPVersion;     /* The version of MXP supported */
  char* pLastTTYPE;      /* Used for the cyclic TTYPE check */
  OOB_t** pVariables;    /* The MSDP variables */
  unordered_set<string> GMCPSupports;
  bool destroyed;
} protocol_t;

/******************************************************************************
 Protocol functions.
 ******************************************************************************/

/* Function: ProtocolCreate
 *
 * Creates, initialises and returns a structure containing protocol data for a
 * single user.  This should be called when the descriptor is initialised.
 */
protocol_t* ProtocolCreate(void);

/* Function: ProtocolDestroy
 *
 * Frees the memory allocated by the specified structure.  This should be
 * called just before a descriptor is freed.
 */
void ProtocolDestroy(protocol_t* apProtocol);

/* Function: ProtocolNegotiate
 *
 * Negatiates with the client to see which protocols the user supports, and
 * stores the results in the user's protocol structure.  Call this when you
 * wish to perform negotiation (but only call it once).  It is usually called
 * either immediately after the user has connected, or just after they have
 * entered the game.
 */
void ProtocolNegotiate(dPtr apDescriptor);

/* Function: ProtocolInput
 *
 * Extracts any negotiation sequences from the input buffer, and passes back
 * whatever is left for the mud to parse normally.  Call this after data has
 * been read into the input buffer, before it is used for anything else.
 */

/* MUD Primary Colours */
extern const char* RGBone;
extern const char* RGBtwo;
extern const char* RGBthree;

ssize_t ProtocolInput(dPtr apDescriptor, char* apData, int aSize, char* apOut);

/* Function: ProtocolOutput
 *
 * This function takes a string, applies colour codes to it, and returns the
 * result.  It should be called just before writing to the output buffer.
 *
 * The special character used to indicate the start of a colour sequence is
 * '\t' (i.e., a tab, or ASCII character 9).  This makes it easy to include
 * in help files (as you can literally press the tab key) as well as strings
 * (where you can use \t instead).  However players can't send tabs (on most
 * muds at least), so this stops them from sending colour codes to each other.
 *
 * The predefined colours are:
 *
 *   n: no colour (switches colour off)
 *   r: dark red                        R: bright red
 *   g: dark green                      G: bright green
 *   b: dark blue                       B: bright blue
 *   y: dark yellow                     Y: bright yellow
 *   m: dark magenta                    M: bright magenta
 *   c: dark cyan                       C: bright cyan
 *   w: dark white                      W: bright white
 *   o: dark orange                     O: bright orange
 *
 * So for example "This is \tOorange\tn." will colour the word "orange".  You
 * can add more colours yourself just by updating the switch statement.
 *
 * It's also possible to explicitly specify an RGB value, by including the four
 * character colour sequence (as used by ColourRGB) within square brackets, eg:
 *
 *    This is a \t[F010]very dark green foreground\tn.
 *
 * The square brackets can also be used to send unicode characters, like this:
 *
 *    Boat: \t[U9973/B]
 *    Rook: \t[U9814/C]
 *
 * For example you might use 'B' to represent a boat on your ASCII map, or a 'C'
 * to represent a castle - but players with UTF-8 support would actually see the
 * appropriate unicode characters for a boat or a rook (the chess playing
 * piece).
 *
 * The exact syntax is '\t' (tab), '[', 'U' (indicating unicode), then the
 * decimal number of the unicode character (see http://www.unicode.org/charts),
 * then '/' followed by the ASCII character/s that should be used if the client
 * doesn't support UTF-8.  The ASCII sequence can be up to 7 characters in
 * length, but in most cases you'll only want it to be one or two characters (so
 * that it has the same alignment as the unicode character).
 *
 * Finally, this function also allows you to embed MXP tags.  The easiest and
 * safest way to do this is via the ( and ) bracket options:
 *
 *    From here, you can walk \t(north\t).
 *
 * However it's also possible to include more explicit MSP tags, like this:
 *
 *    The baker offers to sell you a \t<send href="buy pie">pie\t</send>.
 *
 * Note that the MXP tags will automatically be removed if the user doesn't
 * support MXP, but it's very important you remember to close the tags.
 */
const char* ProtocolOutput(dPtr apDescriptor, const char* apData, int* apLength);

/******************************************************************************
 Copyover save/load functions.
 ******************************************************************************/

/* Function: CopyoverGet
 *
 * Returns the protocol values stored as a short string.  If your mud uses
 * copyover, you should call this for each player and insert it after their
 * name in the temporary text file.
 */
const char* CopyoverGet(dPtr apDescriptor);

/* Function: CopyoverSet
 *
 * Call this function for each player after a copyover, passing in the string
 * you added to the temporary text file.  This will restore their protocol
 * settings, and automatically renegotiate MSDP/GMCP.
 *
 * Note that the client doesn't recognise a copyover, and therefore refuses to
 * renegotiate certain telnet options (to avoid loops), so they really need to
 * be saved.  However MSDP/GMCP is handled through scripts, and we don't want
 * to have to save all of the REPORT variables, so it's easier to renegotiate.
 *
 * Client name and version are not saved.  It is recommended you save these in
 * the player file, as then you can grep to collect client usage stats.
 */
void CopyoverSet(dPtr apDescriptor, const char* apData);

/******************************************************************************
 GMCP functions.
 ******************************************************************************/

/* Function: SendGMCPJ
 *
 * Call this to send a JSON GMCP response to the client
 */
void SendGMCPJ(dPtr apDescriptor, string apVariable, string apValue);

bool GMCPSupports(dPtr d, const char* module);

/******************************************************************************
 OOB functions.
 ******************************************************************************/

/* Function: OOBUpdate
 *
 * Call this regularly (I'd suggest at least once per second) to flush every
 * dirty MSDP variable that has been requested by the client via REPORT.  This
 * will automatically use GMCP instead of MSDP if supported by the client.
 */
void OOBUpdate(dPtr apDescriptor);

/* Function: OOBFlush
 *
 * Works like OOBUpdate(), except only flushes a specific variable.  The
 * variable will only actually be sent if it's both reported and dirty.
 *
 * Call this function after setting a variable if you want it to be reported
 * immediately, instead of on the next update.
 */
void OOBFlush(dPtr apDescriptor, variable_t aOOB);

/* Function: OOBSend
 *
 * Send the specified MSDP variable to the player.  You shouldn't ever really
 * need to do this manually, except perhaps when debugging something.  This
 * will automatically use GMCP instead of MSDP if supported by the client.
 */
void OOBSend(dPtr apDescriptor, variable_t aOOB);

/* Function: OOBSendPair
 *
 * Send the specified strings to the user as an OOB variable/value pair.  This
 * will automatically use GMCP instead of MSDP if supported by the client.
 */
void OOBSendPair(dPtr apDescriptor, const char* apVariable, const char* apValue);

/* Function: OOBSendList
 *
 * Works like OOBSendPair, but the value is sent as an OOB array.
 *
 * apValue should be a list of values separated by spaces.
 */
void OOBSendList(dPtr apDescriptor, const char* apVariable, const char* apValue);

/* Function: OOBSetNumber
 *
 * Call this whenever an OOB integer variable has changed.  The easiest
 * approach is to send every OOB variable within an update function (and
 * this is what the snippet does by default), but if the variable is only
 * set in one place you can just move its MDSPSend() call to there.
 *
 * You can also this function for bools, chars, enums, short ints, etc.
 */
void OOBSetNumber(dPtr apDescriptor, variable_t aOOB, int aValue);

/* Function: OOBSetString
 *
 * Call this whenever an OOB string variable has changed.  The easiest
 * approach is to send every OOB variable within an update function (and
 * this is what the snippet does by default), but if the variable is only
 * set in one place you can just move its MDSPSend() call to there.
 */
void OOBSetString(dPtr apDescriptor, variable_t aOOB, const char* apValue);

/* Function: OOBSetTable
 *
 * Works like OOBSetString, but the value is sent as an MSDP table.
 *
 * You must add the OOB_VAR and OOB_VAL manually, for example:
 *
 * sprintf( Buffer, "%c%s%c%s", (char)OOB_VAR, Name, (char)OOB_VAL, Value );
 * OOBSetTable( d, eOOB_TEST, Buffer );
 */
void OOBSetTable(dPtr apDescriptor, variable_t aOOB, const char* apValue);

/* Function: OOBSetArray
 *
 * Works like OOBSetString, but the value is sent as an MSDP array.
 *
 * You must add the OOB_VAR before each element manually, for example:
 *
 * sprintf( Buffer, "%c%s%c%s", (char)OOB_VAL, Val1, (char)OOB_VAL, Val2 );
 * OOBSetArray( d, eOOB_TEST, Buffer );
 */
void OOBSetArray(dPtr apDescriptor, variable_t aOOB, const char* apValue);

/******************************************************************************
 MSSP functions.
 ******************************************************************************/

/* Function: MSSPSetPlayers
 *
 * Stores the current number of players.  The first time it's called, it also
 * stores the uptime.
 */
void MSSPSetPlayers(int aPlayers);

/******************************************************************************
 MXP functions.
 ******************************************************************************/

/* Function: MXPCreateTag
 *
 * Puts the specified tag into a secure line, if MXP is supported.  If the user
 * doesn't support MXP they will see the string unchanged, meaning they will
 * see the <send> tags or whatever.  You should therefore check for support and
 * provide a different sequence for other users, or better yet just embed MXP
 * tags for the ProtocolOutput() function.
 */
const char* MXPCreateTag(dPtr apDescriptor, const char* apTag);

/* Function: MXPSendTag
 *
 * This works like MXPCreateTag, but instead of returning the string it sends
 * it directly to the user.  This is mainly useful for the <VERSION> tag.
 */
void MXPSendTag(dPtr apDescriptor, const char* apTag);

/******************************************************************************
 Sound functions.
 ******************************************************************************/

/* Function: SoundSend
 *
 * Sends the specified sound trigger to the player, using MSDP or GMCP if
 * supported, MSP if not.  The trigger string itself is a relative path and
 * filename, eg: SoundSend( pDesc, "monster/growl.wav" );
 */
void SoundSend(dPtr apDescriptor, const char* apTrigger);

/******************************************************************************
 Colour functions.
 ******************************************************************************/

/* Function: ColourRGB
 *
 * Returns a colour as an escape code, based on the RGB value provided.  The
 * string must be four characters, where the first is either 'f' for foreground
 * or 'b' for background (case insensitive), and the other three characters are
 * numeric digits in the range 0 to 5, representing red, green and blue.
 *
 * For example "F500" returns a red foreground, "B530" an orange background,
 * and so on.  An invalid colour will clear whatever you've set previously.
 *
 * If the user doesn't support XTerm 256 colours, this function will return the
 * best-fit ANSI colour instead.
 *
 * If you wish to embed colours in strings, use ProtocolOutput().
 */
const char* ColourRGB(dPtr apDescriptor, const char* apRGB);

/******************************************************************************
 Unicode (UTF-8 conversion) functions.
 ******************************************************************************/

/* Function: UnicodeGet
 *
 * Returns the UTF-8 sequence for the specified unicode value.
 */
char* UnicodeGet(int aValue);

/* Function: UnicodeAdd
 *
 * Adds the UTF-8 sequence for the specified unicode value onto the end of the
 * string, without adding a NUL character at the end.
 */
void UnicodeAdd(char** apString, int aValue);

#endif /* PROTOCOL_H */
