/**************************************************************************
*   File: grapevine.cpp                             Part of World of Pain *
*  Usage: Main class and support commands for grapevine chat              *                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 2019 World of Pain                                       *
*  https://worldofpa.in                                                   *
*  Based on ws_chat.cpp by: Benjamin Sergeant                             *
*  Copyright (C) 2017-2019 Machine Zone, Inc. All rights reserved.        *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
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

// locally defined globals
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
        if (_sleeping)
        { // when sleeping, just write directly to the log
            basic_mud_log("GV CHAT: %s", msg.c_str());
        }
        else
        { // when awake, queue log messages
            std::lock_guard<std::mutex> lock(_logMutex);
            _logQueue.push(msg);
        }
    }

// count of queued recieved messages
    size_t GvChat::getReceivedMessagesCount() const
    {
        std::lock_guard<std::mutex> lock(_msgMutex);
        return _receivedQueue.size();
    }

// count of queued log messages
    size_t GvChat::getLogMessagesCount() const
    {
        std::lock_guard<std::mutex> lock(_logMutex);
        return _logQueue.size();
    }

// process message queue and erase processed actions
    void GvChat::processMessages()
    {
        std::lock_guard<std::mutex> lock(_msgMutex);
        while(!_receivedQueue.empty())
        {
            decodeMessage(_receivedQueue.front());
            _receivedQueue.pop();
        }
        while (!_processedActions.empty())
        {
            eraseAction(_processedActions.front());
            _processedActions.pop();
        }
    }

// process log queue
    void GvChat::processLog()
    {
        std::lock_guard<std::mutex> lock(_logMutex);
        while(!_logQueue.empty())
        {
            mudlog(NRM, LVL_IMMORT, TRUE, "GV CHAT: %s",
                _logQueue.front().c_str());
            _logQueue.pop();
        }
    }

// toggle game sleeping status
    void GvChat::setSleeping(bool sleeping)
    {
        _sleeping = sleeping;
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
                       _authenticated = false;
                }
                else if (msg->type == ix::WebSocketMessageType::Message)
                { // decode incoming messages immediately if sleeping
                    if (_sleeping)
                        decodeMessage(msg->str);
                    else
                    { // queue messages if the game is active
                        std::lock_guard<std::mutex> lock(_msgMutex);
                        _receivedQueue.push(msg->str);
                    }
                }
                else if (msg->type == ix::WebSocketMessageType::Error)
                { // log errors
                    ss << "Connection error: " << msg->errorInfo.reason      << std::endl;
                    ss << "#retries: "         << msg->errorInfo.retries     << std::endl;
                    ss << "Wait time(ms): "    << msg->errorInfo.wait_time   << std::endl;
                    ss << "HTTP Status: "      << msg->errorInfo.http_status << std::endl;
                    log(ss.str());
                    _authenticated = false;
                }
                else
                { // log unknown websocket responses
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
        j["payload"]["supports"] = { "channels", "players", "tells", "games" };
        j["payload"]["channels"] = { "gossip", "testing" };
        j["payload"]["version"] = _version;
        j["payload"]["user_agent"] = _user_agent;
        _authenticated = false;
        sendMessage(j);
    }

// decode an incoming message
    void GvChat::decodeMessage(const std::string& str)
    {
        auto j = json::parse(str);
        std::string event = j["event"];

        if (event == "authenticate")
            eventAuthenticate(j);
        else if (event == "heartbeat")
            eventHeartbeat(j);
        else if (event == "restart")
            eventRestart(j);
        else if (_sleeping)
            return; // only the events above are procssed while sleeping
        else if (j["status"] == "failure")
            eventFailure(j);
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
        else if (event == "games/connect")
            eventGamesConnect(j, true);
        else if (event == "games/disconnect")
            eventGamesConnect(j, false);
        else if (event == "games/status")
            eventGamesStatus(j);
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
            _authenticated = true;
        else
            _authenticated = false;
    }

// respond to heartbeat event
    void GvChat::eventHeartbeat(json j)
    {
        chPtr tch;
        vector <string> players;
        json k;

        k["event"] = "heartbeat";

        // build list of players for heartbeat
        if (!_sleeping)
        {
            for (auto &d : descriptor_list) 
            {
                if (d->original)
                    tch = d->original;
                else if (!(tch = d->character))
                    continue;
                else if (STATE(d) != CON_PLAYING)
                    continue; // don't show players that aren't in the game
                if (IS_GOD(tch) && GET_INVIS_LEV(tch))
                    continue; // don't show invisible gods

                players.push_back(GET_NAME(tch));
            }
            k["payload"]["players"] = players;
        }
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

// TODO: not yet implemented
    void GvChat::eventSubscribe(json j)
    {

    }

// TODO: not yet implemented
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
                    "%sGV [%s] %s@%s: %s.%s\r\n", \
                    CCYEL(i->character, C_NRM), \
                    channel.c_str(), name.c_str(), \
                    game.c_str(), message.c_str(), \
                    CCNRM(i->character, C_NRM));
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
            auto a = findAction(refStr);
            chPtr ch = a.first;
            if (!ch)
                return;
            send_to_char(ch, "%sYou have been signed in to Grapevine Chat.%s", \
                CCYEL(ch, C_NRM), CCNRM(ch, C_NRM));
            return;
        }
        else 
        { // we have a payload, a player signed in from another game
            std::string name = j["payload"]["name"];
            std::string game = j["payload"]["game"];
            for (auto &i : descriptor_list) 
            { // TODO: player toggle option, disabled for now
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
            { // TODO: player toggle option, disabled for now
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

        if (j["ref"].is_string())
        {
            std::string refStr = j["ref"];
            auto a = findAction(refStr);
            if (!a.first)
                return;
            chPtr ch = a.first;
            if (ch->gvFirstGame && ch->gvGameAll)
            { // print a header before the list of all games/players
                send_to_char(ch, \
                    "%sPlayers online in all Grapevine games:%s\r\n", \
                    CBYEL(ch, C_NRM), CCNRM(ch, C_NRM));
                ch->gvFirstGame = false;
            }
            std::string players;
            int count = 0; // count of online players in a game
            if (j["payload"]["players"].is_array())
            {
                std::string playerLine;
                int sw = GET_SCREEN_WIDTH(ch);

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
                        send_to_char(ch, "1 character online in %s%s%s: %s%s%s\r\n", \
                            CCBLU(ch, C_NRM), game.c_str(), CCNRM(ch, C_NRM), \
                            CCYEL(ch, C_NRM), players.c_str(), CCNRM(ch, C_NRM));
                    else
                    {
                        send_to_char(ch, "%d characters online in %s%s%s:\r\n", \
                            count, CCBLU(ch, C_NRM), game.c_str(), CCNRM(ch, C_NRM));
                        send_to_char(ch, "%s%s%s\r\n", \
                            CCYEL(ch, C_NRM), players.c_str(), CCNRM(ch, C_NRM));
                    }
                }
                else 
                {
                    send_to_char(ch, "No characters online in %s%s%s.\r\n", \
                        CCBLU(ch, C_NRM), game.c_str(), CCNRM(ch, C_NRM));
                }
            }
        }
    }

// notify player when a GV event fails
    void GvChat::eventFailure(json j)
    {
        std::string refStr = j["ref"];
        auto a = findAction(refStr);
        if (!a.first)
            return;
        chPtr ch = a.first;
        std::string event = j["event"];
        std::string error = j["error"];
        send_to_char(ch, "%sGV %s Failure: %s%s\r\n", \
            CCRED(ch, C_NRM), event.c_str(), error.c_str(), CCNRM(ch, C_NRM));
    }

// notify player when a GV event is successful
    void GvChat::eventSuccess(json j)
    {
        std::string refStr = j["ref"];
        auto a = findAction(refStr);
        chPtr ch = a.first;
        if (!ch || a.second.empty())
            return;
        
        if (j["event"] == "tells/send")
            send_to_char(ch, "%s%s%s", \
                CCRED(ch, C_NRM), a.second.c_str(), CCNRM(ch, C_NRM));
        else if (j["event"] == "channels/send")
            send_to_char(ch, "%s%s%s", \
                CCYEL(ch, C_NRM), a.second.c_str(), CCNRM(ch, C_NRM));

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
                    "%sGV: %s@%s%s%s tells you '%s'%s\r\n", \
                    CCRED(i->character, C_NRM),
                    from_name.c_str(), CCBLU(i->character, C_NRM), \
                    from_game.c_str(), CCRED(i->character, C_NRM), \
                    message.c_str(), CCNRM(i->character, C_NRM));
            }
        }
    }

// games/connect or games/disconnect events
    void GvChat::eventGamesConnect(json j, bool connected)
    {
        std::string game = j["payload"]["game"];

        for (auto &i : descriptor_list) 
        {
            if (connected)
                send_to_char(i->character, \
                    "%sGV: Game %s%s%s has connected to Grapevine.%s\r\n", \
                    CCYEL(i->character, C_NRM), CCBLU(i->character, C_NRM), \
                    game.c_str(), CCYEL(i->character, C_NRM), \
                    CCNRM(i->character, C_NRM));
            else
                send_to_char(i->character, \
                    "%sGV: Game %s%s%s has disconnected from Grapevine.%s\r\n", \
                    CCYEL(i->character, C_NRM), CCBLU(i->character, C_NRM), \
                    game.c_str(), CCYEL(i->character, C_NRM), \
                    CCNRM(i->character, C_NRM));
        }
    }
    
    void GvChat::eventGamesStatus(json j)
    {
        // we should always have a payload for games/status events
        if (!j["payload"].is_object())
            return;
        json p = j["payload"];
        std::string refStr = j["ref"];
        chPtr ch = findAction(refStr).first;
        if (!ch)
            return;
        std::stringstream ss;
        if (ch->gvGameAll)
        {
            if (ch->gvFirstGame)
            {  // print header for the first game only
                
                ss << CBYEL(ch, C_NRM) << "Games Online in Grapevine:";
                ss << CCNRM(ch, C_NRM) << endl;
                ch->gvFirstGame = false;
            }
            ss << "(" << CCBLU(ch, C_NRM) << p["game"].get<std::string>() << \
                CCNRM(ch, C_NRM) << ") ";
            ss << p["display_name"].get<std::string>();
            if (!p["player_online_count"].empty())
                ss << " (Players Online: " << p["player_online_count"] << ")";
            ss << endl;
        }
        else
        {
            ss << CCYEL(ch, C_NRM) << "GV Game: " << CCNRM(ch, C_NRM) << "(" \
                << CCBLU(ch, C_NRM) << p["game"].get<std::string>() \
                << CCNRM(ch, C_NRM) << ") ";
            ss << p["display_name"].get<std::string>() << endl;
            if (p["description"].is_string())
                ss << p["description"].get<std::string>() << endl;
            if (p["homepage_url"].is_string())
                ss << CCYEL(ch, C_NRM) << "URL: " << CCNRM(ch, C_NRM) \
                    << p["homepage_url"].get<std::string>() << endl;
            if (p["user_agent"].is_string())
                ss << CCYEL(ch, C_NRM) << "User Agent: " << CCNRM(ch, C_NRM) \
                    << p["user_agent"].get<std::string>() << endl;
            if (p["user_agent_repo_url"].is_string())
                ss << CCYEL(ch, C_NRM) << "Repo URL: " << CCNRM(ch, C_NRM) \
                    << p["user_agent_repo_url"].get<std::string>() << endl;
            if (!p["player_online_count"].empty())
                ss << CCYEL(ch, C_NRM) << "Players Online: " \
                    << CCNRM(ch, C_NRM) << p["player_online_count"] << endl;
            if (p["supports"].is_array())
            {
                ss << CCYEL(ch, C_NRM) << "Supports: " << CCNRM(ch, C_NRM);
                for (auto &sup : p["supports"])
                    ss << sup.get<std::string>() << " ";
                ss << endl;
            }
            if (p["connections"].is_array())
            {
                ss << CCYEL(ch, C_NRM) << "Connections: " << CCNRM(ch, C_NRM);
                for (auto &con : p["connections"])
                {
                    if (con["type"] == "web")
                        ss << endl << CCYEL(ch, C_NRM) << "    Web: " \
                        << CCNRM(ch, C_NRM) << con["url"].get<std::string>();
                    else if (con["type"] == "telnet")
                        ss << endl << CCYEL(ch, C_NRM) << "    Telnet: " \
                            << CCNRM(ch, C_NRM) \
                            << con["host"].get<std::string>() \
                            << ":" << con["port"];
                    else if (con["type"] == "secure telnet")
                        ss << endl << CCYEL(ch, C_NRM) \
                            << "    Secure Telnet: " << CCNRM(ch, C_NRM) \
                            << con["host"].get<std::string>() << ":" \
                            << con["port"];
                }
                //ss << endl;
            }
        }
        send_to_char(ch, "%s", ss.str().c_str());
        return;
    }

// send a json message to GV if we're connected
    void GvChat::sendMessage(json j)
    {
        std::string event = j["event"];
        if (!isReady())
        {  // make sure the socket is ready before sending
            log("Attempting to send message when not ready: " + j.dump());
            return;
        }
        else if (event != "authenticate" && !_authenticated)
        {  // make sure we're authenticated before sending
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
        auto g = newAction(ch, "");
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
        eraseActions(ch);
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
        sendMessage(j);
    }

// get status for all games
    void GvChat::gameStatus(xg::Guid g)
    {
        json j;
        j["event"] = "games/status";
        j["ref"] = g;
        sendMessage(j);
    }

// get status for players in a specific game
    void GvChat::gameStatus(xg::Guid g, const std::string& game)
    {
        json j;
        j["event"] = "games/status";
        j["ref"] = g;
        j["payload"] = { { "game", removeSpaces(game) } };
        sendMessage(j);
    }

// add a Guid for player actions
    xg::Guid GvChat::newAction(chPtr ch, std::string action = "")
    {
        auto g = xg::newGuid();
        _actions[g] = std::pair(ch, action);
        return g;
    }

// send a channel message to Grapevine
    void GvChat::sendChannel(chPtr ch, const std::string &message)
    {
        json j;
        std::string aMessage("GV [" + *ch->gvChannel + "] " \
         + std::string(GET_NAME(ch)) + "@WoP:" + message + "'\r\n");
        auto g = newAction(ch, aMessage);
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
        std::string aMessage(std::string(CCRED(ch, C_NRM)) + "GV: You tell " \
            + to_name + "@" + to_game + ", '" + message + "'" + \
            CCNRM(ch, C_NRM) + "\r\n");
        auto g = newAction(ch, aMessage);
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

 // find an action object with a given ref guid
    pair<chPtr, std::string> GvChat::findAction(std::string g)
    {
        if (_actions.count(g) > 0)
        {
            _processedActions.push(g);
            return _actions[g];
        }
        else
            return std::pair(nullptr,"");
    }

 // erase an action object with a given ref guid
    void GvChat::eraseAction(std::string g)
    {
        if (_actions.count(g) > 0)
            _actions.erase(g);
    }

 // erase all actions for a given player
    void GvChat::eraseActions(chPtr ch)
    {
        for (auto it = _actions.cbegin(); it != _actions.cend();)
            if ((*it).second.first == ch)
                it =_actions.erase(it);
            else
                ++it;
    }
}

// Grapevine player commands

// get status of players
ACMD(do_gvplayer)
{
  // no mobs allowed!
  if (IS_NPC(ch))
  {
      send_to_char(ch, "Only players are allowed on Grapevine!");
      return;
  }

  if (affected_by_spell(ch, SPELL_BLINDNESS)) 
  {
    send_to_char(ch, "You have been blinded! You cannot see this list!\r\n");
    return;
  }

  std::string game(argument);
  auto g = GvChat->newAction(ch);

  if (!game.empty()) // specific game
  {
    ch->gvGameAll = false;
    GvChat->playerStatus(g, game);
  }
  else  // all games
  {
    ch->gvGameAll = true;
    ch->gvFirstGame = true;
    GvChat->playerStatus(g);
  }
}

// get status of games
ACMD(do_gvgame)
{
  // no mobs allowed!
  if (IS_NPC(ch))
  {
      send_to_char(ch, "Only players are allowed on Grapevine!");
      return;
  }

  std::string game(argument);
  auto g = GvChat->newAction(ch);

  if (!game.empty())  // specific game
  { 
    ch->gvGameAll = false;
    GvChat->gameStatus(g, game);
  }
  else  // all games
  {
    ch->gvGameAll = true;
    ch->gvFirstGame = true;
    GvChat->gameStatus(g);
  }
}

// send tell via Grapevine
ACMD(do_gvtell)
{
  // no mobs allowed!
  if (IS_NPC(ch))
  {
      send_to_char(ch, "Only players are allowed on Grapevine!");
      return;
  }

  if (affected_by_spell(ch, SPELL_BLINDNESS)) 
  {
    send_to_char(ch, "You have been blinded! You cannot GVTell anyone anything!\r\n");
    return;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
  {
    send_to_char(ch, "Whom do you wish to GVTell what??\r\n");
    return;
  }

  // split user@game into two variables
  vector<std::string> dest;
  stringstream ss(buf);
  string item;
  char delim = '@';

  while (getline (ss, item, delim)) 
  {
    dest.push_back(item);
  }

  // dest[0] = user @ dest[1] = game
  if (dest.size() < 2 || dest[0].empty() || dest[1].empty())
  {
    send_to_char(ch, "GVTell User@Game to send a message.\r\n");
    return;
  }

  GvChat->sendTell(ch, dest[0], dest[1], buf2);
}

// broadcast via Grapevine channel
ACMD(do_gvchannel)
{
  // no mobs allowed!
  if (IS_NPC(ch))
  {
      send_to_char(ch, "Only players are allowed on Grapevine!");
      return;
  }

  one_argument(argument, arg);
  std::string message(argument);
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

  if (!message.empty())
  {
    GvChat->sendChannel(ch, message);
    message = "GV [" + *ch->gvChannel + "] " + GET_NAME(ch) + ":" + message;

    // send Grapevine message to all players that can hear
    for (auto &i : descriptor_list) 
    {
        if (i->character && can_hear_check(i->character) && i != ch->desc)
        {
            send_to_char(CCYEL(i->character, C_NRM), i->character);
            act(message.c_str(), FALSE, ch, 0, i->character, \
                0, 0, TO_VICT | TO_SLEEP);
            send_to_char(CCNRM(i->character, C_NRM), i->character);
        }
    }
  }
  else
    send_to_char(ch, "Say what on the Grapevine??\r\n");
}