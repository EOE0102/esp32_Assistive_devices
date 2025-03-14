#include "my_STT.h"


// 处理url格式
String formatDateForURL(String dateString)
{
  // 替换空格为 "+"
    dateString.replace(" ", "+");
    dateString.replace(",", "%2C");
    dateString.replace(":", "%3A");
    return dateString;
}


// 构造讯飞ws连接url
String XF_wsUrl(const char *Secret, const char *Key, String request, String host)
{
    String timeString = getDateTime();
    String signature_origin = "host: " + host;
    signature_origin += "\n";
    signature_origin += "date: ";
    signature_origin += timeString;
    signature_origin += "\n";
    signature_origin += "GET " + request + " HTTP/1.1";

    // 使用 mbedtls 计算 HMAC-SHA256
    unsigned char hmacResult[32]; // SHA256 产生的哈希结果长度为 32 字节
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1); // 1 表示 HMAC
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)Secret, strlen(Secret));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)signature_origin.c_str(), signature_origin.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
    // 对结果进行 Base64 编码
    String base64Result = base64::encode(hmacResult, 32);

    String authorization_origin = "api_key=\"";
    authorization_origin += Key;
    authorization_origin += "\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"";
    authorization_origin += base64Result;
    authorization_origin += "\"";
    String authorization = base64::encode(authorization_origin);

    String url = "ws://" + host + request;
    url += "?authorization=";
    url += authorization;
    url += "&date=";
    url += formatDateForURL(timeString);
    url += "&host=" + host;
    return url;
}

