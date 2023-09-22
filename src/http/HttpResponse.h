//
// Created by 98302 on 2023/9/21.
//

#ifndef WEB_SERVER_HTTPRESPONSE_H
#define WEB_SERVER_HTTPRESPONSE_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unordered_map>
#include "../buffer/Buffer.h"
#include "../log/Log.h"

/*
 * http响应报文
 * 格式：
 * 1. version code status\r\n
 * 2. key: value\r\n
 * 3. ...\r\n
 * 4. \r\n
 * 5. file-content
 */
class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string &src_dir,
              std::string &path,
              bool is_keep_alive = false,
              int code = -1);
    void MakeResponse(Buffer &buffer);
    void UnmapFile();
    char *File();
    size_t FileLen() const;
    void ErrorContent(Buffer &buffer, const std::string& message) const;
    int Code() const { return code_;}
private:
    void AddStateLine_(Buffer &buffer);
    void AddHeader_(Buffer &buffer);
    void AddContent_(Buffer &buffer);
    void ErrorHtml_();
    std::string GetContentType_();

    int code_;
    bool is_keep_alive_;
    std::string path_;
    std::string src_dir_;
    char *mm_file_;
    struct stat mm_file_stat_;

    static const std::unordered_map<std::string, std::string> MIME_TYPE;  // 后缀类型集
    static const std::unordered_map<int, std::string> CODE_STATUS;  // 编码状态集
    static const std::unordered_map<int, std::string> CODE_PATH;  // 编码路径集
};


#endif //WEB_SERVER_HTTPRESPONSE_H
