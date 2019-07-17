/**************************************************************************
*   File: grapevine.h                             Part of World of Pain   *
*  Usage: Header file for grapevine chat                                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 2019 World of Pain                                       *
*                                                                         *
*  Based on ws_chat.cpp by: Benjamin Sergeant                             *
*  Copyright (c) 2017-2019 Machine Zone, Inc. All rights reserved.        *
***************************************************************************/

#include "conf.h"
#include <iostream>
#include <sstream>
#include <queue>
#include <mutex>
#include <vector>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXSocket.h>
#include "utils.h"
#include "json.hpp"

using json = nlohmann::json;

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

            void subscribe(const std::string& channel);
            void authenticate();
            void start();
            void stop();
            bool isReady() const;

            void sendMessage(json j);
            size_t getReceivedMessagesCount() const;
            size_t getLogMessagesCount() const;
            void processMessages();
            void processLog();

            std::string encodeMessage(const std::string& text);
            void decodeMessage(const std::string& str);

            void playerSignIn(chPtr ch);
            void playerSignOut(chPtr ch);
            void playerStatus(xg::Guid g);
            void playerStatus(xg::Guid g, const std::string& game);
            void sendChannel(chPtr ch, const std::string &message);
            void sendTell(chPtr ch, const std::string &to_name,
                const std::string &to_game, const std::string &message);
            void gameStatus(xg::Guid g);
            void gameStatus(xg::Guid g, const std::string& game);

        private:
            std::string _url;
            std::string _client_id;
            std::string _client_secret;
            std::string _version;
            std::string _user_agent;
            ix::WebSocket _webSocket;
            std::queue<std::string> _receivedQueue;
            std::queue<std::string> _logQueue;
            bool authenticated;
            mutable std::mutex _msgMutex;
            mutable std::mutex _logMutex;
            
            void log(const std::string& msg);
            chPtr findCharByRef(xg::Guid g);
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