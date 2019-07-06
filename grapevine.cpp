/**************************************************************************
*   File: grapevine.cpp                             Part of World of Pain *
*  Usage: Main class and support commands for grapevine chat              *                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 2019 World of Pain                                       *
*                                                                         *
*  Based on ws_chat.cpp by: Benjamin Sergeant                             *
*  Copyright (c) 2017-2019 Machine Zone, Inc. All rights reserved.        *
***************************************************************************/
#define __GRAPEVINE_CPP__

#include <crossguid/guid.hpp>
#include "grapevine.h"
#include "acmd.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "spells.h"
#include "wizsup.h"
#include "json.hpp"

using json = nlohmann::json;

// extern functions
int can_hear_check(chPtr ch);

// extern variables
extern list <dPtr> descriptor_list;

// local globals
ix::GvChat *GvChat;

namespace ix
{
// Grapevine class constructor to initiate connection
    GvChat::GvChat(const std::string& url,
                                 const std::string& client_id,
                                 const std::string& client_secret,
                                 const std::string& version,
                                 const std::string& user_agent) :
        _url(url),
        _client_id(client_id),
        _client_secret(client_secret),
        _version(version),
        _user_agent(user_agent)

    {
        ;
    }

// log Grapevine to mud system log
    void GvChat::log(const std::string& msg)
    {
        do_log("GV CHAT: %s", msg.c_str());
    }

// count of recieved messages
    size_t GvChat::getReceivedMessagesCount() const
    {
        return _receivedQueue.size();
    }

// is Grapevine connected?
    bool GvChat::isReady() const
    {
        return _webSocket.getReadyState() == ix::ReadyState::Open;
    }

// close connection to Grapevine
    void GvChat::stop()
    {
        _webSocket.stop();
    }

// connect to Grapevine
    void GvChat::start()
    {
        _webSocket.setUrl(_url);

        std::stringstream ss;
        log(std::string("Connecting to ") + _url);

        _webSocket.setOnMessageCallback(
            [this](const WebSocketMessagePtr& msg)
            {
                std::stringstream ss;
                if (msg->type == ix::WebSocketMessageType::Open)
                { // if we're connected, authenticate
                    log("Connected to " + _url + ". Authenticating...");
                    authenticate();
                }
                else if (msg->type == ix::WebSocketMessageType::Close)
                { // if we're disconnected, report why
                    ss << _url
                       << " disconnected!"
                       << " code " << msg->closeInfo.code
                       << " reason " << msg->closeInfo.reason;
                       log(ss.str());
                       authenticated = false;
                }
                else if (msg->type == ix::WebSocketMessageType::Message)
                { // decode incoming message
                    decodeMessage(msg->str);
                }
                else if (msg->type == ix::WebSocketMessageType::Error)
                { // report errors
                    ss << "Connection error: " << msg->errorInfo.reason      << std::endl;
                    ss << "#retries: "         << msg->errorInfo.retries     << std::endl;
                    ss << "Wait time(ms): "    << msg->errorInfo.wait_time   << std::endl;
                    ss << "HTTP Status: "      << msg->errorInfo.http_status << std::endl;
                    log(ss.str());
                    authenticated = false;
                }
                else
                { // unknown websocket response
                    ss << "Invalid ix::WebSocketMessageType";
                    log(ss.str());
                }
            });

        _webSocket.start();
    }

// authenticate a new Grapevine connection
    void GvChat::authenticate()
    {
        json j;
        j["event"] = "authenticate";
        j["payload"]["client_id"] = _client_id;
        j["payload"]["client_secret"] = _client_secret;
        j["payload"]["supports"] = { "channels", "players", "tells" };
        j["payload"]["channels"] = { "gossip", "testing" };
        j["payload"]["version"] = _version;
        j["payload"]["user_agent"] = _user_agent;
        authenticated = false;
        sendMessage(j);
    }

// decode an incoming message
    void GvChat::decodeMessage(const std::string& str)
    {
        auto j = json::parse(str);
        std::string event = j["event"];

        if (event == "authenticate")
            eventAuthenticate(j);
        else if (j["status"] == "failure")
            eventFailure(j);
        else if (event == "heartbeat")
            eventHeartbeat(j);
        else if (event == "restart")
            eventRestart(j);
        else if (event == "channels/broadcast")
            eventBroadcast(j);
        else if (event == "players/sign-in")
            eventSignIn(j);
        else if (event == "players/sign-out")
            eventSignOut(j);
        else if (event == "players/status")
            eventPlayersStatus(j);
        else if (event == "tells/send" || event == "channels/send")
            eventSuccess(j);
        else if (event == "tells/receive")
            eventTellsReceive(j);
        else
        {
            std::stringstream ss;
            ss << "unhandled event: " << event << endl << j.dump();
            log(ss.str());
        }
        return;
    }

// respond to authentication event
    void GvChat::eventAuthenticate(json j)
    {
        std::stringstream ss;

        ss << "Authentication"
                << " status: " << j["status"]
                << " unicode: " << j["payload"]["unicode"]
                << " version: " << j["payload"]["version"];
        log(ss.str());
        if (j["status"] == "success")
            authenticated = true;
        else
            authenticated = false;
    }

// respond to heartbeat event
    void GvChat::eventHeartbeat(json j)
    {
        chPtr tch;
        vector <string> players;
        json k;

        k["event"] = "heartbeat";

        // build list of players for heartbeat
        for (auto &d : descriptor_list) 
        {
            if (d->original)
                tch = d->original;
            else if (!(tch = d->character))
                continue;
            if (IS_GOD(tch) && GET_INVIS_LEV(tch))
                continue; // don't show invisible gods

            players.push_back(GET_NAME(tch));
        }
        k["payload"]["players"] = players;
        sendMessage(k);
    }

// event saying the GV server is restarting
    void GvChat::eventRestart(json j)
    {
        std::stringstream ss;
        ss << "Server restarting... downtime: "
            << j["payload"]["downtime"];
        log(ss.str());
    }

