/*
Copyright (c) 2014-2017 AscEmu Team <http://www.ascemu.org/>
This file is released under the MIT license. See README-MIT for more information.
*/


#include "RealmStdAfx.h"

initialiseSingleton(ClusterMgr);
ClusterMgr::ClusterMgr()
{
    memset(SingleInstanceMaps, 0, sizeof(WorkerServer*) * MAX_SINGLE_MAPID);
    memset(WorkerServers, 0, sizeof(WorkerServer*) * MAX_WORKER_SERVERS);
    m_maxInstanceId = 0;
    m_maxWorkerServer = 0;
    LogDefault("ClusterMgr", "Interface Created");

    WorkerServer::InitHandlers();
}

WorkerServer * ClusterMgr::GetServerByInstanceId(uint32 InstanceId)
{
    InstanceMap::iterator itr = Instances.find(InstanceId);
    return (itr == Instances.end()) ? 0 : itr->second->Server;
}

WorkerServer * ClusterMgr::GetServerByMapId(uint32 MapId)
{
    ASSERT(IS_MAIN_MAP(MapId));
    return SingleInstanceMaps[MapId]->Server;
}

Instance * ClusterMgr::GetInstanceByInstanceId(uint32 InstanceId)
{
    InstanceMap::iterator itr = Instances.find(InstanceId);
    return (itr == Instances.end()) ? 0 : itr->second;
}

Instance * ClusterMgr::GetInstanceByMapId(uint32 MapId)
{
    m_lock.AcquireReadLock();
    Instance* s = SingleInstanceMaps[MapId];
    m_lock.ReleaseReadLock();
    return s;
}

Instance* ClusterMgr::GetAnyInstance()
{
    //
    m_lock.AcquireReadLock();
    for (uint32 i = 0; i<MAX_SINGLE_MAPID; ++i)
    {
        if (SingleInstanceMaps[i] != NULL)
        {
            m_lock.ReleaseReadLock();
            return SingleInstanceMaps[i];
        }
    }
    m_lock.ReleaseReadLock();
    return NULL;

}

Instance * ClusterMgr::GetPrototypeInstanceByMapId(uint32 MapId)
{
    m_lock.AcquireReadLock();
    //lets go through all the instances of this map and find the one with the least instances :P
    std::multimap<uint32, Instance*>::iterator itr = InstancedMaps.find(MapId);

    if (itr == InstancedMaps.end())
    {
        m_lock.ReleaseReadLock();
        return NULL;
    }

    Instance* i = NULL;
    uint32 min = 500000;
    for (; itr != InstancedMaps.upper_bound(MapId); ++itr)
    {
        if (itr->second->MapCount < min)
        {
            min = itr->second->MapCount;
            i = itr->second;
        }
    }

    m_lock.ReleaseReadLock();
    return i;
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
    WorkerServers[i] = new WorkerServer(i, s);
    if (m_maxWorkerServer < i)
        m_maxWorkerServer = i;
    m_lock.ReleaseWriteLock();
    return WorkerServers[i];
}

void ClusterMgr::AllocateInitialInstances(WorkerServer * server, std::vector<uint32>& preferred)
{
    m_lock.AcquireReadLock();
    std::vector<uint32> result;
    result.reserve(10);

    for (std::vector<uint32>::iterator itr = preferred.begin(); itr != preferred.end(); ++itr)
    {
        if (SingleInstanceMaps[*itr] == 0)
        {
            if (sMySQLStore.GetWorldMapInfo(*itr)->workerid == server->GetID())
                result.push_back(*itr);
        }
    }
    m_lock.ReleaseReadLock();


    for (std::vector<uint32>::iterator itr = result.begin(); itr != result.end(); ++itr)
    {
        CreateInstance(*itr, server);
    }
}

Instance * ClusterMgr::CreateInstance(uint32 MapId, WorkerServer * server)
{
    Instance * pInstance = new Instance;
    pInstance->InstanceId = ++m_maxInstanceId;
    pInstance->MapId = MapId;
    pInstance->Server = server;

    m_lock.AcquireWriteLock();
    Instances.insert(std::make_pair(pInstance->InstanceId, pInstance));

    if (IS_MAIN_MAP(MapId))
        SingleInstanceMaps[MapId] = pInstance;
    m_lock.ReleaseWriteLock();

    LogDebug("ClusterMgr", "Allocating instance %u on map %u to server %u", pInstance->InstanceId, pInstance->MapId, server->GetID());
    return pInstance;
}

WorkerServer * ClusterMgr::GetWorkerServerForNewInstance()
{
    WorkerServer * lowest = 0;
    int32 lowest_load = -1;

    /* for now we'll just work with the instance count. in the future we might want to change this to
    use cpu load instead. */

    m_lock.AcquireReadLock();
    for (uint32 i = 0; i < MAX_WORKER_SERVERS; ++i) {
        if (WorkerServers[i] != 0) {
            if ((int32)WorkerServers[i]->GetInstanceCount() < lowest_load)
            {
                lowest = WorkerServers[i];
                lowest_load = int32(WorkerServers[i]->GetInstanceCount());
            }
        }
    }
    m_lock.ReleaseReadLock();

    return lowest;
}

/* create new instance based on template, or a saved instance */
Instance * ClusterMgr::CreateInstance(uint32 InstanceId, uint32 MapId)
{
    /* pick a server for us :) */
    WorkerServer * server = GetWorkerServerForNewInstance();
    if (server == NULL)
        return NULL;

    ASSERT(GetInstance(InstanceId) == NULL);

    /* bump up the max id if necessary */
    if (m_maxInstanceId <= InstanceId)
        m_maxInstanceId = InstanceId + 1;

    Instance * pInstance = new Instance;
    pInstance->InstanceId = InstanceId;
    pInstance->MapId = MapId;
    pInstance->Server = server;

    m_lock.AcquireWriteLock();
    Instances.insert(std::make_pair(InstanceId, pInstance));
    m_lock.ReleaseWriteLock();

    /* tell the actual server to create the instance */
    WorldPacket data(ISMSG_CREATE_INSTANCE, 8);
    data << MapId << InstanceId;
    server->SendPacket(&data);
    server->AddInstance(pInstance);
    LogDebug("ClusterMgr", "Allocating instance %u on map %u to server %u", pInstance->InstanceId, pInstance->MapId, server->GetID());
    return pInstance;
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

    if (Instances.size())
    {
        InstanceMap::iterator itr = Instances.begin();
        while (itr != Instances.end())
        {
            if (itr->second->Server == s)
            {
                LogWarning("ClusterMgr", "Removing instance %u on map %u due to worker server disconnection", itr->first, itr->second->MapId);
                delete itr->second;
                itr = Instances.erase(itr);
            }
            else
                itr++;
        }
        Instances.clear();
    }

    if (InstancedMaps.size())
    {
        std::multimap<uint32, Instance*>::iterator itr1 = InstancedMaps.begin();
        while (itr1 != InstancedMaps.end())
        {
            if (itr1->second->Server == s)
            {
                LogWarning("ClusterMgr", "Removing instance prototype map %u due to worker server disconnection", itr1->first);
                InstancedMaps.erase(itr1++);
            }
            else
                itr1++;
        }
        InstancedMaps.clear();
    }

    for (uint32 i = 0; i < MAX_SINGLE_MAPID; ++i)
    {
        if (SingleInstanceMaps[i])
        {
            if (SingleInstanceMaps[i]->Server == s)
            {
                LogWarning("ClusterMgr", "Removing single map %u due to worker server disconnection", i);
                SingleInstanceMaps[i] = NULL;
            }
        }
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