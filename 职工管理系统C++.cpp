#include <windows.h>
#include <mysql.h>
#include <conio.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
using namespace std;
class DB_worker {
private:
    char column[20][30];    //存储属性名称
    FILE *fplog;            //存储日志记录
    char info[10][50];      //临时存放当前键入的属性值
    MYSQL mysql, *sock;     //mysql连接
    MYSQL_RES *res;         //这个结构代表返回行的一个查询结果集
    int num_fields;         //结果集中的列数
    char query[1024];       //查询语句
    char tableName[20];     //表名

    bool QueryDatabase(const char*s = 0, int show = 1, const char* todo = "select *", int saveToFile = 0);
    bool ConnectDatabase();
    void printDateTime(FILE * fp);
public:
    DB_worker(const char* tb = "stu");
    ~DB_worker();
    void Choose();
    void Add();
    void Delete();
    void Search();
    void Modify();
    void Sort();
    void ShowAll(int saveToFile = 0);
};

int main()
{
    system("color fd");
    DB_worker workers("workers");
    workers.ShowAll();
    workers.Choose();
    return 0;
}
//构造函数中连接数据库
DB_worker::DB_worker(const char*tb)
{
    fplog = fopen("log.txt", "a+");
    strcpy(tableName, tb);
    if(!ConnectDatabase()) {
        cout << "数据库连接失败,请检查连接后重试！" << endl;
        system("pause");
        exit(-1);
    }
}
//释放连接和其他资源
DB_worker::~DB_worker()
{
    mysql_free_result(res);
    mysql_close(sock);
    fclose(fplog);
}
void DB_worker::Choose()
{
    char c;
    while(c != '0') {
        ShowAll(1);
        for(int j = 0; j < 65; j++)
            cout << "☆";
        cout << endl;
        cout << "\t\t\t\t\t\t\t1.新增一名职工\n\t\t\t\t\t\t\t2.删除一名职工\n\t\t\t\t\t\t\t"
             "3.查找\n\t\t\t\t\t\t\t4.修改\n\t\t\t\t\t\t\t5.排序\n\t\t\t\t\t\t\t"
             "6.显示所有\n\t\t\t\t\t\t\t0.退出" << endl;
        for(int j = 0; j < 65; j++)
            cout << "☆";
        cout << endl;
        cout << endl << "请选择: ";
        do {
            c = getche();
            cout << "\r\t\t\t\r" << c << ":";
            switch(c) {
                case '1':
                    Add();
                    break;
                case '2':
                    Delete();
                    break;
                case '3':
                    Search();
                    break;
                case '4':
                    Modify();
                    break;
                case '5':
                    Sort();
                    break;
                case '6':
                    ShowAll();
                    break;
                case '0':
                    break;
                default:
                    cout << "\r输入错误，请重新输入:";
            }
        } while(c < '0' || c > '6');
        cout << endl;
        fclose(fplog);
        fplog = fopen("log.txt", "a+");
    }
}
void DB_worker::Add()
{
    sprintf(query, "insert into %s values ('", tableName);
    cout << "请输入 " << column[0] << ": ";
    cin >> info[0];
    strcat(query, info[0]);
    for(int i = 1; i < num_fields; i++) {
        cout << "请输入 " << column[i] << ": ";
        cin >> info[i];
        strcat(query, "','");
        strcat(query, info[i]);
    }
    strcat(query, "')");
    printDateTime(fplog);
    if(mysql_query(sock, query)) {      //执行SQL语句
        printf("添加失败 (%s)\n", mysql_error(sock));
        fprintf(fplog, "Error: %s (%s)\n", query, mysql_error(sock));   //增加日志
    } else {
        puts("添加成功");
        fprintf(fplog, "%s\n", query);      //增加日志
    }
}
void DB_worker::Delete()
{
    char temp[128];
    cout << "请输入需要删除的" << column[0] << ": ";
    cin >> info[0];
    sprintf(temp, "where %s='%s'", column[0], info[0]);
    if(QueryDatabase(temp)) {      //执行SQL语句
        cout << "确定删除该元组？(1或y确认):";
        char c = getch();
        if(c == '1' || c == 'y' || c == 'Y') {
            printDateTime(fplog);
            if(QueryDatabase(temp, 1, "delete")) {
                puts("删除成功");
                fprintf(fplog, "%s\n", query);      //增加日志
            } else {
                printf("删除失败 (%s)\n", mysql_error(sock));
                fprintf(fplog, "Error: %s (%s)\n", query, mysql_error(sock));   //增加日志
            }
        } else
            cout << "已取消删除" << endl;
    } else {
        printDateTime(fplog);
        puts("删除失败 (该元组不存在)");
        fprintf(fplog, "Error: %s : 该元组不存在\n", query);      //增加日志
    }
}
void DB_worker::Search()
{
    cout << "请选择查找方式:\n";
    for(int i = 0; i < num_fields; i++)
        cout << i + 1 << "." << column[i] << "  ";
    char c = getch();
    if(c > 48 && c < 49 + num_fields) {
        cout << "\n请输入需要查找的" << column[c - 49] << ": ";
        cin >> info[0];
        char temp[128];
        sprintf(temp, "where %s='%s'", column[c - 49], info[0]);
        QueryDatabase(temp);
    }
}
void DB_worker::Modify()
{
    cout << "请选择需要修改元组的限制属性:\n";
    for(int i = 0; i < num_fields; i++)
        cout << i + 1 << "." << column[i] << "  ";
    char sel = getch();
    char temp[128];
    if(sel > 48 && sel < 49 + num_fields) {
        cout << "\r\t\t\t\t\t\t\t\t\t\t\r请输入限制元组的" << column[sel - 49] << ": ";
        cin >> info[0];
        sprintf(temp, "where %s='%s'", column[sel - 49], info[0]);
        if(QueryDatabase(temp)) {
            cout << "请选择需要修改的属性:\n";
            for(int i = 0; i < num_fields; i++)
                cout << i + 1 << "." << column[i] << "  ";
            sel = getch();
            if(sel > 48 && sel < 49 + num_fields) {
                cout << "\r\t\t\t\t\t\t\t\t\t\t\r请输入新的" << column[sel - 49] << ": ";
                cin >> info[0];
                sprintf(query, "update %s set %s='%s' %s", tableName,
                        column[sel - 49], info[0], temp);
                printDateTime(fplog);
                if(mysql_query(sock, query)) {      //执行SQL语句
                    printf("修改失败 (%s)\n", mysql_error(sock));
                    fprintf(fplog, "Error: %s (%s)\n", query, mysql_error(sock)); //增加日志
                } else {
                    puts("修改成功");
                    fprintf(fplog, "%s\n", query);  //增加日志
                }
            }
        } else
            cout << "未搜索到可修改项，建议更改修改限制属性" << endl;
    }
}
void DB_worker::Sort()
{
    cout << "请选择排序方式:\n";
    for(int i = 0; i < num_fields; i++)
        cout << i + 1 << "." << column[i] << "  ";
    char c = getch();
    if(c > 48 && c < 49 + num_fields) {
        char temp[128], way[5] = "asc";
        cout << "\r\t\t\t\t\t\t\t\t\t\t\t\r以" << column[c - 49]
             << "降序排序请按1或y,按q取消排序,其他键将按升序排序: ";
        char c2 = getch();
        cout << "\r\t\t\t\t\t\t\t\r按" << column[c - 49];
        if(c2 == 'q' || c2 == 'Q') {
            cout << "\r\t\t\t\t\t\t\t\r已取消排序" << endl;
            return;
        } else if(c2 == '1' || c2 == 'y' || c2 == 'Y') {
            strcpy(way, "desc");
            cout << "降序排序" << endl;
        } else
            cout << "升序排序" << endl;
        sprintf(temp, "order by %s %s", column[c - 49], way);
        if(QueryDatabase(temp)) {
            printDateTime(fplog);
            fprintf(fplog, "%s\n", query);
        }
    }
}
void DB_worker::ShowAll(int saveToFile)
{
    QueryDatabase(0, 1, "select *", saveToFile);
}
//连接数据库
bool DB_worker::ConnectDatabase()
{
    int result = 1;
    mysql_init(&mysql);  //初始化mysql，连接数据库
    //我没有设置MySQL密码,所以此处密码字段为空,可以根据具体环境修改参数
    if (!(sock = mysql_real_connect(&mysql, "localhost", "root", "123456", "test", 0, NULL, 0))) {
        printf( "Error connecting to database:%s\n", mysql_error(&mysql));
        result = 0;
    } else {
        printf("Connected successful\n");
        int timeout =  2;         //设置查询超时时长
        if(sock != NULL) {
            //设置链接超时时间.
            mysql_options(sock, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);
            //设置查询数据库(select)超时时间
            mysql_options(sock, MYSQL_OPT_READ_TIMEOUT, (const char *)&timeout);
            //设置写数据库(update,delect,insert,replace等)的超时时间。
            mysql_options(sock, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&timeout);
        }
        mysql_query(sock, "set names 'GBK'"); //设置字符集，防止中文无法正常显示
        sprintf(query, "select * from %s ", tableName);
        mysql_query(sock, query);
        res = mysql_store_result(sock);
        num_fields = mysql_num_fields(res);
        //获取字段的信息
        for(int i = 0; i < num_fields; i++)
            strcpy(column[i], mysql_fetch_field(res)->name);
        puts("");
    }
    puts("————————————————————\n");
    return result;
}
//建立查询
bool DB_worker::QueryDatabase(const char*s, int show, const char* todo, int saveToFile)
{
    sprintf(query, "%s from %s ", todo, tableName);
    if(s)   // 连接上条件语句
        strcat(query, s);
    if(mysql_query(sock, query)) {      //执行SQL语句
        printf("Query failed (%s)\n", mysql_error(sock));
        return 0;
    } else if(strcmp(todo, "delete") == 0)
        return 1;
    //获取结果集
    if (!(res = mysql_store_result(sock))) { //获得sql语句结束后返回的结果集
        printf("Couldn't get result from %s\n", mysql_error(sock));
        return 0;
    }
    //打印结果数
    int affect = mysql_affected_rows(sock);
    if(show) {
        if(!saveToFile)
            printf(">>>职工表中————共查询到 %d 组结果<<<\n", affect);
        //获取字段的信息
        if(affect > 0) {
            if(saveToFile)
                freopen("data.txt", "w", stdout);
            for(int i = 0; i < num_fields; i++)
                printf("%-*s", 130 / num_fields, column[i]);
            puts("");
            //打印获取的数据
            MYSQL_ROW row; //一个行数据的类型安全(type-safe)的表示
            while (row = mysql_fetch_row(res)) {    //获取下一行
                for(int i = 0; i < num_fields; i++)
                    printf("%-*s", 130 / num_fields, row[i]);
                puts("");
            }
            if(saveToFile) {
                freopen("CON", "w", stdout);
                return true;
            }
        }
    }
    puts("————————————————————\n");
    return affect;
}
//在日志文件中打印当前日期和时间
void DB_worker::printDateTime(FILE * fp)
{
    time_t now_time = time(NULL);
    struct tm *newtime = localtime(&now_time);
    char tmpbuf[128];
    static int first = 1;
    if(first) {
        strftime(tmpbuf, 128, "\n******%Y/%m/%d******\n", newtime);
        fprintf(fp, tmpbuf);
        first = 0;
    }
    strftime(tmpbuf, 128, "%H:%M:%S    ", newtime);
    fprintf(fp, tmpbuf);
}
