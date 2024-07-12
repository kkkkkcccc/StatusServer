#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include <mutex>

//生成随机token码
std::string generate_unique_string() {
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);

    return unique_string;
}

//获取聊天服务
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
    std::string prefix("kkcc status server has received :  ");
    const auto& server = getChatServer();
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(generate_unique_string()); //设置IP、port、错误提示、token码给Reply
    insertToken(request->uid(), reply->token());    //缓存uid和token用于验证
    return Status::OK;
}

//缓存服务器发送的请求id和token
void StatusServiceImpl::insertToken(int uid, std::string token)
{
    std::lock_guard<std::mutex> guard(_token_mtx);
    _tokens[uid] = token;   //把uid和token缓存进map，下次服务时校验
}

//寻找负载最小服务器
ChatServer StatusServiceImpl::getChatServer()
{
    std::lock_guard<std::mutex> guard(_server_mtx); //添加服务锁
    auto minServer = _servers.begin()->second;  //获取一个服务
    for (const auto& server : _servers) {
        if (server.second.con_count < minServer.con_count) {    //寻找连接数最少的一个服务
            minServer = server.second;
        }
    }
    return minServer;
}

StatusServiceImpl::StatusServiceImpl()
{
    auto& cfg = ConfigMgr::Inst();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    server.con_count = 0;
    server.name = cfg["ChatServer1"]["Name"];
    _servers[server.name]=server;

    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    server.con_count = 0;
    server.name = cfg["ChatServer2"]["Name"];
    _servers[server.name] = server;
}