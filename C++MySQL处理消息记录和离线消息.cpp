#include <windows.h> //必须在包含mysql.h之前包含windows.h
#include <mysql.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <stdexcept>
#include <unordered_map>
using namespace std;

///消息管理类
class DB_Msg {
protected:
    MYSQL *conn;
    const string tbName; //要操作的表名
    ofstream fsLog; //日志记录文件流
public:
    static string host_;
    static string user_;
    static string passwd_;
    static string db_;
public:
    inline DB_Msg(const string& _tableName, const string& _logFileName);
    inline virtual ~DB_Msg();
    //添加消息
    inline virtual void push(const string& to, const string& from, const string& msg);
    //获取执行指定sql语句后的结果
    inline vector<vector<string> > getBySql(const string& sql);
    //根据指定sql语句删除相应记录
    inline void removeBySql(const string& sql);
    //获取当前日期和时间,用于更新日志
    inline static string getTime();
protected:
    //获取结果集,将其保存到二维string数组中
    inline vector<vector<string> > getResult(MYSQL_RES* res = NULL);
    //创建存储聊天记录的表
    inline void createTable();
};

string DB_Msg::host_ = "localhost";
string DB_Msg::user_ = "root";
string DB_Msg::passwd_ = "123456";
string DB_Msg::db_ = "test";

DB_Msg::DB_Msg(const string& _tableName, const string& _logFileName)
    : tbName(_tableName)
{
    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, host_.c_str(), user_.c_str(), passwd_.c_str(),
                           db_.c_str(), 0, 0, 0) == 0) {
        string err = "Failed to connect database server: ";
        throw logic_error(err + mysql_error(conn));
    }
    mysql_query(conn, "set names 'GBK'");//设置字符集,防止中文出现乱码
    if (_logFileName != "")
        fsLog.open(_logFileName, ios::app);
}

DB_Msg::~DB_Msg()
{
    mysql_close(conn);
    if (fsLog)
        fsLog.close();
}
//添加消息
void DB_Msg::push(const string& to, const string& from, const string& msg)
{
    string sql = "insert into " + tbName + "(toUser,fromUser,msg) values('"
                 + to + "','" + from + "','" + msg + "')";
    if (fsLog)
        fsLog << getTime() << ">>>execute: " + sql << endl;
    if (mysql_query(conn, sql.c_str()) && fsLog) //执行sql语句成功返回0
        fsLog << "Failed to insert: " << mysql_error(conn) << endl;
}
//获取执行指定sql语句后的结果
vector<vector<string> > DB_Msg::getBySql(const string& sql)
{
    vector<vector<string> >result;
    if (fsLog)
        fsLog << getTime() << ">>>execute: " + sql << endl;
    if (mysql_query(conn, sql.c_str()) == 0) {   //执行成功则把结果输出
        result = getResult();
    } else if (fsLog) {
        fsLog << "Failed to search: " << mysql_error(conn) << endl;
    }
    return result;
}
//根据指定sql语句删除相应记录
void DB_Msg::removeBySql(const string& sql)
{
    if (fsLog)
        fsLog << getTime() << ">>>execute: " << sql << endl;
    if (mysql_query(conn, sql.c_str()) && fsLog)
        fsLog << "Failed to delete: " << mysql_error(conn) << endl;
}
//获取当前日期和时间,用于更新日志
inline string DB_Msg::getTime()
{
    char time_buf[64];
    time_t now_time = time(NULL);
    strftime(time_buf, 64, "%Y-%m-%d %H:%M:%S ", localtime(&now_time));
    return time_buf;
}
//获取结果集,将其保存到二维string数组中
vector<vector<string> > DB_Msg::getResult(MYSQL_RES* res)
{
    if (res == NULL) {
        res = mysql_store_result(conn);  //存储查询得到的结果集
    }
    vector<vector<string> >result;
    unsigned num = mysql_num_fields(res); //结果集的列数
    for (MYSQL_ROW row; (row = mysql_fetch_row(res)) != NULL;) {
        result.push_back(vector<string>(num));
        for (int cur = result.size() - 1, i = 0; i < num; ++i) {
            result[cur][i] = row[i];
        }
    }
    mysql_free_result(res); //释放结果集资源
    return result;
}
//创建存储聊天记录的表
void DB_Msg::createTable()
{
    string sql = "CREATE TABLE `" + tbName + "` ("
                 "`id` tinyint(4) NOT NULL AUTO_INCREMENT,"
                 "`toUser` varchar(20) NOT NULL,"
                 "`fromUser` varchar(20) NOT NULL,"
                 "`time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                 "`msg` text NOT NULL,"
                 "PRIMARY KEY (`id`)"
                 ") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
    if (fsLog)
        fsLog << getTime() << ">>>execute: " << sql << endl;
    if (mysql_query(conn, sql.c_str()) && fsLog)
        fsLog << "Failed to create: " << mysql_error(conn) << endl;
}

