//
// Created by 98302 on 2023/9/21.
//

#include "HttpResponse.h"
using namespace std;

const unordered_map<string, string> HttpResponse::MIME_TYPE = {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        { ".txt",   "text/plain" },
        { ".rtf",   "application/rtf" },
        { ".pdf",   "application/pdf" },
        { ".word",  "application/nsword" },
        { ".png",   "image/png" },
        { ".gif",   "image/gif" },
        { ".jpg",   "image/jpeg" },
        { ".jpeg",  "image/jpeg" },
        { ".au",    "audio/basic" },
        { ".mpeg",  "video/mpeg" },
        { ".mpg",   "video/mpeg" },
        { ".avi",   "video/x-msvideo" },
        { ".gz",    "application/x-gzip" },
        { ".tar",   "application/x-tar" },
        { ".css",   "text/css "},
        { ".js",    "text/javascript "},
};
const unordered_map<int, string> HttpResponse::CODE_STATUS = {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"}
};
const unordered_map<int, string> HttpResponse::CODE_PATH = {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"}
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = src_dir_ = "";
    is_keep_alive_ = false;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

/// 初始化响应报文
/// \param src_dir
/// \param path
/// \param is_keep_alive
/// \param code
void HttpResponse::Init(const std::string &src_dir, std::string &path, bool is_keep_alive, int code) {
    assert(!src_dir.empty());
    if(mm_file_){
        UnmapFile();
    }
    code_ = code;
    is_keep_alive_ = is_keep_alive;
    path_ = path;
    src_dir_ = src_dir;
    mm_file_ = nullptr;
    mm_file_stat_ = {0};
}

void HttpResponse::MakeResponse(Buffer &buffer) {
    if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0 ||
            S_ISDIR(mm_file_stat_.st_mode)){
        code_ = 404;
    }else if(!(mm_file_stat_.st_mode & S_IROTH)){
        code_ = 403;
    }else if(code_ == -1){
        code_ = 200;
    }

    ErrorHtml_();
    AddStateLine_(buffer);
    AddHeader_(buffer);
    AddContent_(buffer);
}

/// 取消文件映射
void HttpResponse::UnmapFile() {
    if(mm_file_){
        munmap(mm_file_, mm_file_stat_.st_size);  // 取消映射
        mm_file_ = nullptr;
    }
}

char *HttpResponse::File() {
    return mm_file_;
}

size_t HttpResponse::FileLen() const {
    return mm_file_stat_.st_size;
}

/// 静态异常页面
/// \param buffer
/// \param message
void HttpResponse::ErrorContent(Buffer &buffer, const std::string& message) const {
    string body, status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second;
    }else{
        status = "Bad Request";
    }
    body += to_string(code_)+" : "+status+"\n";
    body += "<p>"+message+"</p>";
    body += "<hr><em>web-server</em></body></html>";
    buffer.Append("Content-length: "+ to_string(body.size())+"\r\n\r\n");
    buffer.Append(body);
}

void HttpResponse::AddStateLine_(Buffer &buffer) {
    string status;
    if(CODE_STATUS.count(code_) == 1){
        status = CODE_STATUS.find(code_)->second;
    }else{
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buffer.Append("HTTP/1.1 "+ to_string(code_)+" "+status+" \r\n");
}

void HttpResponse::AddHeader_(Buffer &buffer) {
    buffer.Append("Connection: ");
    if(is_keep_alive_){
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: max=6, timeout=120\r\n");
    }else{
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: " + GetContentType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer &buffer) {
    int src_fd = open((src_dir_+path_).data(), O_RDONLY);
    if(src_fd < 0){
        ErrorContent(buffer, "File does not exist!");
        return;
    }
    // 文件映射，避免内核拷贝到用户
    LOG_DEBUG("file path %s", (src_dir_+path_).data());
    int *mm_ret = (int*) mmap(0,
                              mm_file_stat_.st_size,
                              PROT_READ,
                              MAP_PRIVATE,  // 写入时拷贝的私有映射
                              src_fd,
                              0);
    if(*mm_ret == -1){
        ErrorContent(buffer, "File does not exist!");
        return;
    }
    mm_file_ = (char*)mm_ret;
    close(src_fd);
    buffer.Append("Content-length: "+ to_string(mm_file_stat_.st_size)+"\r\n\r\n");  // 结束header，末尾增加空行
}

/// 转到异常页面
void HttpResponse::ErrorHtml_() {
    if(CODE_PATH.count(code_) == 1){
        path_ = CODE_PATH.find(code_)->second;
        stat((src_dir_+path_).data(), &mm_file_stat_);
    }
}

std::string HttpResponse::GetContentType_() {
    string::size_type suffix_idx = path_.find_last_of('.');
    if(suffix_idx != string::npos) {
        string suffix = path_.substr(suffix_idx);
        if (MIME_TYPE.count(suffix) == 1) {
            return MIME_TYPE.find(suffix)->second;
        }
    }
    return "text/plain";
}
