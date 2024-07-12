#pragma once
#include"const.h"
#include<thread>
#include<jdbc/mysql_driver.h>
#include<jdbc/mysql_connection.h>
#include<jdbc/cppconn/prepared_statement.h>
#include<jdbc/cppconn/resultset.h>
#include<jdbc/cppconn/statement.h>
#include<jdbc/cppconn/exception.h>

class SqlConnection {
public:
    SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}  //将sqlcnc继承给_con
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_oper_time;    //记录上次更删改查的时间，以计算最长空闲时长 
};

class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
        try {
            for (int i = 0; i < poolSize_; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();  //创建mysql驱动
                auto* con = driver->connect(url_, user_, pass_);    //驱动连接mysql
                /*std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));*/
                con->setSchema(schema_);    //设置数据库
                //获取当前时间戳
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(con, timestamp));    //将SqlConnection推入队列时使用make_unique创建智能指针，自动管理内存
            }
            _check_thread = std::thread([this]() {  //启动一个检测线程
                while (!b_stop_) {  //线程未停止时
                    checkConnection();  //检测连接
                    std::this_thread::sleep_for(std::chrono::seconds(60));  //每隔60s检测一次
                }
                });
            _check_thread.detach(); //后台分离的方式运行，交给操作系统回收
        }
        catch (sql::SQLException& e) {
            // 处理异常
            std::cout << "mysql pool init failed" << std::endl;
        }
    }

    void checkConnection() {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        //获取当前时间戳
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();    //时间戳转秒
        for (int i = 0; i < poolsize; i++) {
            auto con = std::move(pool_.front());    //取队列前端，采用弹出塞进的方式进行遍历保证size大小不变，避免死循环
            pool_.pop();
            Defer defer([this, &con]() {    //defer的析构函数在每次循环时释放，释放时执行定义的lamda表达式，异常运行时也能保证线程的安全性
                pool_.push(std::move(con));
                });
            if (timestamp - con->_last_oper_time < 300) {
                continue;   //操作时间小于预定秒，继续遍历
            }
            //操作时间不小于预定秒，执行trycatch块
            try {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement()); //创建声明
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
                std::cout << "execute timer alive query, cur is " << timestamp << std::endl; //输出查询结果
            }
            catch (sql::SQLException& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                //异常后，重新创建连接并替换旧的连接
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
                auto* newcon = driver->connect(url_,user_,pass_);
                newcon->setSchema(schema_);
                con->_con.reset(newcon);
                con->_last_oper_time = timestamp;
            }
        }
    }

    std::unique_ptr<SqlConnection> getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this] {
            if (b_stop_) {
                return true;
            }
            return !pool_.empty(); });  //线程未停止时检测为空，空返回false，条件变量在返回false时访问它的线程会挂起，后面的代码不会执行
        if (b_stop_) {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));   //取出线程队列前部线程
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con)); //返回右值引用，使用移动构造构造并指向原内存地址
        cond_.notify_one(); //用条件变量去通知其他挂起的线程L87：线程池有变化，可以去检测线程池是否为空，然后取线程操作
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();     //通知所有线程，线程池即将关闭
    }

    ~MySqlPool() {
        std::unique_lock<std::mutex> lock(mutex_);  //防止多线程同时访问和修改共享资源，使用互斥锁可以保证在清空连接池时，没有其他线程访问连接池，引起数据竞争
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;    //数据库名
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_; //线程队列
    std::mutex mutex_;  //确保队列安全性
    std::condition_variable cond_;  //队列为空时的条件变量
    std::atomic<bool> b_stop_;  //线程退出时原子变量置true
    std::thread _check_thread;
};

struct UserInfo {
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
};

class MysqlDao
{
public:
    MysqlDao();
    ~MysqlDao();
    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& newpwd);
    bool CheckPwd(const std::string& name, const std::string& pwd,UserInfo& userInfo);
private:
    std::unique_ptr<MySqlPool> pool_;
};

