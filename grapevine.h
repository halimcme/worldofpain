/**************************************************************************
*   File: grapevine.h                             Part of World of Pain   *
*  Usage: Header file for grapevine chat                                  *
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

#include "conf.h"
#include <iostream>
#include <sstream>
#include <queue>
#include <mutex>
#include <vector>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXSocket.h>
#include <crossguid/guid.hpp>
#include "utils.h"
#include "json.hpp"

using json = nlohmann::json;

#ifndef chPtr
typedef struct char_data * chPtr;
#endif

namespace ix
{
   class GvChat
    {
        public:
            GvChat(const std::string& url,
                                 const std::string& client_id,
                                 const std::string& client_secret,
                                 const std::string& version,
                                 const std::string& user_agent);

            void authenticate();
            void start();
            void stop();
            bool isReady() const;

            void sendMessage(json j);
            size_t getReceivedMessagesCount() const;
            size_t getLogMessagesCount() const;
            void processMessages();
            void processLog();
            void setSleeping(bool sleeping);

            std::string encodeMessage(const std::string& text);
            void decodeMessage(const std::string& str);

            void playerSignIn(chPtr ch);
            void playerSignOut(chPtr ch);
            void playersStatus(xg::Guid g, const std::string game = "");
            void updatePlayersStatus(const std::string game = "");
            void sendChannel(chPtr ch, const std::string &message);
            void sendTell(chPtr ch, const std::string &to_name,
                const std::string &to_game, const std::string &message);
            void gamesStatus(xg::Guid g, const std::string game = "");
            void updateGamesStatus(const std::string game = "");
            xg::Guid newAction(chPtr ch, const std::string action = "");

            std::map<std::string, std::vector<std::string>> playersOnline;
            std::map<std::string, json> gameStatus;

        private:
            std::string _url;
            std::string _client_id;
            std::string _client_secret;
            std::string _version;
            std::string _user_agent;
            ix::WebSocket _webSocket;
            std::queue<std::string> _receivedQueue;
            std::queue<std::string> _logQueue;
            bool _authenticated;
            bool _sleeping;
            mutable std::mutex _msgMutex;
            mutable std::mutex _logMutex;
            std::map<std::string, std::pair<chPtr, std::string>> _actions;
            std::queue<std::string> _processedActions;
            std::vector<std::string> _gameActions;
            bool _clearPlayers;
            bool _clearGames;
            
            void log(const std::string& msg);
            std::pair<chPtr, std::string> findAction(std::string g);
            void eraseAction(std::string g);
            void eraseActions(chPtr ch);
            void eventAuthenticate(json j);
            void eventHeartbeat(json j);
            void eventRestart(json j);
            void eventSubscribe(json j);
            void eventUnsubscribe(json j);
            void eventBroadcast(json j);
            void eventSignIn(json j);
            void eventSignOut(json j);
            void eventPlayersStatus(json j);
            void eventFailure(json j);
            void eventSuccess(json j);
            void eventTellsReceive(json j);
            void eventGamesConnect(json j, bool connected);
            void eventGamesStatus(json j);

    };
}

#ifndef __GRAPEVINE_CPP__
extern ix::GvChat *GvChat;
#endif