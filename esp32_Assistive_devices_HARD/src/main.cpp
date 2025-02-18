#include <Arduino.h>

//my function
#include "my_board_LED.h"
#include "my_WiFi.h"

const char* ssid = "XuHandy";
const char* password = "qwer1234";



// int wifiConnect_hotpoint();

// the setup function runs once when you press reset or power the board
void setup() {
  // 初始化串口通信，波特率为115200
  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  initWiFi_direct(ssid, password);

}

// the loop function runs over and over again forever
void loop() {
  board_led_blink_nonblocking(100);

}






// int wifiConnect_hotpoint()
// {
//     // 断开当前WiFi连接
//     WiFi.disconnect(true);

//     preferences.begin("wifi_store");
//     int numNetworks = preferences.getInt("numNetworks", 0);
//     if (numNetworks == 0)
//     {
//         // 在屏幕上输出提示信息
//         // u8g2.setCursor(0, u8g2.getCursorY() + 12);
//         // u8g2.print("无任何wifi存储信息！");
//         // displayWrappedText("请连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, width);
//         preferences.end();
//         return 0;
//     }

//     // 获取存储的 WiFi 配置
//     for (int i = 0; i < numNetworks; ++i)
//     {
//         String ssid = preferences.getString(("ssid" + String(i)).c_str(), "");
//         String password = preferences.getString(("password" + String(i)).c_str(), "");

//         // 尝试连接到存储的 WiFi 网络
//         if (ssid.length() > 0 && password.length() > 0)
//         {
//             Serial.print("Connecting to ");
//             Serial.println(ssid);
//             Serial.print("password:");
//             Serial.println(password);
//             // 在屏幕上显示每个网络的连接情况
//             // u8g2.setCursor(0, u8g2.getCursorY()+12);
//             // u8g2.print(ssid);

//             uint8_t count = 0;
//             WiFi.begin(ssid.c_str(), password.c_str());
//             // 等待WiFi连接成功
//             while (WiFi.status() != WL_CONNECTED)
//             {
//                 // 闪烁板载LED以指示连接状态
//                 // digitalWrite(led, ledstatus);
//                 // ledstatus = !ledstatus;
//                 count++;

//                 // 如果尝试连接超过30次，则认为连接失败
//                 if (count >= 30)
//                 {
//                     Serial.printf("\r\n-- wifi connect fail! --\r\n");
//                     // 在屏幕上显示连接失败信息
//                     // u8g2.setCursor(u8g2.getCursorX()+6, u8g2.getCursorY());
//                     // u8g2.print("Failed!");
//                     break;
//                 }

//                 vTaskDelay(100);
//             }

//             if (WiFi.status() == WL_CONNECTED)
//             {
//                 // 向串口输出连接成功信息和IP地址
//                 Serial.printf("\r\n-- wifi connect success! --\r\n");
//                 Serial.print("IP address: ");
//                 Serial.println(WiFi.localIP());

//                 // 输出当前空闲堆内存大小
//                 Serial.println("Free Heap: " + String(ESP.getFreeHeap()));
//                 // 在屏幕上显示连接成功信息
//                 // u8g2.setCursor(u8g2.getCursorX()+6, u8g2.getCursorY());
//                 // u8g2.print("Connected!");
//                 preferences.end();
//                 return 1;
//             }
//         }
//     }
//     // 清空屏幕
//     // tft.fillScreen(TFT_WHITE);
//     // // 在屏幕上输出提示信息
//     // u8g2.setCursor(0, 11);
//     // u8g2.print("网络连接失败！请检查");
//     // u8g2.setCursor(0, u8g2.getCursorY() + 12);
//     // u8g2.print("网络设备，确认可用后");
//     // u8g2.setCursor(0, u8g2.getCursorY() + 12);
//     // u8g2.print("重启设备以建立连接！");
//     // displayWrappedText("或者连接热点ESP32-Setup密码为12345678，然后在浏览器中打开http://192.168.4.1添加新的网络！", 0, u8g2.getCursorY() + 12, width);
//     preferences.end();
//     return 0;
// }