    void GvChat::eventSubscribe(json j)
    {

    }

    void GvChat::eventUnsubscribe(json j)
    {

    }

// broadcast channel messages sent to all players that subscribe
    void GvChat::eventBroadcast(json j)
    {
        std::string channel = j["payload"]["channel"];
        std::string message = j["payload"]["message"];
        std::string name = j["payload"]["name"];
        std::string game = j["payload"]["game"];

        for (auto &i : descriptor_list) 
        {
            if (i->character && can_hear_check(i->character))
            {
                send_to_char(i->character, \
                    "GV: [%s] %s@%s: %s.\r\n", \
                    channel.c_str(), name.c_str(), \
                    game.c_str(), message.c_str());
            }
        }
    }

// sign in events can be either for our players or remote games
    void GvChat::eventSignIn(json j)
    {
        // no payload means this is a response to one of our players signing in
        if (!j["payload"].is_object())
        {  // notify a character with given reference of sign in
            std::string refStr = j["ref"];
            xg::Guid ref(refStr);
            chPtr ch = findCharByRef(ref);
            if (!ch)
                return;
            send_to_char(ch, "GV: You have been signed in to Grapevine Chat.");
            return;
        }
        else 
        { // we have a payload, a player signed in from another game
            std::string name = j["payload"]["name"];
            std::string game = j["payload"]["game"];
            for (auto &i : descriptor_list) 
            {
                if (0 && i->character && can_hear_check(i->character))
                {
                    send_to_char(i->character, \
                        "GV: %s has signed in to %s.\r\n", \
                        name.c_str(), game.c_str());
                }
            }
            return;
        }
    }

// notify if a GV player has signed out
    void GvChat::eventSignOut(json j)
    {
        std::string name = j["payload"]["name"];
        std::string game = j["payload"]["game"];
        for (auto &i : descriptor_list) 
        {
            if (0 && i->character && can_hear_check(i->character))
            {
                send_to_char(i->character, \
                    "GV: %s has signed out of %s.\r\n", \
                    name.c_str(), game.c_str());
            }
        }
        return;
    }

// notify about GV player status
    void GvChat::eventPlayersStatus(json j)
    {
        std::string game = j["payload"]["game"];
        std::string nobodyOn;

        if (j["ref"].is_string())
        {
            std::string refStr = j["ref"];
            xg::Guid ref(refStr);
            chPtr ch = findCharByRef(ref);
            if (!ch)
                return;
            std::string players;
            int count = 0; // count of online chars in a game
            if (j["payload"]["players"].is_array())
            {
                std::string playerLine;
                int sw = GET_SCREEN_WIDTH(ch);
                if (sw > 1000)
                    sw = 80;
                for (auto &p : j["payload"]["players"])
                {
                    std::string player = p;
                    if (playerLine.length() + player.length() + 1 > sw)
                    {       
                        players += "\r\n";
                        playerLine = player + " ";
                        players += playerLine;
                    }
                    else
                    {
                        playerLine += player + " ";
                        players += player + " ";
                    }
                    count++;
                }
                if (count)
                {
                    if (count == 1)
                        send_to_char(ch, "1 character online in %s: %s\r\n", \
                            game.c_str(), players.c_str());
                    else
                    {
                        send_to_char(ch, "%d characters online in %s:\r\n", \
                            count, game.c_str());
                        send_to_char(ch, "%s\r\n", players.c_str());
                    }
                }
                else 
                {
                    send_to_char(ch, "No characters online on %s.\r\n", game.c_str());
                }
            }
        }
    }

// notify player when a GV event fails
    void GvChat::eventFailure(json j)
    {
        std::string refStr = j["ref"];
        xg::Guid ref(refStr);
        chPtr ch = findCharByRef(ref);
        if (!ch)
            return;
        std::string event = j["event"];
        std::string error = j["error"];
        send_to_char(ch, "GV: %s Failure: %s\r\n", \
            event.c_str(), error.c_str());
    }

// notify player when a GV event is successful
    void GvChat::eventSuccess(json j)
    {
        std::string refStr = j["ref"];
        xg::Guid ref(refStr);
        chPtr ch = findCharByRef(ref);
        if (!ch || ch->gvMessage->empty())
            return;
        send_to_char(ch->gvMessage->c_str(), ch);
    }

// send GV tell to player
    void GvChat::eventTellsReceive(json j)
    {
        std::string from_game = j["payload"]["from_game"];
        std::string from_name = j["payload"]["from_name"];
        std::string to_name = j["payload"]["to_name"];
        std::string message = j["payload"]["message"];

        for (auto &i : descriptor_list) 
        { // loop through all online players
            if (i->character && can_hear_check(i->character) && \
                !str_cmp(GET_NAME(i->character), to_name.c_str()))
            { // if we're able to hear and our name matches, send tell
                send_to_char(i->character, \
                    "GV: %s@%s tells you '%s'.\r\n", \
                    from_name.c_str(), from_game.c_str(), \
                    message.c_str());
            }
        }
    }

