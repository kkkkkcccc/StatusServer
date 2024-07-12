#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"
class AsioIOServicePool:public Singleton<AsioIOServicePool>	//�̳߳ؼ̳е�������
{
	friend Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using Work = boost::asio::io_context::work;	//work��io_contextʱ����ʹû��ִ��Ҳ��������
	using WorkPtr = std::unique_ptr<Work>;
	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
	// ʹ�� round-robin �ķ�ʽ����һ�� io_service
	boost::asio::io_context& GetIOService();
	void Stop();	//֪ͨ��Դ�����ѹһ����߳�
private:
	AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);	//ע����CPU����������������߳�����2
	std::vector<IOService> _ioServices; //io��û�м����κ��¼�������»�ֱ�ӷ��أ����������߳�
	std::vector<WorkPtr> _works;	//workptr������io_context����
	std::vector<std::thread> _threads;	//io_context�������߳�����
	std::size_t                        _nextIOService;
};

