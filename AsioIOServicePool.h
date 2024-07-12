#pragma once
#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"
class AsioIOServicePool:public Singleton<AsioIOServicePool>	//线程池继承单例基类
{
	friend Singleton<AsioIOServicePool>;
public:
	using IOService = boost::asio::io_context;
	using Work = boost::asio::io_context::work;	//work绑定io_context时，即使没有执行也不会析构
	using WorkPtr = std::unique_ptr<Work>;
	~AsioIOServicePool();
	AsioIOServicePool(const AsioIOServicePool&) = delete;
	AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
	// 使用 round-robin 的方式返回一个 io_service
	boost::asio::io_context& GetIOService();
	void Stop();	//通知资源并唤醒挂机的线程
private:
	AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);	//注释了CPU核数参数，添加了线程数量2
	std::vector<IOService> _ioServices; //io组没有监听任何事件的情况下会直接返回，不会阻塞线程
	std::vector<WorkPtr> _works;	//workptr数量→io_context数量
	std::vector<std::thread> _threads;	//io_context数量→线程数量
	std::size_t                        _nextIOService;
};

