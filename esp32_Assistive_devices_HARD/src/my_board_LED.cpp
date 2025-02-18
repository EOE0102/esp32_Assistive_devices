#include "my_board_LED.h"
#include <Arduino.h>


unsigned long previousMillis = 0;
bool ledState = LOW;


//使用非阻塞方式控制LED，避免阻塞主循环。
void board_led_blink_nonblocking(int total_cycle_ms) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= total_cycle_ms) {
    previousMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}




void board_led_blink(int board_led_blick_ms){
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(board_led_blick_ms);                      // wait for a second
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
    delay(board_led_blick_ms);  
}
