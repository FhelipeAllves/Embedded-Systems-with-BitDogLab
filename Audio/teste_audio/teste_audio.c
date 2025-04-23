#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "audio_data.h"  // <- arquivo com as amostras

#define BUZZER_PIN 21  // ajuste para o pino que você estiver usando

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // clock mais rápido para melhor resolução
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Reproduz o áudio usando o array de amostras
void play_audio_pwm(uint pin) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t pwm_freq = 16000;  // 16 kHz (igual à frequência de amostragem do áudio)
    uint32_t top = clock_freq / pwm_freq;

    pwm_set_wrap(slice_num, top);

    for (int i = 0; i < 1000; i++) {
        uint32_t level = (audio_data[i] * top) / 255; // mapeia 0–255 para 0–top
        pwm_set_gpio_level(pin, level);
        sleep_us(62);  // 1 / 16000 Hz ≈ 62.5 µs por amostra
    }

    pwm_set_gpio_level(pin, 0); // desliga o som após reprodução
}

int main() {
    stdio_init_all();
    pwm_init_buzzer(BUZZER_PIN);

    while (true) {
        play_audio_pwm(BUZZER_PIN);
        sleep_ms(1000); // espera antes de repetir
    }

    return 0;
}
