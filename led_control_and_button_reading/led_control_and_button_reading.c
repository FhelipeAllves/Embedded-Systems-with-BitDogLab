#include "pico/stdlib.h"

#define LED_PIN 11
#define BUT_PIN 5
#define LED_PIN2 13
#define BUT_PIN2 6


int main()
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_init(BUT_PIN);
  gpio_set_dir(BUT_PIN, GPIO_IN);
  gpio_pull_up(BUT_PIN);

  while (1){
    while (gpio_get(BUT_PIN))
    {
        gpio_put(LED_PIN, 0);
    }
    gpio_put(LED_PIN, 1);
  }

}
