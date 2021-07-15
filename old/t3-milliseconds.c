#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

/**
 * A lógica desse programa funciona da mesma forma que o programa que conta
 * segundos, mas aqui são contados milissegundos. Portanto, não há diferença
 * macroscopicamente perceptível na simulação.
 *
 * Novamente, supõe-se que o clock de I/O é 1 MHz. Como queremos contar
 * milissegundos utilizando um timer de 8 bits, utilizamos um prescaler de 8.
 * Nessa configuração, 125 ticks equivalem a 1 milissegundo (1 MHz / 8 = 125 kHz).
 * Assim, o TOP do timer é 125-1 = 124.
 */

// Definição do TOP do timer
#define TIMER_TOP (125 - 1)

// Variável para salvar o número de milissegundos atual
volatile uint16_t time = 0;
// Inicialmente o relógio está em modo crescente.
// Pode ficar decrescente quando `step` é -1
volatile uint16_t step = 1;

// Interrupção que é disparada toda vez que o timer atinge TOP
ISR(TIMER0_COMPA_vect) {
    // Incrementa/decrementa o número de segundos atual
    time += step;

    // Verifica o caso de overflow positivo
    if (time == 60000) {
        time = 0;
    }

    // Verifica o caso de overflow negativo
    if (time == 65535) {
        time = 59999;
    }
}

// Variáveis de estado do relógio
volatile bool clock_running = true;
volatile bool clock_ascending = true;

// Estado anterior dos botões
volatile bool start_stop_button_was_pressed = false;
volatile bool swap_mode_button_was_pressed = false;
volatile bool reset_button_was_pressed = false;

ISR(PCINT1_vect) {
    // Estado atual dos botões
    bool start_stop_button_is_pressed = (PINC & 1<<0) == 0;
    bool swap_mode_button_is_pressed = (PINC & 1<<1) == 0;
    bool reset_button_is_pressed = (PINC & 1<<2) == 0;

    // Verifica se houve borda de descida no botão de start/stop
    if (
        start_stop_button_is_pressed
        && !start_stop_button_was_pressed
    ) {
        // Desabilita/habilita o clock do timer
        if (clock_running) {
            TCCR0B &= ~(1<<1);
            clock_running = false;
        } else {
            TCCR0B |= 1<<1;
            clock_running = true;
        }
    }

    // Verifica se houve borda de descida no botão de swap_mode
    if (
        swap_mode_button_is_pressed
        && !swap_mode_button_was_pressed
    ) {
        // Inverte o sentido de contagem do relógio
        if (clock_ascending) {
            TCNT0 = TIMER_TOP - TCNT0;
            step = -1;
            clock_ascending = false;
        } else {
            TCNT0 = TIMER_TOP - TCNT0;
            step = 1;
            clock_ascending = true;
        }
    }

    // Verifica se houve borda de descida no botão de reset
    if (
        reset_button_is_pressed
        && !reset_button_was_pressed
    ) {
        // Reseta o estado do timer e do relógio
        TCNT0 = 0;
        time = 0;
    }

    // Salva o estado atual dos botões nas variáveis de estado anterior
    start_stop_button_was_pressed = start_stop_button_is_pressed;
    swap_mode_button_was_pressed = swap_mode_button_is_pressed;
    reset_button_was_pressed = reset_button_is_pressed;
}

// Mapeamento dos dígitos de 0 a 9 para os LEDs do display de 7 segmentos
uint8_t digits[10] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01100111,
};

int main() {
    // Botões (apenas os três primeiros pinos da porta C)

    // Configura todos os pinos da porta C como entrada
    DDRC  &= 0b10000000; // Data Direction Register C

    // Todas essas entradas têm pullup interno
    PORTC |= 0b01111111; // Data Register C


    // Dígito 0

    // Configura todos os pinos da porta B como saída
    DDRB  = 0b11111111; // Data Direction Register B

    // Todas essas saídas estão inicialmente em nível lógico baixo
    PORTB = 0b00000000; // Data Register B


    // Dígito 1

    // Configura todos os pinos da porta D como saída
    DDRD  = 0b11111111; // Data Direction Register D

    // Todas essas saídas estão inicialmente em nível lógico baixo
    PORTD = 0b00000000; // Data Register D


    // Configuração do Timer 0, utilizado para a contagem dos milissegundos

    // Modo de operação CTC (Clear Timer on Compare Match)
    // Prescaler de 8
    TCCR0A = 0b00000010; // Timer/Counter 0 Control Register A
    TCCR0B = 0b00000010; // Timer/Counter 0 Control Register B

    // Habilita a interrupção quando o timer atinge TOP
    TIMSK0 = 0b00000010; // Timer/counter 0 Interrupt MaSK register

    // Timer começa em 0
    TCNT0 = 0; // Timer/CouNTer 0

    // TOP do timer é `TIMER_TOP`
    OCR0A = TIMER_TOP; // Output Compare Register 0 A


    // Configuração das interrupções dos botões

    // Habilita interrupções causadas pelos pinos da porta C
    PCICR  |= 0b00000010; // Pin Change Interrupt Control Register

    // Apenas os três primeiros pinos da porta C (os três botões) ativam uma interrupção
    PCMSK1 |= 0b00000111; // Pin Change MaSK register 1


    // Habilita todas as interrupções
    sei();


    while (true) {
        uint8_t current_time = time / 1000;
        uint8_t time_digit_0 = current_time % 10;
        uint8_t time_digit_1 = current_time / 10;

        PORTB = digits[time_digit_0];
        PORTD = digits[time_digit_1];
    }
}
