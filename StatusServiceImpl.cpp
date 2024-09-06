#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include <mutex>
#include"RedisMgr.h"

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
    insertToken(request->uid(), reply->token());    //����uid��token��_tokiens��������֤
    return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
    auto uid = request->uid();
    auto token = request->token();
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (success) {
        reply->set_error(ErrorCodes::UidInvalid);
        return Status::OK;
    }

    if (token_value != token) {
        reply->set_error(ErrorCodes::TokenInvalid);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    return Status::OK;
}

//������������͵�����id��token
void StatusServiceImpl::insertToken(int uid, std::string token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}

//Ѱ�Ҹ�����С������
ChatServer StatusServiceImpl::getChatServer()
{
    std::lock_guard<std::mutex> guard(_server_mtx);
    auto minServer = _servers.begin()->second;
    auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
    if (count_str.empty()) {
        //��������Ĭ������Ϊ���
        minServer.con_count = INT_MAX;
    }
    else {
        minServer.con_count = std::stoi(count_str);
    }


    // ʹ�÷�Χ����forѭ��
    for (auto& server : _servers) {

        if (server.second.name == minServer.name) {
            continue;
        }

        auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
        if (count_str.empty()) {
            server.second.con_count = INT_MAX;
        }
        else {
            server.second.con_count = std::stoi(count_str);
        }

        if (server.second.con_count < minServer.con_count) {
            minServer = server.second;
        }
    }

    return minServer;
}

//��ȡ����
StatusServiceImpl::StatusServiceImpl()
{   ////ֱ����ȡ���� 
    //auto& cfg = ConfigMgr::Inst();
    //ChatServer server;
    //server.port = cfg["ChatServer1"]["Port"];
    //server.host = cfg["ChatServer1"]["Host"];
    //server.con_count = 0;
    //server.name = cfg["ChatServer1"]["Name"];
    //_servers[server.name]=server;

    //server.port = cfg["ChatServer2"]["Port"];
    //server.host = cfg["ChatServer2"]["Host"];
    //server.con_count = 0;
    //server.name = cfg["ChatServer2"]["Name"];
    //_servers[server.name] = server;

    auto& cfg = ConfigMgr::Inst();
    auto server_list = cfg["chatservers"]["Name"];

    std::vector<std::string> words;

    std::stringstream ss(server_list);
    std::string word;

    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }

    for (auto& word : words) {
        if (cfg[word]["Name"].empty()) {
            continue;
        }

        ChatServer server;
        server.port = cfg[word]["Port"];
        server.host = cfg[word]["Host"];
        server.name = cfg[word]["Name"];
        _servers[server.name] = server;
    }
}