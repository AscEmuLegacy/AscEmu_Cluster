/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "RealmStdAfx.h"

WorkerServerHandler WorkerServer::PHandlers[IMSG_NUM_TYPES];

void WorkerServer::InitHandlers()
{
    memset(PHandlers, 0, sizeof(void*) * IMSG_NUM_TYPES);
    PHandlers[ICMSG_REGISTER_WORKER] = &WorkerServer::HandleRegisterWorker;
    PHandlers[ICMSG_WOW_PACKET] = &WorkerServer::HandleWoWPacket;
    PHandlers[ICMSG_PLAYER_LOGIN_RESULT] = &WorkerServer::HandlePlayerLoginResult;
    PHandlers[ICMSG_PLAYER_LOGOUT] = &WorkerServer::HandlePlayerLogout;
    PHandlers[ICMSG_TELEPORT_REQUEST] = &WorkerServer::HandleTeleportRequest;
    PHandlers[ICMSG_ERROR_HANDLER] = &WorkerServer::HandleError;
    PHandlers[ICMSG_SWITCH_SERVER] = &WorkerServer::HandleSwitchServer;
    PHandlers[ICMSG_SAVE_ALL_PLAYERS] = &WorkerServer::HandleSaveAllPlayers;
    PHandlers[ICMSG_TRANSPORTER_MAP_CHANGE] = &WorkerServer::HandleTransporterMapChange;
    PHandlers[ICMSG_CREATE_PLAYER] = &WorkerServer::HandleCreatePlayerResult;
    PHandlers[ICMSG_PLAYER_INFO] = &WorkerServer::HandlePlayerInfo;
    PHandlers[ICMSG_WORLD_PONG_STATUS] = &WorkerServer::Pong;
}

WorkerServer::WorkerServer(uint32 id, WorkerServerSocket * s) : m_id(id), m_socket(s)
{

}

void WorkerServer::HandleCreatePlayerResult(WorldPacket & pck)
{
    uint32 accountid;
    uint8 result;
    pck >> accountid >> result;

    //ok, we need session by account id... gay
    Session* s = sClientMgr.GetSessionByAccountId(accountid);
    if (s == NULL)
        return;

    s->GetSocket()->OutPacket(SMSG_CHAR_CREATE, 1, &result);
}

void WorkerServer::HandleTransporterMapChange(WorldPacket & pck)
{
    LogDetail("WServer", "Recieved ICMSG_TRANSPORTER_MAP_CHANGE");
}

void WorkerServer::HandleSaveAllPlayers(WorldPacket & pck)
{
    //relay this to all world servers
    WorldPacket data(ISMSG_SAVE_ALL_PLAYERS, 1);
    data << uint8(0);
    sClusterMgr.DistributePacketToAll(&data);
}

void WorkerServer::HandleSwitchServer(WorldPacket & pck)
{
    uint32 sessionid, guid, _class, mapid, instanceid;
    LocationVector location;
    float o;

    pck >> sessionid;
    pck >> guid;
    pck >> _class;
    pck >> mapid;
    pck >> instanceid;
    pck >> location;
    pck >> o;

    Session* s = sClientMgr.GetSession(sessionid);

    if (s == NULL)
        return;

    s->SetNextServer();

    WorldPacket data;
    data.SetOpcode(ISMSG_PLAYER_LOGIN);
    data << guid;
    data << mapid;
    data << instanceid;
    data << s->GetAccountId();
    data << s->GetAccountFlags();
    data << s->GetSessionId();
    data << s->GetAccountPermissions();
    data << s->GetAccountName();
    data << 0;
    data << _class;
    data << m_socket;
    s->GetNextServer()->SendPacket(&data);
    printf("Switch to worker Server %u  \n", s->GetNextServer()->GetID());
}

void WorkerServer::HandleRegisterWorker(WorldPacket & pck)
{
    std::vector<uint32> maps;
    std::vector<uint32> instancedmaps;
    uint32 build;

    pck >> build;
    pck >> maps;
    pck >> instancedmaps;

    /* send a packed packet of all online players to this server */
    sClientMgr.SendPackedClientInfo(this);

    std::vector<uint32> result2;
    result2.reserve(maps.size());

    // Filter Maps for This Worker
    for (std::vector<uint32>::iterator itr = maps.begin(); itr != maps.end(); ++itr)
                result2.push_back(*itr);

    //Append Maps
    for (std::vector<uint32>::iterator itr = result2.begin(); itr != result2.end(); ++itr)
    {
        Servers* i = new Servers;
        i->Mapid = (*itr);
        i->workerServer = this;
        sClusterMgr.Maps.insert(std::pair<uint32, Servers*>((*itr), i));
        LogDetail("ClusterMgr", "Allocating map %u to worker %u", (*itr), GetID());
    }

    std::vector<uint32> result;
    result.reserve(instancedmaps.size());

    // Filter Instanced Maps for This Worker
    for (std::vector<uint32>::iterator itr = instancedmaps.begin(); itr != instancedmaps.end(); ++itr)
            result.push_back(*itr);

    //Append Instanced Maps
    for (std::vector<uint32>::iterator itr2 = result.begin(); itr2 != result.end(); ++itr2)
    {
        Servers* i = new Servers;
        i->Mapid  = (*itr2);
        i->workerServer = this;
        sClusterMgr.Maps.insert(std::pair<uint32, Servers*>((*itr2), i));
        LogDetail("ClusterMgr", "Allocating map %u to worker %u", (*itr2), GetID());
    }
}