    void GvChat::eventGamesConnect(json j)
    {

    }

    void GvChat::eventGamesDisconnect(json j)
    {

    }
    
    void GvChat::eventGamesStatus(json j)
    {

    }

// send a json message to GV if we're connected
    void GvChat::sendMessage(json j)
    {
        std::string event = j["event"];
        if (!isReady())
        {
            log("Attempting to send message when not ready: " + j.dump());
            return;
        }
        else if (event != "authenticate" && !authenticated)
        {
            log("Attempting to send message when not authenticated:" \
                + j.dump());
            return;
        }
        std::string output;
        output = j.dump();
        _webSocket.sendText(output);
    }

// notify GV when players sign in
    void GvChat::playerSignIn(chPtr ch)
    {
        json j;
        auto g = xg::newGuid();

        ch->gvGuid = g;
        j["event"] = "players/sign-in";
        j["ref"] = g;
        j["payload"] = { { "name", GET_NAME(ch) }};
        sendMessage(j);
    }

// notify GV when players sign out
    void GvChat::playerSignOut(chPtr ch)
    {
        json j;

        j["event"] = "players/sign-out";
        j["payload"] = { { "name", GET_NAME(ch) }};
        sendMessage(j);
    }

// get status for players in all games
    void GvChat::playerStatus(xg::Guid g)
    {
        json j;
        j["event"] = "players/status";
        j["ref"] = g;
        sendMessage(j);
    }

// get status for players in a specific game
    void GvChat::playerStatus(xg::Guid g, const std::string& game)
    {
        json j;
        j["event"] = "players/status";
        j["ref"] = g;
        j["payload"] = { { "game", removeSpaces(game) } };
        log("playerStatus debug: " + j.dump());
        sendMessage(j);
    }

// send a channel message to Grapevine
    void GvChat::sendChannel(chPtr ch, const std::string &message)
    {
        json j;
        auto g = xg::newGuid();
        ch->gvGuid = g;
        j["event"] = "channels/send";
        j["ref"] = g;
        j["payload"] = { 
            { "channel", *ch->gvChannel },
            { "name", GET_NAME(ch) },
            { "message", message } };
        sendMessage(j);
    }

// send tell to player in a different games
    void GvChat::sendTell(chPtr ch, const std::string &to_name,
        const std::string &to_game, const std::string &message)
    {
        json j;
        auto g = xg::newGuid();
        ch->gvGuid = g;
        auto now = std::chrono::system_clock::now();
        auto itt = std::chrono::system_clock::to_time_t(now);
        std::stringstream ssTp;
        ssTp << std::put_time(gmtime(&itt), "%FT%TZ");
        std::string sent_at = ssTp.str();
  
        j["event"] = "tells/send";
        j["ref"] = g;
        j["payload"] = { 
            { "to_name", to_name },
            { "to_game", to_game },
            { "from_name", GET_NAME(ch) },
            { "sent_at", sent_at },
            { "message", message } };
        sendMessage(j);
    }

