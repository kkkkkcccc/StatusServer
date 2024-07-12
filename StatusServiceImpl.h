#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

struct ChatServer {
    std::string host;
    std::string port;
    std::string name;
    int con_count;
};
class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
        GetChatServerRsp* reply) override;
    /*Status Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)override;*/
private:
    void insertToken(int uid, std::string token);
    std::unordered_map<std::string,ChatServer> _servers;
    ChatServer getChatServer();
    std::mutex _server_mtx;
    std::unordered_map<int, std::string> _tokens;
    std::mutex _token_mtx;
};