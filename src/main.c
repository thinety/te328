#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

/**
 * Algumas considerações sobre a lógica do programa:
 *
 * Supõe-se que o clock de I/O é 1 MHz (oscilador interno de 8 MHz,
 * com o divisor de clock por 8 habilitado). Queremos contar segundos,
 * e como o maior prescaler disponível é 1024, precisamos contar no
 * mínimo 976 ticks (1 Mhz / 1024 = 976.5625 Hz). Esse valor não cabe
 * em 8 bits, portanto é necessário utilizar o Timer1 com precisão de
 * 16 bits.
 *
 * Nota: os outros timers poderiam ser utilizados caso a contagem
 * fosse baseada em milissegundos, mas isso introduziria complexidade
 * desnecessária, já que seria necessário converter de milissegundos para
 * segundos dentro do loop principal do programa. Assim, optou-se por fazer
 * a contagem em segundos, e utilizar o timer de 16 bits.
 *
 * O menor prescaler que pode ser utilizado é 64, resultando em 15625
 * ticks necessários (1 MHz / 64 = 15625 Hz). Portanto, o TOP do timer é
 * 15625-1 (ele conta de 0 até 15624, totalizando 15625 ticks), e é utilizado
 * o modo de operação CTC (Clear Timer on Compare Match), o que reinicia o
 * timer quando ele atinge o TOP.
 */

// Definição do TOP do timer
#define TIMER_TOP (15625 - 1)

// Variável para salvar o número de segundos atual
volatile uint8_t time = 0;
// Inicialmente o relógio está em modo crescente.
// Pode ficar decrescente quando `step` é -1
volatile uint8_t step = 1;

// Interrupção que é disparada toda vez que o timer atinge TOP
ISR(TIMER1_COMPA_vect) {
    // Incrementa/decrementa o número de segundos atual
    time += step;

    // Verifica o caso de overflow positivo
    if (time == 60) {
        time = 0;
    }

    // Verifica o caso de overflow negativo
    if (time == 255) {
        time = 59;
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
            TCCR1B &= ~((1<<1) | (1<<0));
            clock_running = false;
        } else {
            TCCR1B |= (1<<1) | (1<<0);
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
            TCNT1 = TIMER_TOP - TCNT1;
            step = -1;
            clock_ascending = false;
        } else {
            TCNT1 = TIMER_TOP - TCNT1;
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
        TCNT1 = 0;
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


    // Configuração do Timer 1, utilizado para a contagem dos segundos

    // Modo de operação CTC (Clear Timer on Compare Match)
    // Prescaler de 64
    TCCR1A = 0b00000000; // Timer/Counter 1 Control Register A
    TCCR1B = 0b00001011; // Timer/Counter 1 Control Register B

    // Habilita a interrupção quando o timer atinge TOP
    TIMSK1 = 0b00000010; // Timer/counter 1 Interrupt MaSK register

    // Timer começa em 0
    TCNT1 = 0; // Timer/CouNTer 1

    // TOP do timer é `TIMER_TOP`
    OCR1A = TIMER_TOP; // Output Compare Register 1 A


    // Configuração das interrupções dos botões

    // Habilita interrupções causadas pelos pinos da porta C
    PCICR  |= 0b00000010; // Pin Change Interrupt Control Register

    // Apenas os três primeiros pinos da porta C (os três botões) ativam uma interrupção
    PCMSK1 |= 0b00000111; // Pin Change MaSK register 1


    // Habilita todas as interrupções
    sei();


    while (true) {
        uint8_t current_time = time;
        uint8_t time_digit_0 = current_time % 10;
        uint8_t time_digit_1 = current_time / 10;

        PORTB = digits[time_digit_0];
        PORTD = digits[time_digit_1];
    }
}
