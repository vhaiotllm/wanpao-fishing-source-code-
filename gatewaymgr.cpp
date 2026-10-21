#include "base/utils.h"
#include "nskernel/connection.h"
#include "gatewaymgr.h"
#include "loginserver.h"

void GateWayServerMgr::init(LoginServer *p_loginserver) {
    assert(p_loginserver);
    m_pLoginServer = p_loginserver;
    m_nMinLoadSvrFd = NO_VALID_SERVER_LINK;
    DORW_LOCK_INIT(m_lock);
}

GateWayServerMgr::~GateWayServerMgr() {
    DORW_LOCK_DESTROY(m_lock);
}

void GateWayServerMgr::update_min_load() 
{ //private function. called internally. no lock needed!
    if(m_mapFd2GwSvrInfo.size() <= 0) 
    {
        m_pLoginServer->m_pRollLog->debug("info: GateWayServerMgr::update_min_load. empty gameserver list, so min_load_gs fd=-1");
        m_nMinLoadSvrFd = NO_VALID_SERVER_LINK;
        return;
    }
    map<int, GateWayServerInfo>::iterator it;
    int min_load = MAX_LOAD_PER_GS;
    for(it = m_mapFd2GwSvrInfo.begin(); it!= m_mapFd2GwSvrInfo.end(); it++)
     {
        if(min_load > (it->second).load && (it->second).can_dispatch()) 
        {
            min_load = (it->second).load;
            m_nMinLoadSvrFd = it->first;
        }
    }
}

void GateWayServerMgr::linkscanner(int heart_beat) {
    long curr_time = time(NULL);
    vector<int> vecDeadSvrFd;
    map<int, GateWayServerInfo>::iterator itFd2SvrInfo;
   // cout<<"linkscanner running! time="<<time(NULL)<<endl;
    DORW_WRLOCK(m_lock);
    for(itFd2SvrInfo = m_mapFd2GwSvrInfo.begin(); itFd2SvrInfo != m_mapFd2GwSvrInfo.end(); itFd2SvrInfo++) 
    {
        if((itFd2SvrInfo->second).last_heartbeat + heart_beat < curr_time) 
        {
          //这个连接已经死亡
            vecDeadSvrFd.push_back(itFd2SvrInfo->first);
            m_pLoginServer->m_pRollLog->error("critical: GateWayServerMgr::linkscanner. server=%s failed to timely send heart beat. so connection lost!",
                    (itFd2SvrInfo->second).address.c_str());
            try 
            {
                (itFd2SvrInfo->second).pConn->close();
            } 
            catch(std::exception& e) 
            {
                m_pLoginServer->m_pRollLog->error("critical: GateWayServerMgr::linkscanner. server=%s; fd=%d. close exception=%s!",
                        (itFd2SvrInfo->second).address.c_str(),(itFd2SvrInfo->second).pConn->fd(), e.what());
            } 
            catch(...) 
            {
                m_pLoginServer->m_pRollLog->error("GateWayServerMgr::linkscanner. unknown exception!");
            }
        }
    }
    if(vecDeadSvrFd.size() > 0) 
    {
        m_pLoginServer->m_pRollLog->debug("info: GateWayServerMgr::linkscanner. delete invalid gs link number=%d", vecDeadSvrFd.size());
        vector<int>::iterator it;
        for(it = vecDeadSvrFd.begin(); it != vecDeadSvrFd.end(); it++)
        {
            m_mapFd2GwSvrInfo.erase(*it);
        }
        update_min_load(); 
    }
    DORW_UNLOCK(m_lock);
}

void GateWayServerMgr::registserver(string ip, int port, int svrid, long timestamp, Connection *pConn) 
{
    if(pConn == NULL || !pConn->valid()) 
	{ 
	    //链接要有效
        m_pLoginServer->m_pRollLog->error("error: GateWayServerMgr::registserver. ip=%s. port=%d. regist gameserver. invalid link!",
                ip.c_str(),port);
        return;
    }
    m_pLoginServer->m_pRollLog->debug("info: GateWayServerMgr::registserver. ip=%s. port=%d. svrid=%d, time_stamp=%d, conn=%d",
            ip.c_str(),port, svrid, timestamp, pConn->fd());

    int link_fd = pConn->fd();
    GateWayServerInfo svrInfo;
    svrInfo.last_heartbeat = timestamp;
    svrInfo.svrIp        = ip;
    svrInfo.port          = port;
    svrInfo.svrId       = svrid;
    svrInfo.load           = 0;
    svrInfo.pConn          = pConn;
	svrInfo.register_time = time(NULL);
    //cout<<"regist link fd="<<link_fd<<endl;
    DORW_WRLOCK(m_lock);
    m_mapFd2GwSvrInfo[link_fd]   = svrInfo;
    update_min_load();
    DORW_UNLOCK(m_lock);
}

bool GateWayServerMgr::updateserver(long timestamp, int load, Connection *pConn, int nSvrId) {
    if(pConn == NULL || !pConn->valid()) {
        m_pLoginServer->m_pRollLog->error("error: GateWayServerMgr::updateserver. invalid link!");
        return false;
    }
    //linkscanner(); //清理心跳超时的gameserver注册
    bool update_result = true;
    int link_fd = pConn->fd();
    //cout<<"link_fd="<<link_fd<<endl;
    DORW_WRLOCK(m_lock);
    do {
        if(m_mapFd2GwSvrInfo.find(link_fd) != m_mapFd2GwSvrInfo.end()) 
        {
            if(load < 0) //心跳包不带负载，只更新时间
            {
                load = m_mapFd2GwSvrInfo[link_fd].load;
            }
            m_mapFd2GwSvrInfo[link_fd].update_load(timestamp, load);
            update_min_load();
        } 
        else 
        {
            m_pLoginServer->m_pRollLog->error("error: GateWayServerMgr::updateserver. link is not registed now!");
            update_result = false;
            break;
        } 
    } while(false);
    DORW_UNLOCK(m_lock);
	if(!update_result)
	{
		registserver(pConn->getIPStr(), pConn->getPort(), nSvrId, timestamp, pConn);
	}
    return update_result;
}

