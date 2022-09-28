# Grapevine support for CircleMUD/tbaMUD

## Requirements:
- C++ compiling support for your code
- Graeme-Hill Crossguid library: https://github.com/graeme-hill/crossguid
- JSON for Modern C++ library: https://github.com/nlohmann/json
- Machine Zone IXWebSocket library: https://github.com/machinezone/IXWebSocket (tested w/v5.1.3-11.4.3, build with cmake -DUSE_TLS=1 .)
- Register your game at https://grapevine.haus/register/new and grab the client ID and secret to setup in your code.

Modify GVGAME in grapevine.h to match the short name of your game as configured in Grapevine.
Modify these files to add Grapevine support. Add grapevine.h to the includes section at the top of each .c file.
* db.c:
At the end of the free_char function:
```
  if (ch->gvChannel)
    delete ch->gvChannel;
```   
At the end of the init_char function:
```
  ch->gvChannel = NULL;
```
* interpreter.c:
In your command_info cmd_info[] struct:
```
  { "gvplayer" , "gvp"   , POS_DEAD    , do_gvplayer , 0, 0, 0 },
  { "gvgame"   , "gvg"   , POS_DEAD    , do_gvgame   , 0, 0, 0 },
  { "gvtell"   , "gvt"   , POS_DEAD    , do_gvtell   , 0, 0, 0 },
  { "gvchannel", "gvc"   , POS_DEAD    , do_gvchannel, 0, 0, 0 },
  { "gvset"    , "gvs"   , POS_DEAD    , do_gvset    , 0, 0, 0 },
```
Adjust number of fields to suit your code as needed.

* comm.c:
After boot_db() in the init_game function:
```
  // start Grapevine connection
     std::string url = "wss://grapevine.haus/socket";
     std::string client_id = "<your Grapevine client id>";
     std::string client_secret = "<your Grapevline client secret>";

     GvChat = new ix::GvChat(url, client_id, client_secret, "1.0.0", "<your mud name>");
     GvChat->start();
```
In copyover_recovery() after "Warm Boot initiated" or "Copyover recovery initiated":
```
  // connect to Grapevine
	GvChat->start();
```
In game_loop() right at the top of the main loop:
```
// Process log queue
if (GvChat->getLogMessagesCount() > 0)
  GvChat->processLog();
```
In game_loop() just before "Entering Select Sleep, no sockets.":
```
      // tell GvChat we're sleeping
      GvChat->setSleeping(true);
```
In game loop() right after "Waking up to process connection.":
```
    // tell GvChat we're no longer sleeping
    GvChat->setSleeping(false);
```
In game_loop() right before "Process descriptors with input pending":
```
// Process Grapevine messages
      if (GvChat->getReceivedMessagesCount() > 0)
        GvChat->processMessages();
```
In heartbeat() right at the end before return;:
```
// update Grapevine games and players every 5 minutes
  if (!(pulse % (300 * PASSES_PER_SEC)))
  {
      GvChat->updateGamesStatus();
      GvChat->updatePlayersStatus();
      GvChat->updateGamesStatus(GVGAME);
      GvChat->updatePlayersStatus(GVGAME);
  }
 ```
* handler.c:
At the start of the extract_char function:
```
  if (!IS_NPC(ch))
      GvChat->playerSignOut(ch);
```
* interpreter.c:
At the end of enter_player_game before the return:
```
  GvChat->playerSignIn(d->character);
```
* utils.c:
At the end of the file:
```
// Function to remove all spaces from a given string 
std::string removeSpaces(std::string str)  
{ 
    str.erase(remove(str.begin(), str.end(), ' '), str.end()); 
    return str; 
}
```
* utils.h:
At the end of your function definitions:
```
std::string removeSpaces(std::string str); 
```
* structs.h:
At the top in the includes section:
```
#include <string>
#include <crossguid/guid.hpp>
```
At the end of struct char_data:
```
   std::string *gvChannel;   // current channel for this player
```
* act.h (or wherever you want to put your ACMDs, we use acmd.h)
```
//grapevine commands
ACMD(do_gvplayer);  ACMD(do_gvgame);      ACMD(do_gvtell);
ACMD(do_gvchannel); ACMD(do_gvset);
```
### Makefile changes
You'll have to add the following libraries to compile:
```
-lixwebsocket -lcrypto -lssl -lpthread -luuid -lcrossguid
```
### Changes needed for stock Circle/tba code bases:
Unlike stock CircleMUD/tbaMUD, World of Pain uses C++ vector classes for things like descriptor_list and character_list, so if you don't, you'll need to replace lines in grapevine.cpp such as the following:
```
  for (auto &i : descriptor_list) 
```
and
```
  for (auto &i : character_list)
```
with stock Circle/tba conventions like:
```
  for (auto i = descriptor_list; i; i = i->next)
```
and
```
  for (auto i = character_list; i; i = i->next)
```
## Player Command Help
### gvplayer
Shows the players online with Grapevine in a game or all games.
```
Usage: gvplayer <game>
<game> is the optional short name for a game in Grapevine. Specify no game to see all players in all games.
```
### gvgame
Shows all games currently online in Grapevine or gets detailed information on a specific game.
```
Usage: gvgame <game>
<game> is the optional short name for a game in Grapevine. Specify no game to see all games online.
```
### gvtell
Send a private tell to a player in another game on Grapevine.
```
Usage: gvtell <player@game> <message>
```
### gvchannel
Send a message to everyone on a particular channel on all games on Grapevine, or switch channels.
```
Usage: gvchannel <message>
OR gvchannel <channel to switch to>
Currently supported channels are gossip and testing. The default channel is gossip.
```
### gvset
Toggle player preferences for display of Grapevine status messages and channels.
```
Usage: gvset <games | players | channels>
```
  