///离线消息管理类 - 仅用于服务器端
class DB_OfflineMsg : public DB_Msg {
public:
    DB_OfflineMsg(const string& _tableName, const string _logFileName = "")
        : DB_Msg(_tableName, _logFileName)
    { }
    //从离线消息表中得到所有发送给user的消息,提取完后从数据库删除
    inline vector<vector<string> > pop(const string& user);
};

//从离线消息表中得到所有发送给user的消息,提取完后从数据库删除
vector<vector<string> > DB_OfflineMsg::pop(const string& user)
{
    string sql = "select fromUser,time,msg from " + tbName
                 + " where toUser='" + user + "'";
    vector<vector<string> >result = getBySql(sql);
    //删除发给user的所有消息
    if (result.size() > 0) {
        sql = "delete from " + tbName + " where toUser='" + user + "'";
        removeBySql(sql);
    }
    return result;
}

///聊天记录消息管理类
class DB_ChatLogMsg : public DB_Msg {
public:
    inline DB_ChatLogMsg(const string& _userName, const string _logFileName = "");
    //添加聊天记录消息
    inline void push(const string& userOthers, const string& msg, bool isReceived = 1);
    //获取消息user发来的或发给user的所有消息
    inline vector<vector<string> > get(const string& user);
    //在该用户的聊天记录中查找包含str的消息记录
    inline vector<vector<string> > find(const string& str);
    //根据传递的用户名删除与对应用户的聊天记录,传入"*"则删除所有记录
    inline void remove(const string& user);
    //获取与当前用户有聊天记录的用户列表
    inline vector<string> getUserWithChatLog();
protected:
    string userName; //该用户的用户名
};

DB_ChatLogMsg::DB_ChatLogMsg(const string& _userName, const string _logFileName)
    : DB_Msg("chat_log_" + _userName, _logFileName), userName(_userName)
{
    createTable();
}
//添加聊天记录消息
void DB_ChatLogMsg::push(const string& userOthers, const string& msg, bool isReceived)
{
    if (isReceived) { //收到消息
        DB_Msg::push(userName, userOthers, msg);
    } else { //发送消息
        DB_Msg::push(userOthers, userName, msg);
    }
}
//获取消息user发来的或发给user的所有消息
vector<vector<string> > DB_ChatLogMsg::get(const string& user)
{
    string sql = "select fromUser,time,msg from " + tbName +
                 " where toUser='" + user + "' or fromUser='" + user + "'";
    return getBySql(sql);
}
//在该用户的聊天记录中查找包含str的消息记录
vector<vector<string> > DB_ChatLogMsg::find(const string& str)
{
    string sql = "select fromUser,time,msg from " + tbName
                 + " where (toUser='" + userName + "' or fromUser='"
                 + userName + "') and msg like '%" + str + "%'";
    return getBySql(sql);
}
//根据传递的用户名删除与用户user的聊天记录,传入"*"则删除所有记录
void DB_ChatLogMsg::remove(const string& user)
{
    string sql = "delete from " + tbName;
    if (user != "*")
        sql += " where toUser='" + user + "' or fromUser='" + user + "'";
    removeBySql(sql);
}
//获取与当前用户有聊天记录的用户列表
vector<string> DB_ChatLogMsg::getUserWithChatLog()
{
    string sql = "select distinct toUser from " + tbName;
    auto users = getBySql(sql);
    unordered_map<string, bool> userList;
    for (auto &elem : users)
        userList[elem[0]];
    sql = "select distinct fromUser from " + tbName;
    users = getBySql(sql);
    for (auto &elem : users)
        userList[elem[0]];
    userList.erase(userList.find(userName)); //删除用户列表中的自己
    //将结果保存到vector<string>中返回
    vector<string> res;
    res.reserve(userList.size());
    for (auto &it : userList)
        res.push_back(it.first);
    return res;
}


ostream& operator<<(ostream& out, const vector<vector<string> >& res)
{
    if (res.size() == 0) {
        cout << "暂无内容可显示!" << endl;
        return out;
    }
    for (auto &it : res) { //显示提取出的离线消息内容
        for (auto &elem : it)
            cout << elem << " ";
        cout << endl;
    }
    return out;
}

int main()
{
    DB_OfflineMsg off_msg("offline_msg", "offileMsg.log");
    off_msg.push("to", "from", "hey");   //存入一条离线消息
    auto res = off_msg.pop("to"); //读取发给该用户的全部离线消息
    cout << res << endl;

    DB_ChatLogMsg clMsg("webary", "chat_log_webary.log");
    clMsg.push("Miranda-lym", "hello", 0);
    clMsg.push("Miranda-lym", "hi!", 1);
    clMsg.push("Miranda-lym", "this a test case!", 0);
    clMsg.push("Miranda-lym", "soga,I get it!", 1);
    clMsg.push("John", "what are you doing!", 1);
    cout << clMsg.get("Miranda-lym") << endl;

    cout << clMsg.find("hi") << endl;

    auto userList = clMsg.getUserWithChatLog();
    for (auto &elem : userList)
        cout << elem << endl;
    cin.get();
    return 0;
}
