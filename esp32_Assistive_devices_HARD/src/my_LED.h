#ifndef MY_LED_H
#define MY_LED_H

    void board_led_blink(int led_pin, int board_led_blick_ms);
    void board_led_blink_nonblocking(int led_pin, int total_cycle_ms);
    
#endif
