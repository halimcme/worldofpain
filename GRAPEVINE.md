# Grapevine support for CircleMUD/tbaMUD

## Requirements:
- C++ compiling support for your code
- Graeme-Hill Crossguid library: https://github.com/graeme-hill/crossguid
- Nlohmann JSON library: https://github.com/nlohmann/json
- Machine Zone IXWebSocket library: https://github.com/machinezone/IXWebSocket

Other files to modify to add Grapevine support:
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
  { "gvstatus" , "gvs"   , POS_DEAD    , do_gvstatus , 0, 0, 0 },
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

     GvChat = new ix::GvChat(url, client_id, client_secret, "1.0.0", "World of Pain");
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
At the end of struct char_data:
```
   xg::Guid gvGuid;         // guid for current Grapevine action
   std::string *gvMessage;   // send this text on GV success result
   std::string *gvChannel;   // current channel for this player
```
* act.h (or wherever you want to put your ACMDs, we use acmd.h)
```
//grapevine commands
ACMD(do_gvstatus);  ACMD(do_gvtell);    ACMD(do_gvchannel);
```
