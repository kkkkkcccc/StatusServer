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
    SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}  //��sqlcnc�̳и�_con
    std::unique_ptr<sql::Connection> _con;
    int64_t _last_oper_time;    //��¼�ϴθ�ɾ�Ĳ��ʱ�䣬�Լ��������ʱ�� 
};

class MySqlPool {
public:
    MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
        : url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
        try {
            for (int i = 0; i < poolSize_; ++i) {
                sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();  //����mysql����
                auto* con = driver->connect(url_, user_, pass_);    //��������mysql
                /*std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));*/
                con->setSchema(schema_);    //�������ݿ�
                //��ȡ��ǰʱ���
                auto currentTime = std::chrono::system_clock::now().time_since_epoch();
                long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
                pool_.push(std::make_unique<SqlConnection>(con, timestamp));    //��SqlConnection�������ʱʹ��make_unique��������ָ�룬�Զ������ڴ�
            }
            _check_thread = std::thread([this]() {  //����һ������߳�
                while (!b_stop_) {  //�߳�δֹͣʱ
                    checkConnection();  //�������
                    std::this_thread::sleep_for(std::chrono::seconds(60));  //ÿ��60s���һ��
                }
                });
            _check_thread.detach(); //��̨����ķ�ʽ���У���������ϵͳ����
        }
        catch (sql::SQLException& e) {
            // �����쳣
            std::cout << "mysql pool init failed" << std::endl;
        }
    }

    void checkConnection() {
        std::lock_guard<std::mutex> guard(mutex_);
        int poolsize = pool_.size();
        //��ȡ��ǰʱ���
        auto currentTime = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();    //ʱ���ת��
        for (int i = 0; i < poolsize; i++) {
            auto con = std::move(pool_.front());    //ȡ����ǰ�ˣ����õ��������ķ�ʽ���б�����֤size��С���䣬������ѭ��
            pool_.pop();
            Defer defer([this, &con]() {    //defer������������ÿ��ѭ��ʱ�ͷţ��ͷ�ʱִ�ж����lamda���ʽ���쳣����ʱҲ�ܱ�֤�̵߳İ�ȫ��
                pool_.push(std::move(con));
                });
            if (timestamp - con->_last_oper_time < 300) {
                continue;   //����ʱ��С��Ԥ���룬��������
            }
            //����ʱ�䲻С��Ԥ���룬ִ��trycatch��
            try {
                std::unique_ptr<sql::Statement> stmt(con->_con->createStatement()); //��������
                stmt->executeQuery("SELECT 1");
                con->_last_oper_time = timestamp;
                std::cout << "execute timer alive query, cur is " << timestamp << std::endl; //�����ѯ���
            }
            catch (sql::SQLException& e) {
                std::cout << "Error keeping connection alive: " << e.what() << std::endl;
                //�쳣�����´������Ӳ��滻�ɵ�����
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
            return !pool_.empty(); });  //�߳�δֹͣʱ���Ϊ�գ��շ���false�����������ڷ���falseʱ���������̻߳���𣬺���Ĵ��벻��ִ��
        if (b_stop_) {
            return nullptr;
        }
        std::unique_ptr<SqlConnection> con(std::move(pool_.front()));   //ȡ���̶߳���ǰ���߳�
        pool_.pop();
        return con;
    }

    void returnConnection(std::unique_ptr<SqlConnection> con) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (b_stop_) {
            return;
        }
        pool_.push(std::move(con)); //������ֵ���ã�ʹ���ƶ����칹�첢ָ��ԭ�ڴ��ַ
        cond_.notify_one(); //����������ȥ֪ͨ����������߳�L87���̳߳��б仯������ȥ����̳߳��Ƿ�Ϊ�գ�Ȼ��ȡ�̲߳���
    }

    void Close() {
        b_stop_ = true;
        cond_.notify_all();     //֪ͨ�����̣߳��̳߳ؼ����ر�
    }

    ~MySqlPool() {
        std::unique_lock<std::mutex> lock(mutex_);  //��ֹ���߳�ͬʱ���ʺ��޸Ĺ�����Դ��ʹ�û��������Ա�֤��������ӳ�ʱ��û�������̷߳������ӳأ��������ݾ���
        while (!pool_.empty()) {
            pool_.pop();
        }
    }

private:
    std::string url_;
    std::string user_;
    std::string pass_;
    std::string schema_;    //���ݿ���
    int poolSize_;
    std::queue<std::unique_ptr<SqlConnection>> pool_; //�̶߳���
    std::mutex mutex_;  //ȷ�����а�ȫ��
    std::condition_variable cond_;  //����Ϊ��ʱ����������
    std::atomic<bool> b_stop_;  //�߳��˳�ʱԭ�ӱ�����true
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