void WorkerServer::HandleWoWPacket(WorldPacket & pck)
{
    uint32 sessionid, size;
    uint16 opcode;

    /* get session */
    pck >> sessionid >> opcode >> size;
    Session * session = sClientMgr.GetSession(sessionid);
    if (!session) return;

    /* write it to that session's output buffer */
    WorldSocket * s = session->GetSocket();
    if (s) s->OutPacket(opcode, size, size ? ((const void*)(pck.contents() + 10)) : 0);
}

void WorkerServer::HandlePlayerLogout(WorldPacket & pck)
{
    uint32 sessionid, guid;
    pck >> sessionid >> guid;
    RPlayerInfo * pi = sClientMgr.GetRPlayer(guid);
    Session * s = sClientMgr.GetSession(sessionid);
    if (pi && s)
    {
        /* tell all other servers this player has gone offline */
        WorldPacket data(ISMSG_DESTROY_PLAYER_INFO, 4);
        data << guid;
        sClusterMgr.DistributePacketToAll(&data, this);

        /* clear the player from the session */
        s->ClearCurrentPlayer();
        s->ClearServers();

        /* destroy the playerinfo struct here */
        sClientMgr.DestroyRPlayerInfo(guid);
    }
}

void WorkerServer::HandleTeleportRequest(WorldPacket & pck)
{
    WorldPacket data(ISMSG_TELEPORT_RESULT, 100);
    RPlayerInfo* pi;
    Session* s;
    WorkerServer* dest;
    uint32 mapid, sessionid, instanceid;

    /* this packet is only used upon changing main maps! */
    pck >> sessionid >> mapid >> instanceid;

    s = sClientMgr.GetSession(sessionid);
    if (s)
    {
        pi = s->GetPlayer();
        ASSERT(pi);

        /* find the destination server */
        dest = sClusterMgr.GetServerByMapId(mapid);

        /* server up? */
        if (dest == NULL)
        {
            data.Initialize(SMSG_TRANSFER_ABORTED);
            data << uint32(0x02);	// INSTANCE_ABORT_NOT_FOUND
            s->SendPacket(&data);
        }
        else
        {
            /* server found! */
            LocationVector vec;
            pck >> vec >> vec.o;

            pi->MapId = mapid;
            pi->InstanceId = instanceid;
            pi->PositionX = vec.x;
            pi->PositionY = vec.y;

            if (dest == s->GetServer())
            {
                /* we're not changing servers, the new instance is on the same server */
                data << sessionid << uint8(1) << mapid << instanceid << vec << vec.o;
                SendPacket(&data);
            }
            else
            {
                /* notify the old server to pack the player info together to send to the new server, and delete the player */
                data << sessionid << uint8(0) << mapid << instanceid << vec << vec.o;
                //cache this to next server and switch servers when were ready :P
                s->SetNextServer(dest);
                SendPacket(&data);
            }

            data.Initialize(ISMSG_PLAYER_INFO);
            pi->Pack(data);
            sClusterMgr.DistributePacketToAll(&data, this);
        }
    }
}

void WorkerServer::HandlePlayerLoginResult(WorldPacket & pck)
{
    uint32 guid, sessionid;
    uint8 result;
    pck >> guid >> sessionid >> result;
    if (result)
    {
        LogDefault("WServer", "Worldserver %u reports successful login of player %u", m_id, guid);
        Session * s = sClientMgr.GetSession(sessionid);
        if (s)
        {
            /* update server */
            s->SetNextServer();

            /* pack together a player info packet and distribute it to all the other servers */
            ASSERT(s->GetPlayer());

            WorldPacket data(ISMSG_PLAYER_INFO, 200);
            data << s->GetPlayer()->Guid;
            s->GetPlayer()->Pack(data);

            sClusterMgr.DistributePacketToAll(&data);
        }
    }
    else
    {
        LogError("WServer", "Worldserver %u reports failed login of player %u", m_id, guid);
        Session * s = sClientMgr.GetSession(sessionid);
        if (s)
        {
            s->ClearCurrentPlayer();
            s->ClearServers();
        }

        sClientMgr.DestroyRPlayerInfo(guid);
    }
}

void WorkerServer::Update()
{
    WorldPacket * pck;
    uint16 opcode;
    while ((pck = m_recvQueue.Pop()))
    {
        opcode = pck->GetOpcode();
        if (opcode < IMSG_NUM_TYPES && WorkerServer::PHandlers[opcode] != 0)
            (this->*WorkerServer::PHandlers[opcode])(*pck);
        else
            LogError("WorkerServer", "Unhandled packet %u\n", opcode);
    }
    /*
    uint32 t = (uint32)UNIXTIME;
    // Ping the World Server to check for Disconnection.

    if (last_pong < t && ((t - last_pong) > 60))
    {
    // no pong for 60 seconds -> remove the socket
    printf("Remove the Socket time out \n");

    }

    if ((t - last_ping) > 15)
    {
    // send a ping packet.
    SendPing();
    }*/

}

void WorkerServer::SendPing()
{
    /*pingtime = getMSTime();
    WorldPacket data(ICMSG_REALM_PING_STATUS, 4);
    SendPacket(&data);

    last_ping = (uint32)UNIXTIME;*/
}

void WorkerServer::HandleError(WorldPacket & pck)
{
    uint32 sessionid;
    uint8 errorcode;

    pck >> sessionid >> errorcode;

    switch (errorcode)
    {
    case 1: //no session
        sClientMgr.DestroySession(sessionid);
        break;
    }
}

void WorkerServer::HandlePlayerInfo(WorldPacket & pck)
{
    uint32 guid;
    pck >> guid;
    RPlayerInfo * pRPlayer = sClientMgr.GetRPlayer(guid);
    ASSERT(pRPlayer);

    pRPlayer->Unpack(pck);
}

void WorkerServer::Pong(WorldPacket & pck)
{
    latency = getMSTime() - pingtime;
    last_pong = uint32(time(NULL));
}