 // find a character object with a given ref guid
 // TODO: multiple guid per player support
    chPtr GvChat::findCharByRef(xg::Guid g)
    {
        for (auto &i : character_list)
            if (i->gvGuid == g)
                return i;
        return nullptr;
    }

}

// Grapevine player commands

// get status of players or players on a specific game
ACMD(do_gvstatus)
{
  if (affected_by_spell(ch, SPELL_BLINDNESS)) 
  {
    send_to_char(ch, "You have been blinded! You cannot see this list!\r\n");
    return;
  }

  std::string game(argument);
  auto g = xg::newGuid();
  ch->gvGuid = g;

  if (!game.empty())
    GvChat->playerStatus(g, game);
  else
    GvChat->playerStatus(g);
}

// send tell via Grapevine
ACMD(do_gvtell)
{
  if (affected_by_spell(ch, SPELL_BLINDNESS)) 
  {
    send_to_char(ch, "You have been blinded! You cannot GVTell anyone anything!\r\n");
    return;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to GVTell what??\r\n");

  // split user@game into two variables
  vector<string> dest;
  stringstream ss(buf);
  string item;
  char delim = '@';

  while (getline (ss, item, delim)) 
  {
    dest.push_back(item);
  }

  // dest[0] = user @ dest[1] = game
  if (dest[0].empty() || dest[1].empty())
  {
    send_to_char(ch, "GVTell User@Game to send a message.\r\n");
    return;
  }

  GvChat->sendTell(ch, dest[0], dest[1], buf2);
  ch->gvMessage = new std::string("GV: You tell " + \
        string(buf) + " '" + string(buf2) + "'\r\n");

}

// broadcast via Grapevine channel
ACMD(do_gvchannel)
{
  one_argument(argument, arg);
  std::string broadcast(argument);
  std::string channel(arg);

  // change channel if the first arg is a channel name
  if (channel == "gossip" || channel == "testing")
  {
      if (ch->gvChannel)
        delete ch->gvChannel;
      ch->gvChannel = new std::string(channel);
      send_to_char(ch, "GV: Channel set to %s", arg);
      return;
  }

  // set default channel to gossip
  if (!ch->gvChannel)
    ch->gvChannel = new std::string("gossip");

  if (!broadcast.empty())
  {
    if (ch->gvMessage)
      delete ch->gvMessage;
    GvChat->sendChannel(ch, broadcast);
    ch->gvMessage = new std::string("GV: [" + *ch->gvChannel + "] " \
         + string(GET_NAME(ch)) + "@WoP:" + broadcast + "'\r\n");

  }
  else
    send_to_char(ch, "Say what on the Grapevine??\r\n");
}