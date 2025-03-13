// 代码参考：https://oshwhub.com/shukkkk/esp32s3_tft_mp3
#include <Arduino.h>
// #include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "time.h"
#include "esp_sntp.h"
#include <mbedtls/md.h>
#include <base64.h>
#include "Base64_Arturo.h"
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>
#include "SPI.h"
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <esp_system.h>
// #define PIN_PIXS LED_BUILTIN
// #define PIX_NUM 1

// Adafruit_NeoPixel pixels(PIX_NUM, PIN_PIXS, NEO_GRB + NEO_KHZ800);
#define I2S_WS D9
#define I2S_SD D3
#define I2S_SCK D10
#define I2S_PORT_0 I2S_NUM_0
#define SAMPLE_RATE 16000
#define RECORD_TIME_SECONDS 10
#define BUFFER_SIZE (SAMPLE_RATE * RECORD_TIME_SECONDS)

#define CHUNK_SIZE 2048
const int recordTimeSeconds = 3;//录音时间秒为单位
int16_t audioData[2560];
int16_t *pcm_data; // 录音缓存区
uint recordingSize = 0;

// char* psramBuffer = (char*)ps_malloc(512000);
String odl_answer;

String answer_list[10];
uint8_t answer_list_num = 0;
bool answer_ste = 0;

const char *ssid = "XuHandy";
const char *password = "qwer1234";

// 讯飞STT 的key
String STTAPPID = "304358f2";
const char *STTAPISecret = "NjFmYjA2OWFiZTk4ZmE5OTU4NjUyZTgz";
const char *STTAPIKey = "6da7ae9a118c30d95a71b51ab122a9fd";


using namespace websockets;
WebsocketsClient client;

const char *ntpServer1 = "ntp.org";
const char *ntpServer2 = "ntp.ntsc.ac.cn";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
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

void i2s_install()
{
  const i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = i2s_bits_per_sample_t(16),
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = 0, // default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = 1024,
      .use_apll = false};

  esp_err_t err = i2s_driver_install(I2S_PORT_0, &i2s_config, 0, NULL);
  if (err != ESP_OK)
  {
    Serial.printf("I2S driver install failed (I2S_PORT_0): %d\n", err);
    while (true)
      ;
  }

 
}

void i2s_setpin()
{
  const i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  esp_err_t err = i2s_set_pin(I2S_PORT_0, &pin_config);
  if (err != ESP_OK)
  {
    Serial.printf("I2S set pin failed (I2S_PORT_0): %d\n", err);
    while (true)
      ;
  }

  
}

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

// 向讯飞STT发送音频数据
void STTsend()
{
  uint8_t status = 0;
  int dataSize = 1280 * 8;
  int audioDataSize = recordingSize * 2;
  uint lan = (audioDataSize) / dataSize;
  uint lan_end = (audioDataSize) % dataSize;
  if (lan_end > 0)
  {
    lan++;
  }

  // Serial.printf("byteDatasize: %d , lan: %d , lan_end: %d \n", audioDataSize, lan, lan_end);
  String host_url = XF_wsUrl(STTAPISecret, STTAPIKey, "/v2/iat", "ws-api.xfyun.cn");
  Serial.println("Connecting to server.");
  bool connected = client.connect(host_url);
  if (connected)
  {
    Serial.println("Connected!");
  }
  else
  {
    Serial.println("Not Connected!");
  }
  // 分段向STT发送PCM音频数据
  for (int i = 0; i < lan; i++)
  {

    if (i == (lan - 1))
    {
      status = 2;
    }
    if (status == 0)
    {
      String input = "{";
      input += "\"common\":{ \"app_id\":\"" + STTAPPID + "\" },";
      input += "\"business\":{\"domain\": \"iat\", \"language\": \"zh_cn\", \"accent\": \"mandarin\", \"vinfo\":1,\"vad_eos\":10000},";
      input += "\"data\":{\"status\": 0, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
      String base64audioString = base64::encode((uint8_t *)pcm_data, dataSize);
      input += base64audioString;
      input += "\"}}";
      Serial.printf("input: %d , status: %d \n", i, status);
      client.send(input);
      status = 1;
    }
    else if (status == 1)
    {
      String input = "{";
      input += "\"data\":{\"status\": 1, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
      String base64audioString = base64::encode((uint8_t *)pcm_data + (i * dataSize), dataSize);
      input += base64audioString;
      input += "\"}}";
      // Serial.printf("input: %d , status: %d \n", i, status);
      client.send(input);
    }
    else if (status == 2)
    {
      if (lan_end == 0)
      {
        String input = "{";
        input += "\"data\":{\"status\": 2, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
        String base64audioString = base64::encode((uint8_t *)pcm_data + (i * dataSize), dataSize);
        input += base64audioString;
        input += "\"}}";
        Serial.printf("input: %d , status: %d \n", i, status);
        client.send(input);
      }
      if (lan_end > 0)
      {
        String input = "{";
        input += "\"data\":{\"status\": 2, \"format\": \"audio/L16;rate=16000\",\"encoding\": \"raw\",\"audio\":\"";
        String base64audioString = base64::encode((uint8_t *)pcm_data + (i * dataSize), lan_end);
        input += base64audioString;
        input += "\"}}";
        Serial.printf("input: %d , status: %d \n", i, status);
        client.send(input);
      }
    }
    delay(30);
  }
}


void setup()
{

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  setup_ntp_client();
  getDateTime();

  // pixels.begin();
  // pixels.setBrightness(8);
  // // 熄灭2812
  // pixels.clear();
  // pixels.show();

  Serial.println("Setup I2S ...");
  i2s_install();
  i2s_setpin();
  esp_err_t err = i2s_start(I2S_PORT_0);
  if (err != ESP_OK)
  {
    Serial.printf("I2S start failed (I2S_PORT_0): %d\n", err);
    while (true)
      ;
  }

  // run callback when messages are received
  client.onMessage([&](WebsocketsMessage message) { // STT ws连接的回调函数
    Serial.print("Got Message: ");
    Serial.println(message.data());
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message.data());
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonArray ws = doc["data"]["result"]["ws"];
    for (JsonObject word : ws)
    {
      int bg = word["bg"];
      const char *w = word["cw"][0]["w"];
      stttext += w;
    }
    if (doc["data"]["status"] == 2)
    { // 收到结束标志
      sttste = 1;
      Serial.print("语音识别：");
      Serial.println(stttext);
    }
  });

}


void loop()
{

  if (Serial.available())
  {
    // delay(20);
    if (Serial.read() == '1')
    {
      stttext = "";
      Serial.println("Recording...");
      size_t bytes_read = 0;
      recordingSize = 0;
      // 分配 pcm_data
      pcm_data = (int16_t *)ps_malloc(BUFFER_SIZE * sizeof(int16_t));
      if (!pcm_data)
      {
        Serial.println("Failed to allocate memory for pcm_data from PSRAM");
        return;
      }

      // uint16_t x = 0, y = 0;
      while (recordingSize < recordTimeSeconds* SAMPLE_RATE)
      { // 开始循环录音，将录制结果保存在pcm_data中
        esp_err_t result = i2s_read(I2S_PORT_0, audioData, sizeof(audioData), &bytes_read, portMAX_DELAY);
        memcpy(pcm_data + recordingSize, audioData, bytes_read);
        recordingSize += bytes_read / 2;
      }

      Serial.printf("Total bytes read: %d\n", recordingSize);
      Serial.println("Recording complete.");
      STTsend(); // STT请求开始
      free(pcm_data);
    }
  }

  if (client.available())
  {
    client.poll();
  }
  
  delay(50);
}


