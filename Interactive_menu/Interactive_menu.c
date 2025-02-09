#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Definição dos pinos para I2C
#define I2C_SDA 14
#define I2C_SCL 15

// Pinos do LED
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12
#define SW 22 
#define SW_botao 22
#define BUZZER_PIN 21

const int VRX = 26;          // Pino de leitura do eixo X do joystick (conectado ao ADC)
const int VRY = 27;          // Pino de leitura do eixo Y do joystick (conectado ao ADC)
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X do joystick
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y do joystick

const int LED_B = 13;                    // Pino para controle do LED azul via PWM
const int LED_R = 11;                    // Pino para controle do LED vermelho via PWM
const float DIVIDER_PWM = 16.0;          // Divisor fracional do clock para o PWM
const uint16_t PERIOD = 4096;            // Período do PWM (valor máximo do contador)
uint16_t led_b_level, led_r_level = 100; // Inicialização dos níveis de PWM para os LEDs
uint slice_led_b, slice_led_r;           // Variáveis para armazenar os slices de PWM correspondentes aos LEDs

const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};

// Duração das notas em milissegundos
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};

const uint LED = 12;            // Pino do LED conectado
const uint16_t PERIOD_LED_RGB = 2000;   // Período do PWM (valor máximo do contador)
const uint16_t LED_STEP = 100;  // Passo de incremento/decremento para o duty cycle do LED
uint16_t led_level = 100;       // Nível inicial do PWM (duty cycle)


// Função para configurar o joystick (pinos de leitura e ADC)
void setup_joystick()
{
  // Inicializa o ADC e os pinos de entrada analógica
  adc_init();         // Inicializa o módulo ADC
  adc_gpio_init(VRX); // Configura o pino VRX (eixo X) para entrada ADC
  adc_gpio_init(VRY); // Configura o pino VRY (eixo Y) para entrada ADC

  // Inicializa o pino do botão do joystick
  gpio_init(SW);             // Inicializa o pino do botão
  gpio_set_dir(SW, GPIO_IN); // Configura o pino do botão como entrada
  gpio_pull_up(SW);          // Ativa o pull-up no pino do botão para evitar flutuações
}

// Função para configurar o PWM de um LED (genérica para azul e vermelho)
void setup_pwm_led(uint led, uint *slice, uint16_t level)
{
  gpio_set_function(led, GPIO_FUNC_PWM); // Configura o pino do LED como saída PWM
  *slice = pwm_gpio_to_slice_num(led);   // Obtém o slice do PWM associado ao pino do LED
  pwm_set_clkdiv(*slice, DIVIDER_PWM);   // Define o divisor de clock do PWM
  pwm_set_wrap(*slice, PERIOD);          // Configura o valor máximo do contador (período do PWM)
  pwm_set_gpio_level(led, level);        // Define o nível inicial do PWM para o LED
  pwm_set_enabled(*slice, true);         // Habilita o PWM no slice correspondente ao LED
}

// Função de configuração geral
void setup()
{
  stdio_init_all();                                // Inicializa a porta serial para saída de dados
  setup_joystick();                                // Chama a função de configuração do joystick
  setup_pwm_led(LED_B, &slice_led_b, led_b_level); // Configura o PWM para o LED azul
  setup_pwm_led(LED_R, &slice_led_r, led_r_level); // Configura o PWM para o LED vermelho
}

// Função para ler os valores dos eixos do joystick (X e Y)
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value)
{
  // Leitura do valor do eixo X do joystick
  adc_select_input(ADC_CHANNEL_0); // Seleciona o canal ADC para o eixo X
  sleep_us(2);                     // Pequeno delay para estabilidade
  *vrx_value = adc_read();         // Lê o valor do eixo X (0-4095)

  // Leitura do valor do eixo Y do joystick
  adc_select_input(ADC_CHANNEL_1); // Seleciona o canal ADC para o eixo Y
  sleep_us(2);                     // Pequeno delay para estabilidade
  *vry_value = adc_read();         // Lê o valor do eixo Y (0-4095)
}

// Função principal














void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Toca uma nota com a frequência e duração especificadas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}

// Função principal para tocar a música





void setup_pwm()
{
    uint slice;
    gpio_set_function(LED, GPIO_FUNC_PWM); // Configura o pino do LED para função PWM
    slice = pwm_gpio_to_slice_num(LED);    // Obtém o slice do PWM associado ao pino do LED
    pwm_set_clkdiv(slice, DIVIDER_PWM);    // Define o divisor de clock do PWM
    pwm_set_wrap(slice, PERIOD_LED_RGB);           // Configura o valor máximo do contador (período do PWM)
    pwm_set_gpio_level(LED, led_level);    // Define o nível inicial do PWM para o pino do LED
    pwm_set_enabled(slice, true);          // Habilita o PWM no slice correspondente
}










//Texto do menu
char *MenuText[] = {
    "      MENU:     ",
    "               ",
    "    Joy. LED    ",
    "               ",
    "    Buzzer     ",
    "               ",
    "    LED RGB    ",
    "               "
};

int count0 = 0;
int count2 = 0;
bool botao_press = 0;
void set_leds(bool red, bool green, bool blue){
    gpio_put(LED_R_PIN, red);
    gpio_put(LED_G_PIN, green);
    gpio_put(LED_B_PIN, blue);
}

// Estrutura para área de renderização do display OLED
struct render_area frame_area;
uint8_t ssd[ssd1306_buffer_length];

