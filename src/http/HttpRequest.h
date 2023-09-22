//
// Created by 98302 on 2023/9/20.
//

#ifndef WEB_SERVER_HTTPREQUEST_H
#define WEB_SERVER_HTTPREQUEST_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <regex>
#include "../buffer/Buffer.h"
#include "../log/Log.h"
#include "../pool/SqlConnPool.h"

/*
 * http请求类
 * 按行读取buffer并解析单个http请求
 * TODO： format
 * HTTP request：
 * 1. method path version\r\n
 * 2. key: value\r\n
 * 3. ...\r\n
 * 4. body
 */
class HttpRequest {
public:
    enum PARSE_STATE{
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    HttpRequest(){Init();}
    ~HttpRequest() = default;
    void Init();
    bool Parse(Buffer &buffer);

    std::string Path() const;
    std::string &Path();
    std::string Method() const;
    std::string Version() const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;

    bool IsKeepAlive() const;
private:
    bool ParseRequestLine_(const std::string &line);
    void ParseHeader_(const std::string &line);
    void ParseBody_(const std::string &line);

    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlEncoded_();

    static bool UserVerify(const std::string &name,
                           const std::string &pwd,
                           bool is_login);
    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConvertHex_(char ch);  // 16进制转10进制
};


#endif //WEB_SERVER_HTTPREQUEST_H
