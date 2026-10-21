#ifndef __GATEWAY_MGR_H__
#define __GATEWAY_MGR_H__

#include "nskernel/connection.h"
#include <bson.h>
#include "base/lock_common.h"
#include "base/log.h"
#include "errorcode.h"

using namespace bson;

class LoginServer;
class DBMgr;

#define MAX_LOAD_PER_GS 2000000
#define NO_VALID_SERVER_LINK -1
//#include "pusermgr.h"
//class string;
#include <map>
#include <string>
#include "base/lock_common.h"

using namespace std;


class Connection;
class LoginServer;

struct GateWayServerInfo 
{
    long last_heartbeat;
    int load;
    int max_load;
    string address;
    Connection *pConn;
	long  register_time;   //当前server的创建时间
	int svrId; //服务器id
	string svrIp; //服务器ip
	int port; //服务端口

    std::set<int> gstypes;    //玩法类型
    std::string gsstr;
    GateWayServerInfo() 
	{
        last_heartbeat  = time(NULL);
        load  = 0;
        max_load = 0;
        pConn = NULL;
		register_time = -1;
		
    };
    bool can_dispatch() 
	{
        //return (max_load >0 && load < max_load);
        return true;
    };
    void update_load(long timestamp, int load_value) 
	{
        last_heartbeat = timestamp;
        load           = load_value;
    };
};

/*
* 记录网关负载信息，给客户端分配网关ip端口
*/

class GateWayServerMgr 
{
private:
    map<int, GateWayServerInfo> m_mapFd2GwSvrInfo; //<fd, struct GateWayServerInfo>
    RWLOCK m_lock;
    //PUserMgr * p_pumgr;
    //LoginServer *p_ls;
    LoginServer* m_pLoginServer;
    int min_load_gs_fd; //当前负载最小的服务器
    map<int,int>  m_mapSvrId2UsrCnt; //key是网关的serverid，value是用户数，即负载
    map<int,string> m_mapSvrId2IpPort; //key是网关的serverid，value是网关的ip端口字符串：ip:port
    map<int,int> m_mapFd2SvrId;
    int m_nMinLoadSvrId; //当前最小负载的网关id
    int m_nMinLoadSvrFd; //当前最小负载的网关Fd
    
    void update_min_load();
public:
    GateWayServerMgr() {};
    ~GateWayServerMgr();
    void init(LoginServer *p_loginserver);
    //注册新的GameServer
    void registserver(string ip, int port, int svrid, long timestamp, Connection *pConn);
    //根据时间戳更新负载
    bool updateserver(long timestamp, int load, Connection *pConn, int nSvrId);
    //删除一个GameServer
    void delete_gameserver(int fd);
    //根据FD获取一个Server
    string getgs_by_fd(int fd);
    // 获取当前负载最小的游戏服务器的FD
    int get_minload_gs(); 
    // 根据服务器地址获取链接
    Connection* get_gs_connection(string &address);
    // 扫描处理过心跳期的服务器
    void linkscanner(int heart_beat);
    // 判断一个服务器是否可以分派
    bool can_dispatch(int fd);
    //获取当前负载
    int getTotalLoad();

     //顺序分配
     //返回最初分配的可用serverid (按照registtime先后顺序分配)
	string getLatestGs();
    void getGsList(std::vector<GateWayServerInfo*>& svrs);
    void getGsByAdress(const string& address, GateWayServerInfo* pGS);
};

#endif


