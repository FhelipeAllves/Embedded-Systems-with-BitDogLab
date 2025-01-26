#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

#define BTN_A_PIN 5

int A_state = 0;    //Botao A está pressionado?

void SinalAberto(){
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);   
}

void SinalAtencao(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 1);
    gpio_put(LED_B_PIN, 0);
}

void SinalFechado(){
    gpio_put(LED_R_PIN, 1);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 0);
}

void Display(int cond, struct render_area *frame_area) {

    // Zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, frame_area);

    // Declaração da variável 'text'
    char *text[3]; // Máximo de 3 strings 

    if (cond == 1) {  
        text[0] = " SINAL ABERTO";
        text[1] = "- ATRAVESSAR";
        text[2] = " COM CUIDADO ";
    } else if (cond == 2) {
        text[0] = " SINAL DE";
        text[1] = " ATENCAO";
        text[2] = "-PREPARE-SE";
    } else {
        text[0] = " SINAL FECHADO ";
        text[1] = " AGUARDE";
        text[2] = NULL; // Finaliza o array, pois só tem duas mensagens aqui
    }

    // Exibe as strings no display
    int y = 0;
    for (uint i = 0; text[i] != NULL; i++) { // Verifica até encontrar NULL
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, frame_area);
}

int WaitWithRead(int timeMS){
    for(int i = 0; i < timeMS; i = i+100){
        A_state = !gpio_get(BTN_A_PIN);
        if(A_state == 1){
            return 1;
        }
        sleep_ms(100);
    }
    return 0;
}

int main(){
    int cond = 0; // cond é um inteiro que indica a condição do sinal e é passado para a 
                  // função display

    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário

    // INICIANDO LEDS
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    // INICIANDO BOTÄO
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);
    
    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    while(true){

        //SINAL VERMELHO PARA OS PEDESTRES POR 8s OU ATÉ O BOTÃO SER PRESSIONADO
        SinalFechado();
        cond = 3;
        Display(cond, &frame_area);     // função para apresentar a mensagem no display
        A_state = WaitWithRead(8000);   //espera com leitura do botäo
        //sleep_ms(8000);

        if(A_state){               //ALGUEM APERTOU O BOTAO - SAI DO SEMAFORO NORMAL
            //SINAL AMARELO PARA OS PEDESTRES POR 5s
            SinalAtencao();
            cond = 2;
            Display(cond, &frame_area);
            sleep_ms(5000);

            //SINAL VERDE PARA OS PEDESTRES POR 10s
            SinalAberto();
            cond = 1;
            Display(cond, &frame_area);
            sleep_ms(10000);

        }else{                          //NINGUEM APERTOU O BOTAO - CONTINUA NO SEMAFORO NORMAL
                                      
            //SINAL AMARELO PARA OS PEDESTRES POR 2s
            SinalAtencao();
            cond = 2;
            Display(cond, &frame_area);
            sleep_ms(2000);

            //SINAL VERDE PARA OS PEDESTRES POR 8s
            SinalAberto();
            cond = 1;
            Display(cond, &frame_area);
            sleep_ms(8000);
        }
                
    }

    return 0;

}