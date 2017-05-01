/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "RealmStdAfx.h"

initialiseSingleton(ClusterMgr);
ClusterMgr::ClusterMgr()
{
    memset(WorkerServers, 0, sizeof(WorkerServer*) * MAX_WORKER_SERVERS);
    m_maxWorkerServer = 0;
    LogDefault("ClusterMgr", "Interface Created");

    WorkerServer::InitHandlers();
}

WorkerServer * ClusterMgr::GetServerByMapId(uint32 MapId)
{
    WorkerServerMap::iterator itr = Maps.find(MapId);
    return (itr == Maps.end()) ? NULL : itr->second->workerServer;
}

WorkerServer* ClusterMgr::GetAnyWorkerServer()
{
    m_lock.AcquireReadLock();

    for (uint32 i = 0; i < MAX_WORKER_SERVERS; ++i)
    {
        if (WorkerServers[i] != NULL)
        {
            m_lock.ReleaseReadLock();
            return WorkerServers[i];
        }
    }

    m_lock.ReleaseReadLock();

    return NULL;
}

WorkerServer * ClusterMgr::CreateWorkerServer(WorkerServerSocket * s)
{
    /* find an id */
    m_lock.AcquireWriteLock();
    uint32 i;
    for (i = 1; i < MAX_WORKER_SERVERS; ++i)
    {
        if (WorkerServers[i] == 0)
            break;
    }

    if (i == MAX_WORKER_SERVERS)
    {
        m_lock.ReleaseWriteLock();
        return NULL;// No spaces
    }

    LogDebug("ClusterMgr", "Allocating worker server %u to %s:%u", i, s->GetRemoteIP().c_str(), s->GetRemotePort());

    if (m_maxWorkerServer < i)
        m_maxWorkerServer = i;

    m_lock.ReleaseWriteLock();
    return WorkerServers[i];
}

void ClusterMgr::Update()
{
    //Slave_lock.Acquire();

    for (uint32 i = 1; i <= m_maxWorkerServer; ++i)
        if (WorkerServers[i])
            WorkerServers[i]->Update();

    //Slave_lock.Release();
}

void ClusterMgr::DistributePacketToAll(WorldPacket * data, WorkerServer * exclude)
{
    for (uint32 i = 0; i <= m_maxWorkerServer; ++i)
        if (WorkerServers[i] && WorkerServers[i] != exclude)
            WorkerServers[i]->SendPacket(data);
}

void ClusterMgr::OnServerDisconnect(WorkerServer* s)
{
    //grab ze lock
    m_lock.AcquireWriteLock();

    if (Maps.size())
    {
        WorkerServerMap::iterator itr = Maps.begin();
        while (itr != Maps.end())
        {
            if (itr->second->workerServer == s)
            {
                LogWarning("ClusterMgr", "Removing Map %u on WorkerServer %u due to worker server disconnection", s->GetID(), itr->second->Mapid);
                delete itr->second;
                itr = Maps.erase(itr);
            }
            else
                itr++;
        }
        Maps.clear();
    }

    for (uint32 i = 0; i < m_maxWorkerServer; i++)
    {
        if (WorkerServers[i] == s)
        {
            LogWarning("ClusterMgr", "Removing Worker Server due to disconnection");
            WorkerServers[i] = NULL;
        }
    }

    delete s;

    m_lock.ReleaseWriteLock();
}