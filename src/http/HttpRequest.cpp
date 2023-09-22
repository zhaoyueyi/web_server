//
// Created by 98302 on 2023/9/20.
//

#include "HttpRequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture"
};
const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
        {"/register.html", 0},
        {"/login.html", 1}
};

/// 初始化请求
void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

/// 解析报文
/// \param buffer
/// \return
bool HttpRequest::Parse(Buffer &buffer) {
    const char CRLF[] = "\r\n";  // 行结束符
    if(buffer.ReadableBytes() <= 0){  // 无可读内容
        return false;
    }
    while (buffer.ReadableBytes() && state_ != FINISH){
        // 从buffer的读指针开始到读指针结束，查找CRLF出现的位置
        const char *line_end = search(buffer.Peek(), buffer.BeginWriteConst(), CRLF, CRLF+2);
        string line(buffer.Peek(), line_end);
        // 有限状态机
        switch (state_) {
            case REQUEST_LINE:
                if(!ParseRequestLine_(line)){  // 解析失败终止
                    return false;
                }
                ParsePath_();
                break;
            case HEADERS:
                ParseHeader_(line);
                if(buffer.ReadableBytes() <= 2){  // 无body
                    state_ = FINISH;
                }
                break;
            case BODY:
                ParseBody_(line);
                break;
            default:
                break;
        }
        if(line_end == buffer.BeginWrite()){  // 无可读内容
            break;
        }
        buffer.RetrieveUntil(line_end+2);
    }
    LOG_DEBUG("[%s], [%s], [%s]",
              method_.c_str(),
              path_.c_str(),
              version_.c_str());
    return true;
}

std::string HttpRequest::Path() const {
    return path_;
}

std::string &HttpRequest::Path() {
    return path_;
}

std::string HttpRequest::Method() const {
    return method_;
}

std::string HttpRequest::Version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string &key) const {
    assert(!key.empty());
    if(post_.count(key) == 1){
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char *key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1){
        return post_.find(key)->second;
    }
    return "";
}

bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" &&
               version_ == "1.1";
    }
    return false;
}

/// 解析第一行
/// ^ 限定开头
/// () 视为整体
/// [] 限定匹配括号内字符
/// [^ ] 限定匹配括号内字符以外的内容
/// * 出现0次或多次
/// . 除换行符以外的字符
/// (.*) 除换行符以外的任意字符出现0次或多次
/// ? 匹配任意数量的重复
/// ?(.*) 匹配任意字符串直到换行
/// $ 匹配行尾
/// \param line
/// \return
bool HttpRequest::ParseRequestLine_(const std::string &line) {
    //
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");  // “GET / HTTP/1.1” 匹配除空格外的任意内容
    smatch sub_match;
    if(regex_match(line, sub_match, pattern)){  // sub_match[0]表示整体
        method_ = sub_match[1];
        path_ = sub_match[2];  // "" => 路径
        version_ = sub_match[3];
        state_ = HEADERS;  // 状态转换
        return true;
    }
    LOG_ERROR("RequestLine error!");
    return false;
}

/// 解析请求头
/// \param line
void HttpRequest::ParseHeader_(const std::string &line) {
    regex pattern("^([^:]*): ?(.*)$");  // 匹配冒号键值对，key匹配到第一个冒号，value匹配剩余内容
    smatch sub_match;
    if(regex_match(line, sub_match, pattern)){
        header_[sub_match[1]] = sub_match[2];
    }else{  // 未匹配到键值对则结束请求头
        state_ = BODY;  // 状态转换
    }
}

/// 解析请求体
/// \param line
void HttpRequest::ParseBody_(const std::string &line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

/// 解析路径 路径 => html文件名
void HttpRequest::ParsePath_() {
    if(path_ == "/"){  // 默认
        path_ = "/index.html";
    }else{
        for(auto &item: DEFAULT_HTML){
            if(item == path_){
                path_ += ".html";
                break;
            }
        }
    }
}

/// 处理POST请求
void HttpRequest::ParsePost_() {
    if(method_ == "POST" &&
       header_["Content-Type"] == "application/x-www-form-urlencoded"){  // 检查method和content-type是否为post请求，带有表单数据
        ParseFromUrlEncoded_();
        if (DEFAULT_HTML_TAG.count(path_)){
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1){
                bool is_login = (tag == 1);  // 用户登录/注册
                if(UserVerify(post_["username"],
                              post_["password"],
                              is_login)){
                    path_ = "/welcome.html";
                }else{
                    path_ = "/error.html";
                }
            }
        }
    }
}

/// 解析url
void HttpRequest::ParseFromUrlEncoded_() {
    if(body_.empty()){
        return;
    }
    string key, value;
    int num = 0;
    int n = static_cast<int>(body_.size());
    int i = 0, j = 0;
    for(;i<n; ++i){
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = body_.substr(j, i-j);
                j = i+1;
                break;
            case '+':  // +换成空格
                body_[i] = ' ';
                break;
            case '%':
                num = ConvertHex_(body_[i+1])*16 + ConvertHex_(body_[i+2]);
                body_[i+2] = static_cast<char>(num % 10 + '0');
                body_[i+1] = static_cast<char>(num / 10 + '0');
                i += 2;
                break;
            case '&':  // 键值对连接符
                value = body_.substr(j, i-j);
                j = i+1;
                post_[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if(post_.count(key) == 0 && j < i){
        value = body_.substr(j, i-j);
        post_[key] = value;
    }
}

/// 用户登录
/// \param name
/// \param pwd
/// \param is_login
/// \return
bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool is_login) {
    if(name.empty() || pwd.empty()){
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnRAII conn(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    uint j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    if(!is_login){
        flag = true;
    }
    snprintf(order,
             256,
             "SELECT username, password FROM user WHERE username='%s' LIMIT 1",
             name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)){
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_field(res);
    while(MYSQL_ROW row = mysql_fetch_row(res)){
        LOG_DEBUG("MySQL row:%s %s", row[0], row[1]);
        string password(row[1]);
        if(is_login){
            if(pwd == password){
                flag = true;
            }else{
                flag = false;
                LOG_INFO("pwd error!");
            }
        }else{
            flag = false;
            LOG_INFO("user used!");
        }
    }
    mysql_free_result(res);

    // 注册
    if(!is_login && flag){
        LOG_DEBUG("register!");
        bzero(order, 256);  // 清空指令
        snprintf(order,
                 256,
                 "INSERT INTO user(username, password) VALUES('%s','%s')",
                 name.c_str(),
                 pwd.c_str());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)){
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    LOG_DEBUG("User verify success!");
    return flag;  // sql&conn 离开作用域自动析构
}

/// 16进制转10进制
/// \param ch
/// \return
int HttpRequest::ConvertHex_(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}
