# Grapevine support for CircleMUD/tbaMUD

## Requirements:
- C++ compiling support for your code
- Graeme-Hill Crossguid library: https://github.com/graeme-hill/crossguid
- JSON for Modern C++ library: https://github.com/nlohmann/json
- Machine Zone IXWebSocket library: https://github.com/machinezone/IXWebSocket
- Register your game at https://grapevine.haus/register/new and grab the client ID and secret to setup in your code.

Modify these files to add Grapevine support. Add grapevine.h to the includes section at the top of each .c file.
* db.c:
After the end of the free_char function:
```
  if (ch->gvChannel)
    delete ch->gvChannel;

  if (ch->gvMessage)
    delete ch->gvMessage;
```   
After the end of the init_char function:
```
  ch->gvChannel = NULL;
  ch->gvMessage = NULL;
```
* interpreter.c:
In your command_info cmd_info[] struct:
```
  { "gvplayer" , "gvp"   , POS_DEAD    , do_gvplayer , 0, 0, 0 },
  { "gvgame"   , "gvg"   , POS_DEAD    , do_gvgame   , 0, 0, 0 },
  { "gvtell"   , "gvt"   , POS_DEAD    , do_gvtell   , 0, 0, 0 },
  { "gvchannel", "gvc"   , POS_DEAD    , do_gvchannel, 0, 0, 0 },
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
After game_loop() in the init_game function:
```
  GvChat->stop();
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
* structs.h:
At the top in the includes section:
```
#include <string>
#include <crossguid/guid.hpp>
```
At the end of struct char_data:
```
   xg::Guid gvGuid;         // guid for current Grapevine action
   std::string *gvMessage;   // send this text on GV success result
   std::string *gvChannel;   // current channel for this player
   bool gvGameAll;          // show all games or just one?
   bool gvFirstGame;        // is this the first game if gvGameAll == true?
```
* act.h (or wherever you want to put your ACMDs, we use acmd.h)
```
//grapevine commands
ACMD(do_gvplayer);  ACMD(do_gvgame);      ACMD(do_gvtell);
ACMD(do_gvchannel);
```
### Makefile changes
You'll have to add the following libraries to compile:
```
-lstdc++ -lm -lixwebsocket -lcrypto -lssl -lpthread -luuid -lcrossguid
```
### Changes needed for stock Circle/tba code bases:
World of Pain uses C++ vector classes for things like descriptor_list and character_list, so if you don't, you'll need to replace lines in grapevine.cpp such as the following:
```
  for (auto &i : descriptor_list) 
```
and
```
  for (auto &i : character_list)
```
with stock Circle/tba conventions like:
```
  for (i = descriptor_list; i; i = i->next)
```
and
```
  for (i = character_list; i; i = i->next)
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
OR gvchannel <channel>
Currently supported channels are gossip and testing. The default channel is gossip.
```
  