// Função para inicializar o display OLED
void init_oled() {
    // Configura o barramento I2C
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o controlador SSD1306 do OLED
    ssd1306_init();

    // Configura a área de renderização
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;

    calculate_render_area_buffer_length(&frame_area);

    // Limpa o display
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}
int posicao_menu = 0;
int posicao_apertada = 0;
// Exibe uma mensagem no display OLED
void display_message(char* message[]) {
    // Limpa o buffer do display
    //memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    int y = 0;
    
    for (uint i = 0; i < 8; i++) {
        ssd1306_draw_string(ssd, 5, y, message[i]);
    
        y += 8; // Incrementa a posição vertical
    }
    
    //ssd1306_draw_string(ssd, 5, posicao_menu*8, message[posicao_menu]);
    // Atualiza o display com a mensagem
    render_on_display(ssd, &frame_area);
}


void changeMenu(int sel){
    MenuText[2] =  "    Joy LED    ";
    MenuText[4] =  "    Buzzer     ",
    MenuText[6] =  "    LED RGB    ";

    if (count0 != 0){
        posicao_menu = posicao_menu + 2;
        if (posicao_menu > 6){
            posicao_menu = 2;
        }
    }
    else if (count2 != 0){
        posicao_menu = posicao_menu - 2;
        if (posicao_menu < 2){
            posicao_menu = 6;
        }
    }
    switch (posicao_menu/3)
    {
    case 0: 
        MenuText[2] =  " X  Joy LED    ";
        break;
    case 1:
        MenuText[4] =  " X  Buzzer     ";
        break;
    case 2:
        MenuText[6] =  " X  LED RGB    ";
        break;
    default:
        break;
    }
}

void loop1(){

    while(1){
        if(gpio_get(SW_botao) == 0){
            botao_press = !botao_press;
            printf("%d", botao_press);
            sleep_ms(1000);
        }
    }
}

void joystick_led()
{
  uint16_t vrx_value, vry_value, sw_value; // Variáveis para armazenar os valores do joystick (eixos X e Y) e botão                                 // Chama a função de configuração
  printf("Joystick-PWM\n");                // Exibe uma mensagem inicial via porta serial
  // Loop principal
  while (botao_press)
  {
    joystick_read_axis(&vrx_value, &vry_value); // Lê os valores dos eixos do joystick
    // Ajusta os níveis PWM dos LEDs de acordo com os valores do joystick
    pwm_set_gpio_level(LED_B, vrx_value); // Ajusta o brilho do LED azul com o valor do eixo X
    pwm_set_gpio_level(LED_R, vry_value); // Ajusta o brilho do LED vermelho com o valor do eixo Y

    // Pequeno delay antes da próxima leitura
    sleep_ms(100); // Espera 100 ms antes de repetir o ciclo
  }
  pwm_set_gpio_level(LED_B, 0); // Ajusta o brilho do LED azul com o valor do eixo X
  pwm_set_gpio_level(LED_R, 0);
}

void play_star_wars(uint pin) {
    for (int i = 0; i < sizeof(star_wars_notes) / sizeof(star_wars_notes[0]); i++) {
        if (star_wars_notes[i] == 0) {
            sleep_ms(note_duration[i]);
            printf("Tocando musica1\n");
        } else {
            play_tone(pin, star_wars_notes[i], note_duration[i]);
            if (!botao_press){
                break;
            }
        }
    }
}

void pwm_buzzer(){
    pwm_init_buzzer(BUZZER_PIN);
    while(botao_press){
      play_star_wars(BUZZER_PIN);
    }
}


void LED_RGB(){
    uint up_down = 1; // Variável para controlar se o nível do LED aumenta ou diminui
    while (botao_press)
    {
        pwm_set_gpio_level(LED, led_level); // Define o nível atual do PWM (duty cycle)
        sleep_ms(1000);                     // Atraso de 1 segundo
        if (up_down)
        {
            led_level += LED_STEP; // Incrementa o nível do LED
            if (led_level >= PERIOD)
                up_down = 0; // Muda direção para diminuir quando atingir o período máximo
        }
        else
        {
            led_level -= LED_STEP; // Decrementa o nível do LED
            if (led_level <= LED_STEP)
                up_down = 1; // Muda direção para aumentar quando atingir o mínimo
        }
    }
    pwm_set_gpio_level(LED, 0); 
}



int main() {
    int sel = 0;
    stdio_init_all();
    
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    init_oled();
    adc_init();

    adc_gpio_init(26);
    adc_gpio_init(27);
    gpio_init(SW_botao);             // Inicializa o pino do botão
    gpio_set_dir(SW_botao, GPIO_IN); // Configura o pino do botão como entrada
    gpio_pull_up(SW_botao);
    setup_pwm();
    setup();
    pwm_set_gpio_level(LED_B, 0); // Ajusta o brilho do LED azul com o valor do eixo X
    pwm_set_gpio_level(LED_R, 0);
    pwm_set_gpio_level(LED, 0);
    multicore_launch_core1(loop1);
    display_message(MenuText);
    while (1) {
        adc_select_input(0);
        uint adc_y_raw = adc_read();

        if(adc_y_raw/1400<1){
            count0 = count0 + 1;
            count2 = 0;
        }
        else if(adc_y_raw/1400==1){
            count0 = 0;
            count2 = 0;
        }
        else{
            count2 = count2 + 1;
            count0 = 0;
        }
        sleep_ms(10);
        changeMenu(adc_y_raw/1400);
        display_message(MenuText);
        if(botao_press){
            switch (posicao_menu/3)
            {
                case 0: 
                    //parada do led
                    joystick_led();
                    break;
                case 1:
                    //parada do buzzer
                    pwm_buzzer();
                    break;
                case 2:
                    //parada do led rgb
                    LED_RGB();
                    break;
                default:
                    break;
            }
        }
    }
}
