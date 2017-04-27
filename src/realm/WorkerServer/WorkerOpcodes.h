/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/


#ifndef _WORKER_OPCODES_H
#define _WORKER_OPCODES_H

enum WorkerServerOpcodes
{
    // Realmserver opcodes
    ISMSG_AUTH_REQUEST,             // auth request to worker server
    ICMSG_AUTH_REPLY,               // auth reply to worker server
    ISMSG_AUTH_RESULT,              // auth result from worker server
    ICMSG_REGISTER_WORKER,          // register a worker server

    // Player
    ICMSG_CREATE_PLAYER,            // create a character
    ICMSG_PLAYER_LOGIN_RESULT,      // player login result
    ICMSG_PLAYER_LOGOUT,            // player logout
    ICMSG_PLAYER_INFO,              // handle Player Info from worker server
    ICMSG_SAVE_ALL_PLAYERS,         // incoming msg to save all players
    ICMSG_SWITCH_SERVER,            // handle switch server request from worker server

    // Teleport
    ICMSG_TELEPORT_REQUEST,         // teleport request from worker server
    ICMSG_TRANSPORTER_MAP_CHANGE,   // unused currently

    // Chat
    ICMSG_WHISPER,                  // whisper request from worker server
    ICMSG_CHAT,                     // chat request from worker server

    // WoW Packets
    ICMSG_WOW_PACKET,               //

    // Connection Releated
    ICMSG_REALM_PING_STATUS,        // ping
    ICMSG_WORLD_PONG_STATUS,        // pong
    ICMSG_ERROR_HANDLER,            //

    // Instances and Worker server
    ISMSG_REGISTER_RESULT,          // register worker server result
    ISMSG_CREATE_INSTANCE,          // create instance
    ISMSG_DESTROY_INSTANCE,         // destroy instance

    // Player releated
    ISMSG_CREATE_PLAYER,            // send character create request to a worker server for handling
    ISMSG_PLAYER_LOGIN,             // login a player to a worker server
    ISMSG_PLAYER_INFO,              // handle Player Info to worker server
    ISMSG_PACKED_PLAYER_INFO,       // pack all clients together and send to worker servers  
    ISMSG_DESTROY_PLAYER_INFO,      // destroy Player Info 
    ISMSG_SAVE_ALL_PLAYERS,         // send to worker servers they need to save the players
   
    // Teleport releated
    ISMSG_TRANSPORTER_MAP_CHANGE,   // unused currently
    ISMSG_TELEPORT_RESULT,          // teleport result to worker server 
    ISMSG_SESSION_REMOVED,          // send session remove

    // WoW Packets
    ISMSG_WOW_PACKET,               //

    // Chat
    ISMSG_WHISPER,                  // send whisper request to specific worker
    ISMSG_CHAT,                     // send chat to worker servers

    IMSG_NUM_TYPES
};


enum RealmServerOpcodes
{
    LRCMSG_REALM_REGISTER_REQUEST       = 0x001,  // try register our realm
    LRSMSG_REALM_REGISTER_RESULT        = 0x002,  // register result from logonserver
    LRCMSG_ACC_SESSION_REQUEST          = 0x003,
    LRSMSG_ACC_SESSION_RESULT           = 0x004,
    LRCMSG_LOGON_PING_STATUS            = 0x005,  // request logon online
    LRSMSG_LOGON_PING_RESULT            = 0x006,  // send result if logon is online
    LRCMSG_FREE_01                      = 0x007,  // unused
    LRSMSG_FREE_02                      = 0x008,  // unused
    LRCMSG_AUTH_REQUEST                 = 0x009,  // try authenticate our realm
    LRSMSG_AUTH_RESPONSE                = 0x00A,  // authentication result from logonserver
    LRSMSG_ACC_CHAR_MAPPING_REQUEST     = 0x00B,
    LRCMSG_ACC_CHAR_MAPPING_RESULT      = 0x00C,
    LRCMSG_ACC_CHAR_MAPPING_UPDATE      = 0x00D,
    LRSMSG_SEND_ACCOUNT_DISCONNECT      = 0x00E,  // send when account is disconnected
    LRCMSG_LOGIN_CONSOLE_REQUEST        = 0x00F,
    LRSMSG_LOGIN_CONSOLE_RESULT         = 0x010,
    LRCMSG_ACCOUNT_DB_MODIFY_REQUEST    = 0x011,  // request logon db change
    LRSMSG_ACCOUNT_DB_MODIFY_RESULT     = 0x012,
    LRSMSG_REALM_POPULATION_REQUEST     = 0x013,
    LRCMSG_REALM_POPULATION_RESULT      = 0x014,
    LRCMSG_ACCOUNT_REQUEST              = 0x015,  // request account data
    LRSMSG_ACCOUNT_RESULT               = 0x016,  // send account information to realm

    LRMSG_MAX_OPCODES                           // max opcodes
};

#endif		// _WORKER_OPCODES_H