void GateWayServerMgr::delete_gameserver(int fd) {
    DORW_WRLOCK(m_lock);
    if(m_mapFd2GwSvrInfo.find(fd) != m_mapFd2GwSvrInfo.end()) {
        m_mapFd2GwSvrInfo.erase(fd);
        update_min_load();
    }
    DORW_UNLOCK(m_lock);
}

string GateWayServerMgr::getgs_by_fd(int fd) {
    string gameserver_addr = "";
    DORW_RDLOCK(m_lock);
    if(m_mapFd2GwSvrInfo.find(fd) != m_mapFd2GwSvrInfo.end())
        gameserver_addr = m_mapFd2GwSvrInfo.find(fd)->second.address;
    DORW_UNLOCK(m_lock);
    return gameserver_addr;
}


string GateWayServerMgr::getLatestGs()
{
   
    string strAddr;
	long nFistRegisterTime = 0;
    DORW_RDLOCK(m_lock);
    for( map<int, GateWayServerInfo>::iterator iter  =m_mapFd2GwSvrInfo.begin(); iter != m_mapFd2GwSvrInfo.end(); iter++) 
	{
        if (iter->second.register_time == -1)
		{
		    continue;
		}
		if (iter->second.can_dispatch())
		{
            //第一次
			if (nFistRegisterTime == 0)
			{
			    nFistRegisterTime = iter->second.register_time;
				strAddr =  iter->second.address;
				continue;
			}		
			if (nFistRegisterTime < iter->second.register_time)
			{
		        continue;
			}
			nFistRegisterTime = iter->second.register_time;
			strAddr =  iter->second.address;
		}

    }
    DORW_UNLOCK(m_lock);
	m_pLoginServer->m_pRollLog->debug("GateWayServerMgr::getLatestGs: %s dispatch,time:%ld", strAddr.c_str(),nFistRegisterTime);
    return strAddr;  
}

void GateWayServerMgr::getGsList(std::vector<GateWayServerInfo*>& svrs)
{
    DORW_RDLOCK(m_lock);
    for( map<int, GateWayServerInfo>::iterator iter = m_mapFd2GwSvrInfo.begin(); iter != m_mapFd2GwSvrInfo.end(); iter++) 
    {
        if (iter->second.register_time == -1 || iter->second.svrIp == "" || iter->second.svrId <= 0)
        {
            continue;
        }
        if (iter->second.can_dispatch())
        {
            svrs.push_back(&iter->second);
        }

    }
    DORW_UNLOCK(m_lock);
}

void GateWayServerMgr::getGsByAdress(const string& address, GateWayServerInfo* pGs)
{
    DORW_RDLOCK(m_lock);
    for( map<int, GateWayServerInfo>::iterator iter = m_mapFd2GwSvrInfo.begin(); iter != m_mapFd2GwSvrInfo.end(); iter++) 
    {
        if (iter->second.address == address)
        {
            pGs = &iter->second;
            break;
        }

    }
    DORW_UNLOCK(m_lock);
}


int GateWayServerMgr::get_minload_gs()
{
    //返回负载最低的服务器
    return m_nMinLoadSvrFd;
}

Connection* GateWayServerMgr::get_gs_connection(string &address) {
    map<int, GateWayServerInfo>::iterator it;
    Connection* target_conn = NULL;
    DORW_RDLOCK(m_lock);
    for(it=m_mapFd2GwSvrInfo.begin(); it != m_mapFd2GwSvrInfo.end(); it++) {
        if((it->second).address == address) {
            target_conn = (it->second).pConn;
            break;
        }
    }
    DORW_UNLOCK(m_lock);
    return target_conn;
}

bool GateWayServerMgr::can_dispatch(int fd) {
    bool can_dispatch = true;
    DORW_RDLOCK(m_lock);
    do {
        if(m_mapFd2GwSvrInfo.find(fd) == m_mapFd2GwSvrInfo.end()) { //已经无效了
            can_dispatch =false;
            m_pLoginServer->m_pRollLog->error("error: GateWayServerMgr::can_dispatch. the server fd=%d is not registed. so can not dispatch!", fd);
            break;
        }
        if(m_mapFd2GwSvrInfo[fd].can_dispatch() == false) { //已经满员了
            can_dispatch =false;
            m_pLoginServer->m_pRollLog->error("error: GateWayServerMgr::can_dispatch. the server fd=%d can not dispatch. maybe too much users!", fd);
            break;
        }
    } while(false);
    DORW_UNLOCK(m_lock);
    return can_dispatch;
}

int GateWayServerMgr::getTotalLoad() {
    int total_load = 0;
    map<int, GateWayServerInfo>::iterator it;
    DORW_RDLOCK(m_lock);
    for(it = m_mapFd2GwSvrInfo.begin(); it != m_mapFd2GwSvrInfo.end(); it++)
    {
        total_load += (it->second).load;
    }
    DORW_UNLOCK(m_lock);
    return total_load;
}
