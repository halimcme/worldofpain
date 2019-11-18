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
//#include <mysql++/null.h>
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
        setSleeping(false);
        _channels.push_back("gossip");
        _channels.push_back("testing");
    }

// log Grapevine to mud system log
    void GvChat::log(const std::string& msg)
    {
        if (isSleeping())
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

// process send queue
    void GvChat::processSends()
    {
        return;
        std::lock_guard<std::mutex> lock(_sendMutex);
        while(!_sendQueue.empty())
        {
            sendMessage(_sendQueue.front());
            _sendQueue.pop();
        }
    }

// toggle game sleeping status
    void GvChat::setSleeping(bool sleeping)
    {
        _sleeping = sleeping;
    }

// is the game sleeping?
    bool GvChat::isSleeping()
    {
        return _sleeping;
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
        log("Connecting to " + _url);

        _webSocket.setOnMessageCallback(
            [this](const WebSocketMessagePtr& msg)
            {
                std::stringstream ss;
                if (msg->type == ix::WebSocketMessageType::Open)
                { // if we're connected, authenticate
                    ss << "Connected to: " << msg->openInfo.uri << std::endl;
                    log(ss.str());
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
                    if (isSleeping())
                    {
                        decodeMessage(msg->str);
                    }
                    else
                    { // queue messages if the game is active
                        std::lock_guard<std::mutex> lock(_msgMutex);
                        _receivedQueue.push(msg->str);
                    }
                }
                else if (msg->type == ix::WebSocketMessageType::Error)
                { // log errors
                    ss << "Connection error: " << msg->errorInfo.reason;
                    ss << " #retries: "        << msg->errorInfo.retries;
                    ss << " Wait time(ms): "    << msg->errorInfo.wait_time;
                    ss << " HTTP Status: "      << msg->errorInfo.http_status;
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
        j["payload"]["channels"] = _channels;
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

        if (j["status"] == "failure")
            eventFailure(j);
        else if (event == "authenticate")
            eventAuthenticate(j);
        else if (event == "heartbeat")
            eventHeartbeat(j);
        else if (event == "restart")
            eventRestart(j);
        else if (event == "games/connect")
            eventGamesConnect(j, true);
        else if (event == "games/disconnect")
            eventGamesConnect(j, false);
        else if (event == "games/status")
            eventGamesStatus(j);
        else if (event == "players/sign-in")
            eventSignIn(j);
        else if (event == "players/sign-out")
            eventSignOut(j);
        else if (event == "players/status")
            eventPlayersStatus(j);
        else if (isSleeping())
            return; // only the events above are procssed while sleeping
        else if (event == "channels/broadcast")
            eventBroadcast(j);
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

// authentication event handler
    void GvChat::eventAuthenticate(json j)
    {
        std::stringstream ss;

        ss << "Authentication status: " << j["status"];

        if (j["status"] == "success")
        {
            ss << " unicode: " << j["payload"]["unicode"]
                << " version: " << j["payload"]["version"];
            _authenticated = true;
            updateGamesStatus();
            updatePlayersStatus();
            processSends();
            updateGamesStatus(GVGAME);
        }
        else
            _authenticated = false;
        log(ss.str());

    }

// heartbeat event handler
    void GvChat::eventHeartbeat(json j)
    {
        chPtr tch = nullptr;
        vector <std::string> players;
        json k;

        k["event"] = "heartbeat";

        // no players if sleeping
        if (!isSleeping())
        { // build list of players for heartbeat
            for (auto &d : descriptor_list)
            {
                if (d->original) {
                    tch = d->original;
                } else if (!(tch = d->character) || STATE(d) != CON_PLAYING) {
                    continue;
                }
                if (IS_GOD(tch) && GET_INVIS_LEV(tch)) {
                    continue; // don't show invisible gods
                }
                players.push_back(GET_NAME(tch));
            }
            k["payload"]["players"] = players;
        }
        sendMessage(k);
    }

// restart event handler
    void GvChat::eventRestart(json j)
    {
        std::stringstream ss;
        ss << "Server restarting... downtime: "
            << j["payload"]["downtime"];
        log(ss.str());
    }

// TODO(Blizzard): not yet implemented
// channels/subscribe event handler
    void GvChat::eventSubscribe(json j)
    {

    }

// TODO(Blizzard): not yet implemented
// channels/unsubscribe event handler
    void GvChat::eventUnsubscribe(json j)
    {

    }

// channels/broadcast event handler
    void GvChat::eventBroadcast(json j)
    {
        std::string channel, message, name, game;
        if (j["payload"]["channel"].is_string()) {
            channel = j["payload"]["channel"];
        }
        if (j["payload"]["message"].is_string()) {
            message = j["payload"]["message"];
        }
        if (j["payload"]["name"].is_string()) {
            name = j["payload"]["name"];
        }
        if (j["payload"]["game"].is_string()) {
            game = j["payload"]["game"];
        }

        for (auto &i : descriptor_list)
        {
            if (i->character && can_hear_check(i->character))
            {
                send_to_char(i->character, \
                    "%sGV [%s] %s@%s%s%s: %s.%s\r\n", \
                    CCYEL(i->character, C_NRM), channel.c_str(), \
                    name.c_str(), CBBLU(i->character, C_NRM), \
                    game.c_str(), CCYEL(i->character, C_NRM), \
                    message.c_str(), CCNRM(i->character, C_NRM));
            }
        }
    }

// NOTE: sign-in events can be either for our players or remote games
// players/sign-in event handler
    void GvChat::eventSignIn(json j)
    {
        // no payload means this is one of our players signing in
        if (!j["payload"].is_object())
        {  // notify a character with given reference of sign in
            if (isSleeping()) {
                return;
            }
            std::string refStr = j["ref"];
            auto a = findAction(refStr);
            chPtr ch = a.first;
            if (!ch) {
                return;
            }
            send_to_char(ch, "%sYou have been signed in to Grapevine Chat.%s",
                CBYEL(ch, C_NRM), CCNRM(ch, C_NRM));
            updateGamesStatus(GVGAME);
            return;
        }
        else
        { // we have a payload, a player signed in from another game
            std::string name = j["payload"]["name"];
            std::string game = j["payload"]["game"];

            updateGamesStatus(game);
            
            // avoid duplicates
            bool foundPlayer = false;
            if (!playersOnline[game].empty())
                for (auto p : playersOnline[game])
                    if (p == name)
                    {
                        foundPlayer = true;
                        break;
                    }
            if (!foundPlayer)
                playersOnline[game].push_back(name);

            for (auto &i : descriptor_list)
            {
                if (i->character && can_hear_check(i->character) &&
                    PLT_FLAGGED(i->character, PLT_GVPLAYERS))
                {
                    send_to_char(i->character,
                        "%sGV: %s has signed in to %s%s%s.\r\n",
                        CCYEL(i->character, C_NRM), name.c_str(),
                        CBBLU(i->character, C_NRM), game.c_str(),
                        CCNRM(i->character, C_NRM));
                }
            }
        }
    }

// players/sign-out event handler
    void GvChat::eventSignOut(json j)
    {
        std::string name = j["payload"]["name"];
        std::string game = j["payload"]["game"];

        updateGamesStatus(game);
        if (!playersOnline[game].empty())
        {
            for (auto p = playersOnline[game].begin();
                p != playersOnline[game].end();)
            {
                if (*p == name)
                   p = playersOnline[game].erase(p);
                else
                  ++p;
            }
        }

        if (isSleeping())
            return;

        for (auto &i : descriptor_list)
        {
            if (i->character && can_hear_check(i->character) &&
                PLT_FLAGGED(i->character, PLT_GVPLAYERS))
            {
                send_to_char(i->character,
                    "%sGV: %s has signed out of %s%s%s.\r\n",
                    CCYEL(i->character, C_NRM), name.c_str(),
                    CBBLU(i->character, C_NRM), game.c_str(),
                    CCNRM(i->character, C_NRM));
            }
        }
        return;
    }

// players/status event handler
    void GvChat::eventPlayersStatus(json j)
    {
        std::string game = j["payload"]["game"];

        playersOnline[game].clear();
        if (j["payload"]["players"].is_array())
            for (auto &p : j["payload"]["players"])
            {
                std::string player = p;
                playersOnline[game].push_back(player);
            }
    }

// failure GV event handler
    void GvChat::eventFailure(json j)
    {
        std::string refStr = j["ref"];
        auto a = findAction(refStr);
        if (!a.first)
            return;
        chPtr ch = a.first;
        std::string event = j["event"];
        std::string error = j["error"];
        send_to_char(ch, "%sGV %s Failure: %s%s\r\n",
            CCRED(ch, C_NRM), event.c_str(), error.c_str(), CCNRM(ch, C_NRM));
    }

// success GV event handler
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

// tells/receive event handler
    void GvChat::eventTellsReceive(json j)
    {
        std::string from_game = j["payload"]["from_game"];
        std::string from_name = j["payload"]["from_name"];
        std::string to_name = j["payload"]["to_name"];
        std::string message = j["payload"]["message"];

        for (auto &i : descriptor_list)
        { // loop through all online players
            if (i->character && can_hear_check(i->character) &&
                !str_cmp(GET_NAME(i->character), to_name.c_str()))
            { // if we're able to hear and our name matches, send tell
                send_to_char(i->character,
                    "%sGV: %s%s@%s%s%s tells you '%s'%s",
                    CCYEL(i->character, C_NRM), CCRED(i->character, C_NRM),
                    from_name.c_str(), CBBLU(i->character, C_NRM),
                    from_game.c_str(), CCRED(i->character, C_NRM),
                    message.c_str(), CCNRM(i->character, C_NRM));
            }
        }
    }

// games/connect and games/disconnect event handler
    void GvChat::eventGamesConnect(json j, bool connected)
    {
        std::string game = j["payload"]["game"];

        if (connected)
        {
            updateGamesStatus(game);
            updatePlayersStatus(game);
        }
        else
        {
            playersOnline.erase(game);
            gameStatus.erase(game);
        }

        if (isSleeping())
            return;

        for (auto &i : descriptor_list)
        {
            if (STATE(i) != CON_PLAYING || !i->character ||
                !PLT_FLAGGED(i->character, PLT_GVGAMES))
                continue;

            if (connected)
                send_to_char(i->character,
                    "%sGV: Game %s%s%s has connected to Grapevine.%s\r\n",
                    CCYEL(i->character, C_NRM), CBBLU(i->character, C_NRM),
                    game.c_str(), CCYEL(i->character, C_NRM),
                    CCNRM(i->character, C_NRM));
            else
                send_to_char(i->character,
                    "%sGV: Game %s%s%s has disconnected from Grapevine.%s\r\n",
                    CCYEL(i->character, C_NRM), CBBLU(i->character, C_NRM),
                    game.c_str(), CCYEL(i->character, C_NRM),
                    CCNRM(i->character, C_NRM));
        }
    }

// games/status event handler
    void GvChat::eventGamesStatus(json j)
    {
        // we should always have a payload for games/status events
        if (!j["payload"].is_object())
            return;
        json p = j["payload"];
        std::string game = p["game"];
        if (_clearGames)
        { // clear the game list on the first response
            gameStatus.clear();
            _clearGames = false;
        }
        gameStatus[game] = p;

        return;
    }

// send a json message to GV if we're online
    void GvChat::sendMessage(json j)
    {
        std::string event = j["event"];
        if (!isReady() || (event != "authenticate" && !_authenticated))
        {  // queue sent messages if not connected or authenticated
            std::lock_guard<std::mutex> lock(_sendMutex);
            _sendQueue.push(j.dump());
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
        auto g = newAction(ch);
        std::string name = GET_NAME(ch);
        j["event"] = "players/sign-in";
        j["ref"] = g;
        j["payload"] = { { "name", name }};
        sendMessage(j);

        // update local player
        bool foundPlayer = false;
        if (!playersOnline[GVGAME].empty())
        {
            for (auto p : playersOnline[GVGAME])
                if (p == name)
                {
                    foundPlayer = true;
                    break;
                }
        }
        if (!foundPlayer) {
            playersOnline[GVGAME].push_back(name);
        }
    }

// notify GV when players sign out
    void GvChat::playerSignOut(chPtr ch)
    {
        json j;
        std::string name = GET_NAME(ch);
        j["event"] = "players/sign-out";
        j["payload"] = { { "name", name }};
        sendMessage(j);
        eraseActions(ch);

        // remove from local game list of players
        if (!playersOnline[GVGAME].empty())
        {
            for (auto p = playersOnline[GVGAME].begin();
                p != playersOnline[GVGAME].end();)
            {
                if (*p == name)
                   p = playersOnline[GVGAME].erase(p);
                else
                  ++p;
            }
        }
    }

// get status for players
    void GvChat::playersStatus(xg::Guid g, const std::string game)
    {
        json j;
        j["event"] = "players/status";
        j["ref"] = g;
        if (!game.empty())
            j["payload"] = { { "game", removeSpaces(game) } };
        sendMessage(j);
    }

// get status for games
    void GvChat::gamesStatus(xg::Guid g, const std::string game)
    {
        json j;
        j["event"] = "games/status";
        j["ref"] = g;
        if (!game.empty())
            j["payload"] = { { "game", removeSpaces(game) } };
        sendMessage(j);
    }

// add a Guid for player actions
    xg::Guid GvChat::newAction(chPtr ch, std::string action)
    {
        auto g = xg::newGuid();
        _actions[g] = std::pair(ch, action);
        return g;
    }

// send a channel message to Grapevine
    void GvChat::sendChannel(chPtr ch, const std::string &message)
    {
        json j;
        std::string aMessage("GV [" + *ch->player.gvChannel + "] "
         + std::string(GET_NAME(ch)) + "@" + GVGAME + ":" + message + "\r\n");
        auto g = newAction(ch, aMessage);
        j["event"] = "channels/send";
        j["ref"] = g;
        j["payload"] = {
            { "channel", *ch->player.gvChannel },
            { "name", GET_NAME(ch) },
            { "message", message } };
        sendMessage(j);
    }

// send tell to player in a different game
    void GvChat::sendTell(chPtr ch, const std::string &to_name,
        const std::string &to_game, const std::string &message)
    {
        json j;
        std::string aMessage(std::string(CCYEL(ch, C_NRM)) + "GV: "
            + CCRED(ch, C_NRM) + "You tell " + to_name + "@"
            + CBBLU(ch, C_NRM) + to_game + CCRED(ch, C_NRM)
            + ", '" + message + "'" +
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

  // update online games Status
    void GvChat::updateGamesStatus(std::string game)
    {
        auto g = xg::newGuid();
        gamesStatus(g, game);
        if (game.empty())
            _clearGames = true;
        else
            _clearGames = false;

    }

  // update online players status
    void GvChat::updatePlayersStatus(std::string game)
    {
        auto g = xg::newGuid();
        playersStatus(g, game);
        if (game.empty()) {
            _clearPlayers = true;
        } else {
            _clearPlayers = false;
        }
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

  std::string game(removeSpaces(argument));
  std::stringstream ss;

  if (game.empty()) // all games
  {
      int games = 0, total = 0;
      vector <std::string> noPlayers;

      for (const auto &[g, j]: GvChat->gameStatus)
      {
        int count = 0;
        if (!j.empty() && j["player_online_count"].is_number_integer()) {
          count = j["player_online_count"].get<int>();
        }
        if (!GvChat->playersOnline[g].empty())
        {
            if (GvChat->playersOnline[g].size() > count)
            {
                count = GvChat->playersOnline[g].size();
            }
        }
        total += count;
        ++games;
        if (!count)
        {
            noPlayers.push_back(g);
            continue;
        }
        else
            ss << CCYEL(ch, C_NRM) << count << " player"
               << (count == 1 ? "" : "s") << " online on "
               << CBBLU(ch, C_NRM) << g << CCNRM(ch, C_NRM) << ":";
        for (const auto &p : GvChat->playersOnline[g])
        {
          ss << " " << p;
        }
        ss << CCNRM(ch, C_NRM) << endl;
      }
      if (noPlayers.size())
      {
        ss << CCYEL(ch, C_NRM) << "No players online on:" << CBBLU(ch, C_NRM);
        for (const auto &g: noPlayers)
          ss << " " << g;
        ss << CCNRM(ch, C_NRM) << endl;
      }
      sprintf(buf, "%s%d players in %d games on Grapevine:%s\r\n%s",
          CBYEL(ch, C_NRM), total, games, CCNRM(ch, C_NRM), ss.str().c_str());
      page_string(ch->desc, buf, TRUE, "");
  }
  else
  {
      if (!GvChat->gameStatus.count(game))
      {
        send_to_char(ch, "Game %s not found on Grapevine.\r\n", game.c_str());
        return;
      }
      int count = GvChat->playersOnline[game].size();
      sprintf(buf, "player%s in Grapevine game %s%s%s%s",
        (count == 1) ? "" : "s", CBBLU(ch, C_NRM), game.c_str(),
        CCNRM(ch, C_NRM), count ? ": " : "");
      for (auto &p : GvChat->playersOnline[game])
      {
          sprintf(buf, "%s%s%s%s ", buf,
            CCYEL(ch, C_NRM), p.c_str(), CCNRM(ch, C_NRM));
      }
      if (count) {
        sprintf(buf1, "%s%d %s", CBYEL(ch, C_NRM), count, buf);
      } else {
        sprintf(buf1, "%sNo %s", CBYEL(ch, C_NRM), buf);
      }
      page_string(ch->desc, buf1, TRUE, "");

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

  std::string game(removeSpaces(argument));
  std::stringstream ss;

  if (game.empty())  // all games
  {
    int games = 0, total = 0, player_online_count = 0;

    for (const auto [g, j] : GvChat->gameStatus)
    {
        ss << "(" << CBBLU(ch, C_NRM) << g << \
            CCNRM(ch, C_NRM) << ") ";
        if (j["display_name"].is_string()) {
            ss << CCYEL(ch, C_NRM) << j["display_name"].get<std::string>();
        }
        ss << CCNRM(ch, C_NRM);
 
        if (!GvChat->playersOnline[game].empty())
        {
            player_online_count = GvChat->playersOnline[game].size();
        }
        else
        {
            player_online_count = j["player_online_count"].get<int>();
        }

        if (player_online_count > 0)
        {
            ss << " (Players Online: " << player_online_count << ")";
            total += player_online_count;
        }
  
        ss << endl;

        ++games;
    }
    sprintf(buf, "%s%d players online on %d games in Grapevine:%s\r\n%s",
          CBYEL(ch, C_NRM), total, games, CCNRM(ch, C_NRM), ss.str().c_str());
    page_string(ch->desc, buf, TRUE, "");
  }
  else  // specific game
  {
    if (!GvChat->gameStatus.count(game))
    {
        send_to_char(ch, "Game %s not found in Grapevine.\r\n", game.c_str());
        return;
    }
    json g = GvChat->gameStatus[game];
    int player_online_count = 0;
    if (!GvChat->playersOnline[game].empty())
    {
        player_online_count = GvChat->playersOnline[game].size();
    }
    else if (g["player_online_count"].is_number())
    {
        player_online_count = g["player_online_count"].get<int>();
    }

    ss << CBYEL(ch, C_NRM) << "GV Game: " << CCNRM(ch, C_NRM) << "("
        << CBBLU(ch, C_NRM) << g["game"].get<std::string>()
        << CCNRM(ch, C_NRM) << ") ";
    ss << g["display_name"].get<std::string>() << endl;
    if (g["description"].is_string())
        ss << g["description"].get<std::string>() << endl;
    if (g["homepage_url"].is_string())
        ss << CCYEL(ch, C_NRM) << "URL: " << CCNRM(ch, C_NRM)
            << g["homepage_url"].get<std::string>() << endl;
    if (g["user_agent"].is_string())
        ss << CCYEL(ch, C_NRM) << "User Agent: " << CCNRM(ch, C_NRM)
            << g["user_agent"].get<std::string>() << endl;
    if (g["user_agent_repo_url"].is_string())
        ss << CCYEL(ch, C_NRM) << "Repo URL: " << CCNRM(ch, C_NRM)
            << g["user_agent_repo_url"].get<std::string>() << endl;
    ss << CCYEL(ch, C_NRM) << "Players Online: "
        << CCNRM(ch, C_NRM) << player_online_count << endl;
    if (g["supports"].is_array())
    {
        ss << CCYEL(ch, C_NRM) << "Supports: " << CCNRM(ch, C_NRM);
        for (auto &sup : g["supports"])
            ss << sup.get<std::string>() << " ";
        ss << endl;
    }
    if (g["connections"].is_array())
    {
        ss << CCYEL(ch, C_NRM) << "Connections: " << CCNRM(ch, C_NRM);
        for (auto &con : g["connections"])
        {
            if (con["type"] == "web")
                ss << endl << CCYEL(ch, C_NRM) << "    Web: "
                << CCNRM(ch, C_NRM) << con["url"].get<std::string>();
            else if (con["type"] == "telnet")
                ss << endl << CCYEL(ch, C_NRM) << "    Telnet: "
                    << CCNRM(ch, C_NRM)
                    << con["host"].get<std::string>()
                    << ":" << con["port"];
            else if (con["type"] == "secure telnet")
                ss << endl << CCYEL(ch, C_NRM)
                    << "    Secure Telnet: " << CCNRM(ch, C_NRM)
                    << con["host"].get<std::string>() << ":"
                    << con["port"];
        }
    }
    sprintf(buf, "%s", ss.str().c_str());
    page_string(ch->desc, buf, TRUE, "");
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
      if (ch->player.gvChannel)
        delete ch->player.gvChannel;
      ch->player.gvChannel = new std::string(channel);
      send_to_char(ch, "%sGV: Channel set to %s%s", CCYEL(ch, C_NRM), arg,
        CCNRM(ch, C_NRM));
      return;
  }

  // set default channel to gossip
  if (!ch->player.gvChannel)
    ch->player.gvChannel = new std::string("gossip");

  if (!message.empty())
  {
    GvChat->sendChannel(ch, message);
    message = "GV [" + *ch->player.gvChannel + "] " + GET_NAME(ch) + ":" + message;

    // send Grapevine message to all players that can hear
    for (auto &i : descriptor_list)
    {
        if (i->character && can_hear_check(i->character) && i != ch->desc &&
            i->character->player.gvChannel)
        {
            send_to_char(CCYEL(i->character, C_NRM), i->character);
            act(message.c_str(), FALSE, ch, 0, i->character,
                0, 0, TO_VICT | TO_SLEEP);
            send_to_char(CCNRM(i->character, C_NRM), i->character);
        }
    }
  }
  else
    send_to_char(ch, "Say what on the Grapevine??\r\n");
}

// set Grapevine preferences
ACMD(do_gvset)
{
  // no mobs allowed!
  if (IS_NPC(ch))
  {
      send_to_char(ch, "Only players are allowed on Grapevine!");
      return;
  }

  one_argument(argument, buf);

  if (!*buf)
  {
    send_to_char(ch,
      "Usage: gvset <games | players | channels>\r\n");
    return;
  }


  if (is_abbrev(buf, "games"))
  {
      // toggle games
      TOGGLE_BIT(PLT_FLAGS(ch), PLT_GVGAMES);
      send_to_char(ch, "Grapevine game status messages are now %s.",
        ONOFF(PLT_FLAGGED(ch, PLT_GVGAMES)));
  }
  else if (is_abbrev(buf, "players"))
  {
      // toggle players
      TOGGLE_BIT(PLT_FLAGS(ch), PLT_GVPLAYERS);
      send_to_char(ch, "Grapevine player status messages are now %s.",
        ONOFF(PLT_FLAGGED(ch, PLT_GVPLAYERS)));
  }
  else if (is_abbrev(buf, "channels"))
  {
      // toggle channel messages
      TOGGLE_BIT(PLT_FLAGS(ch), PLT_GVNOCHANNELS);
      send_to_char(ch, "Grapevine channel messages are now %s.",
        ONOFF(!PLT_FLAGGED(ch, PLT_GVNOCHANNELS)));
  }
  else
  {
    send_to_char(ch,
      "Usage: gvset <games | players | channels>\r\n");
    return;
  }
}
