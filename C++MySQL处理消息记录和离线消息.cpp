#include <windows.h> //必须在包含mysql.h之前包含windows.h
#include <mysql.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
using namespace std;

class DB_Msg {
protected:
    MYSQL *conn;
    const string tbName; //要操作的表名
    ofstream fsLog; //日志记录文件流
public:
    static string host;
    static string user;
    static string passwd;
    static string db;
public:
    DB_Msg(const string& _tableName, const string& _logFileName);
    ~DB_Msg();
    //添加消息
    void push(const string& to, const string& from, const string& msg);
    //获取消息
    vector<vector<string> > get(const string& user);
    //删除user的所有消息
    void remove(const string& user);
protected:
    //获取结果集
    vector<vector<string> > getResult(MYSQL_RES* res = NULL);
};

string DB_Msg::host = "localhost";
string DB_Msg::user = "root";
string DB_Msg::passwd = "123456";
string DB_Msg::db = "test";

DB_Msg::DB_Msg(const string& _tableName, const string& _logFileName)
    : tbName(_tableName)
{
    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, host.c_str(), user.c_str(), passwd.c_str(),
                           db.c_str(), 0, 0, 0) == 0) {
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
        fsLog << "\n>>>execute: " + sql << endl;
    if (mysql_query(conn, sql.c_str()) && fsLog) //执行sql语句成功返回0
        fsLog << "Failed to insert: " << mysql_error(conn) << endl;
}
//获取消息
vector<vector<string> > DB_Msg::get(const string& user)
{
    vector<vector<string> >result;
    string sql = "select fromUser,time,msg from " + tbName
                 + " where toUser='" + user + "'";
    if (fsLog)
        fsLog << "\n>>>execute: " + sql << endl;
    if (mysql_query(conn, sql.c_str()) == 0) {   //执行成功则把结果输出
        result = getResult();
    } else {
        if (fsLog)
            fsLog << "Failed to search: " << mysql_error(conn) << endl;
    }
    return result;
}
//删除user的所有消息
void DB_Msg::remove(const string& user)
{
    string sql = "delete from " + tbName + " where toUser='" + user + "'";
    if (fsLog)
        fsLog << ">>>execute: " << sql << endl;
    if (mysql_query(conn, sql.c_str())) //执行sql语句
        if (fsLog)
            fsLog << "Failed to delete: " << mysql_error(conn) << endl;
}
//获取结果集
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

///离线消息管理类
class DB_OfflineMsg : public DB_Msg {
public:
    DB_OfflineMsg(const string& _tableName, const string _logFileName = "")
        : DB_Msg(_tableName, _logFileName)
    { }
    //提取离线消息,提取完后从数据库删除
    vector<vector<string> > pop(const string& user)
    {
        vector<vector<string> >result = get(user);
        if (result.size() > 0) {
            remove(user);
        }
        return result;
    }
};

///聊天记录管理类
class DB_ChatLogMsg : public DB_Msg {
public:
    DB_ChatLogMsg(const string& _tableName, const string _logFileName = "")
        : DB_Msg(_tableName, _logFileName)
    {
        createTable();
    }
protected:
    //创建存储聊天记录的表
    void createTable()
    {
        string sql = "CREATE TABLE `chat_log` ("
                     "`id` tinyint(4) NOT NULL AUTO_INCREMENT,"
                     "`toUser` varchar(20) NOT NULL,"
                     "`fromUser` varchar(20) NOT NULL,"
                     "`time` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                     "`msg` text NOT NULL,"
                     "PRIMARY KEY (`id`)"
                     ") ENGINE=InnoDB DEFAULT CHARSET=utf8;";
        if (fsLog)
            fsLog << ">>>execute: " << sql << endl;
        if (mysql_query(conn, sql.c_str()) && fsLog)
            fsLog << "Failed to create: " << mysql_error(conn) << endl;
    }
};

ostream& operator<<(ostream& out, const vector<vector<string> >& res)
{
    for (auto it : res) { //显示提取出的离线消息内容
        for (auto elem : it)
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

    DB_ChatLogMsg clMsg("chat_log");
    clMsg.push("webary", "Miranda-lym", "hello");
    clMsg.push("Miranda-lym", "webary", "hi!");
    clMsg.push("webary", "Miranda-lym", "this a test case!");
    cout << clMsg.get("webary") << endl;

    cin.get();
    return 0;
}
