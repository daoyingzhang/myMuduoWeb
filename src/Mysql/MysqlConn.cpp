#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
    // 分配或初始化与mysql_real_connect()相适应的MYSQL对象
    // 如果mysql是NULL指针，该函数将分配、初始化、并返回新对象
    conn_=mysql_init(nullptr);
    // 设置字符编码，可以储存中文
    mysql_set_character_set(conn_,"utf8");
}

MysqlConn::~MysqlConn()
{
    if(conn_!=nullptr){
        mysql_close(conn_);
    }
    freeResult();
}

// 连接数据库
bool MysqlConn::connect(const std::string &user, const std::string &pwd,
                        const std::string &dbName, const std::string &localHost,
                        const uint16_t port)
{
    MYSQL* connptr=mysql_real_connect(conn_,localHost.c_str(),user.c_str(),pwd.c_str(),dbName.c_str(),static_cast<unsigned int>(port),nullptr,0);
    return connptr!=nullptr;
}

// 更新数据库包括插入、删除、更新
bool MysqlConn::update(const char* sql)
{
    // mysql_query支持查询、删除、更新、表结构更改,执行成功返回 0   对于整个函数来说查询成功返回true，下边的quary也是如此。
    if(mysql_query(conn_,sql)){
        return false;
    }
    return true;
}
// 查询
bool MysqlConn::query(const char* sql)
{
    freeResult();
    if(mysql_query(conn_,sql)){
        return false;
    }
    //存储结果 结果是一个二重指针
    result_=mysql_store_result(conn_);
    return true;
}

// 遍历查询到的结果集,每次查一行
bool MysqlConn::next()
{
    if(result_!=nullptr){
        row_=mysql_fetch_row(result_);
        if(row_!=nullptr){
            return true;
        }
    }
    return false;
}
// 得到结果集的字段值
std::string MysqlConn::value(int index){
    int row_index=mysql_num_fields(result_);
    if(index>=row_index || index < 0){
        return std::string();
    }
    //因为如果存的是二进制的，其中有可能包含 ‘\0’
    //那么就可能无法获得完整的，因此需要获得字符串的头指针和字符串长度。
    char* val=row_[index];
    unsigned long length=mysql_fetch_lengths(result_)[index];

    return std::string(val,length);
}

// 事务操作
bool MysqlConn::transaction(){

    //true 自动提交
    //false 手动提交
    return mysql_autocommit(conn_,false);
}
// 提交事务
bool MysqlConn::ccommit(){
    return mysql_commit(conn_);
}
// 事务回滚
bool MysqlConn::rollback(){
    return mysql_rollback(conn_);
}

// 刷新起始的空闲时间点
void MysqlConn::refreshAliveTime(){
    //获取时间戳
    m_alivetime=std::chrono::steady_clock::now();
}
// 计算连接的存活时长。
long long MysqlConn::getAliveTime(){

    std::chrono::nanoseconds res=std::chrono::steady_clock::now()-m_alivetime;
    // 纳秒 -> 毫秒，高精度向低精度转换需要duration_cast
    std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
    // 返回毫秒数量
    return millsec.count();
}

//清空结果集，查询或者什么后数据是要保存在结果集中的
void MysqlConn::freeResult(){
    if(result_){
        mysql_free_result(result_);
        result_=nullptr;
    }
}