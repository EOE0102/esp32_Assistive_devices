#include "my_WiFi.h"

// 定义最大重试次数和超时时间
#define MAX_RETRIES 10
#define CONNECTION_TIMEOUT_MS 10000


unsigned long lastCheckTime = 0;
const unsigned long CHECK_INTERVAL = 60000; // 每分钟检查一次
bool isConnected = false;



void connectToWiFi(const char* ssid, const char* password) {
    // 检查输入参数是否为空
    if (ssid == nullptr || password == nullptr) {
        Serial.println("SSID or Password is null.");
        return;
    }

    // 设置Wi-Fi为STA模式并断开之前的连接
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // 开始连接到指定的SSID和密码
    WiFi.begin(ssid, password);

    unsigned long startAttemptTime = millis();
    int retryCount = 0;

    Serial.print("Connecting to WiFi ..");

    // 尝试连接，直到成功或达到最大重试次数/超时
    while (WiFi.status() != WL_CONNECTED && retryCount < MAX_RETRIES) {
        if (millis() - startAttemptTime > CONNECTION_TIMEOUT_MS) {
            Serial.println("\nConnection timed out!");
            break;
        }

        Serial.print('.');
        delay(500);  // 减少延迟以提高响应速度
        retryCount++;
    }

    // 检查是否成功连接
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        Serial.print("SSID: ");
        Serial.println(ssid);
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // 启用自动重连功能
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true);
        isConnected = true;
    } else {
        Serial.println("\nFailed to connect to WiFi.");
        isConnected = false;
    }
}





String getTimeFromServer()
{
    String timeurl = "https://www.baidu.com";   // 定义用于获取时间的URL
    HTTPClient http;                // 创建HTTPClient对象
    http.begin(timeurl);            // 初始化HTTP连接
    const char *headerKeys[] = {"Date"};        // 定义需要收集的HTTP头字段
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));    // 设置要收集的HTTP头字段
    int httpCode = http.GET();      // 发送HTTP GET请求

    String serverTime = "";
    if (httpCode > 0) {
        serverTime = http.header("Date");     // 从HTTP响应头中获取Date字段
        Serial.println(serverTime);           // 输出获取到的Date字段到串口
    } else {
        Serial.printf("Error on sending GET: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();                     // 结束HTTP连接

    return serverTime;
}





String checkAndPrintServerTime(const char* ssid, const char* password) {
    unsigned long currentTime = millis();

    if (!isConnected) {
        // 如果未连接到Wi-Fi，则尝试重新连接
        Serial.println("Not connected to WiFi. Attempting to reconnect...");
        connectToWiFi(ssid, password);
        return ""; // 返回空字符串表示未获取到时间
    }

    if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
        // 获取服务器时间
        String serverTime = getTimeFromServer();

        // 打印服务器时间到串口监视器
        Serial.print("Server Time: ");
        Serial.println(serverTime);

        // 更新上次检查时间
        lastCheckTime = currentTime;
        return serverTime;
    }

}




//来自例程 xunfei STT


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup_ntp_client()
{
    timeClient.begin();
    // 设置时区
    // GMT +1 = 3600
    // GMT +8 = 28800
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient.setTimeOffset(+28800);
}

bool timeste = 0;
String stttext = "";
bool sttste = 0;

String unixTimeToGMTString(time_t unixTime)
{
    char buffer[80];
    struct tm timeinfo;
    gmtime_r(&unixTime, &timeinfo);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &timeinfo);
    return String(buffer);
}

String getDateTime()
{
  // 请求网络时间
    timeClient.update();

    unsigned long epochTime = timeClient.getEpochTime();
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);

    String timeString = unixTimeToGMTString(epochTime);

    // 打印结果
    Serial.println(timeString);
    return timeString;
}




