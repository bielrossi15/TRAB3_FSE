#include "led.h"
#include "driver/gpio.h"

#define LED 2

void init_led() 
{
    gpio_reset_pin(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
}

void set_led_state(int state)
{
    gpio_set_level(LED, state);
}

