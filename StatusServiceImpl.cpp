#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include <mutex>

//�������token��
std::string generate_unique_string() {
    // ����UUID����
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // ��UUIDת��Ϊ�ַ���
    std::string unique_string = to_string(uuid);

    return unique_string;
}

//��ȡ�������
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
    std::string prefix("kkcc status server has received :  ");
    const auto& server = getChatServer();
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(generate_unique_string()); //����IP��port��������ʾ��token���Reply
    insertToken(request->uid(), reply->token());    //����uid��token������֤
    return Status::OK;
}

//������������͵�����id��token
void StatusServiceImpl::insertToken(int uid, std::string token)
{
    std::lock_guard<std::mutex> guard(_token_mtx);
    _tokens[uid] = token;   //��uid��token�����map���´η���ʱУ��
}

//Ѱ�Ҹ�����С������
ChatServer StatusServiceImpl::getChatServer()
{
    std::lock_guard<std::mutex> guard(_server_mtx); //��ӷ�����
    auto minServer = _servers.begin()->second;  //��ȡһ������
    for (const auto& server : _servers) {
        if (server.second.con_count < minServer.con_count) {    //Ѱ�����������ٵ�һ������
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