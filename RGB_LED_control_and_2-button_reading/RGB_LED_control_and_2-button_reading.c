#include <stdio.h>
#include "pico/stdlib.h"

// Pinos dos componentes
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define RED_LED_PIN 12
#define GREEN_LED_PIN 13
#define BLUE_LED_PIN 11

int main() {
    // Inicializa os LEDs como saídas
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);

    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);

    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);

    // Inicializa os botões como entradas com pull-down
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    while (true) {
        // Verifica se o botão A foi pressionado
        if (!gpio_get(BUTTON_A_PIN)) {
            // Botão A pressionado, acende o LED azul
            gpio_put(BLUE_LED_PIN, 1);
            gpio_put(RED_LED_PIN, 0);
            gpio_put(GREEN_LED_PIN, 0);
            sleep_ms(100); // Atraso para evitar múltiplas leituras
        } else if (!gpio_get(BUTTON_B_PIN)) {
            // Botão B pressionado, acende o LED verde
            gpio_put(GREEN_LED_PIN, 1);
            gpio_put(RED_LED_PIN, 0);
            gpio_put(BLUE_LED_PIN, 0);
            sleep_ms(100); // Atraso para evitar múltiplas leituras
        } else {
            // Nenhum botão pressionado, apaga todos os LEDs
            gpio_put(BLUE_LED_PIN, 0);
            gpio_put(RED_LED_PIN, 0);
            gpio_put(GREEN_LED_PIN, 0);
        }
    }
}
