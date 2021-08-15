#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

/**
 * A única diferença desse programa para o anterior é que nesse o ADC inicia a
 * conversão automaticamente quando o Timer0 atinge o TOP (auto-trigger em
 * Timer/Counter0 Compare Match A).
 */


// Frequência da CPU (em Hz)
#define CPU_CLOCK 1000000

// Taxa de amostragem do sinal analógico (em Hz)
#define SAMPLING_RATE 1000

// Número de valores do ADC salvos simultaneamente na memória
#define SAMPLES_NUMBER 20


// Interrupção que é disparada toda vez que o timer atinge TOP
ISR(TIMER0_COMPA_vect) { }


// Variáveis para salvar os valores do ADC na memória
volatile uint16_t samples[SAMPLES_NUMBER] = { 0 };
volatile uint8_t current_sample = 0;

// Interrupção que é disparada quando o ADC completa a conversão
ISR(ADC_vect) {
    // Realiza a leitura do valor convertido pelo ADC
    samples[current_sample] = ADC;

    // Avança no array utilizado para salvar as amostras
    current_sample += 1;
    if (current_sample == SAMPLES_NUMBER) {
        current_sample = 0;
    }
}


int main() {
    // Configura todos os pinos expostos do ATmega328p como entrada pull-up
    DDRB = 0b00000000;
    DDRC = 0b00000000;
    DDRD = 0b00000000;
    PORTB = 0b11111111;
    PORTC = 0b01111111;
    PORTD = 0b11111111;


    // Configuração do Timer 0, utilizado para a amostragem da entrada analógica

    // Modo de operação CTC (Clear Timer on Compare Match)
    // Prescaler de 8
    TCCR0A = 0b00000010;
    TCCR0B = 0b00000010;

    // Habilita a interrupção quando o timer atinge TOP
    TIMSK0 = 0b00000010;

    // Timer começa em 0
    TCNT0 = 0;

    // TOP do timer (8 se refere ao prescaler configurado anteriormente)
    OCR0A = CPU_CLOCK / 8 / SAMPLING_RATE - 1;


    // Configuração do ADC

    // Tensão de referência AREF externa
    // Resultado right-adjusted no registrador ADC
    // Leitura realizada na entrada ADC0
    ADMUX = 0b00000000;

    // Habilita o ADC
    // Habilita a interrupção quando o ADC termina a conversão
    // Prescaler de 16
    // Habilita o auto-trigger da conversão do ADC em Timer/Counter0 Compare Match A
    ADCSRA = 0b10101100;
    ADCSRB = 0b00000011;


    // Habilita todas as interrupções
    sei();


    // Loop principal, nada a fazer
    while (true) { }
}
