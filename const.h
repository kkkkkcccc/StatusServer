#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include<memory>	//����ָ��ͷ�ļ�
#include<iostream>
#include"Singleton.h"
#include<functional>
#include<map>
#include<json/json.h>
#include<json/value.h>
#include<json/reader.h>
#include<unordered_map>
#include<boost/filesystem.hpp>
#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/ini_parser.hpp>
#include<atomic>
#include<queue>
#include<mutex>
#include<condition_variable>
#include"hiredis.h"
#include<cassert>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>



namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,	//Json��������
	RPCFailed= 1002,	//RPC�������
	VarifyExpired=1003,	//��֤�����
	VarifyCodeErr=1004,	//��֤�����
	UserExist=1005,	//�û��Ѿ�����
	PasswdErr=1006,	//�������
	EmailNotMatch=1007,	//���䲻ƥ��
	PasswdUpFailed=1008,	//��������ʧ��
	PasswdInvalid=1009,	//�������ʧ��

};

class Defer {
public:
	// ����һ��lamda���ʽ���ߺ���ָ��
	Defer(std::function<void()> func) : func_(func) {}

	// ����������ִ�д���ĺ���
	~Defer() {
		func_();
	}

private:
	std::function<void()> func_;	//�������func��Ϊ��Ա����
};

#define CODEPREFIX  "code_"