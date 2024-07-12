#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size),	//不支持拷贝的类要通过初始化列表初始化
_works(size), _nextIOService(0){
	for (std::size_t i = 0; i < size; ++i) {
		_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));	//创建size个work对象的指针，并绑定ioservice，保证每个指针在没有绑定事件的时候也不会退出
	}

	//遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
	for (std::size_t i = 0; i < _ioServices.size(); ++i) {
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}

AsioIOServicePool::~AsioIOServicePool() {
	Stop();	//确保所有线程都全部退出再回收
	std::cout << "AsioIOServicePool destruct" << endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++];
	if (_nextIOService == _ioServices.size()) {
		_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::Stop(){
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	for (auto& work : _works) {
		//把服务先停止
		work->get_io_context().stop();
		work.reset();	//变为空指针并回收context
	}

	for (auto& t : _threads) {
		t.join();
	}
}
