#include <avr/io.h>

// Mapeamento dos dígitos de 0 a 8 para os
// LEDs do display de 7 segmentos
uint8_t digits[9] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
};

int main() {
    // PD - entrada pull-up
    DDRD  = 0b00000000;
    PORTD = 0b11111111;

    // PB - saída inicialmente desligada
    DDRB  = 0b11111111;
    PORTB = 0b00000000;

    while (1) {
        // Quando nenhum botão está pressionado, o dígito mostrado será 0
        uint8_t pressed_digit = 0;

        // Passa por todas as entradas, checando se o botão está pressionado.
        // Caso mais de um esteja pressionado ao mesmo tempo, o dígito mais
        // alto será mostrado
        for (uint8_t i = 0; i < 8; ++i) {
            uint8_t mask = 1<<i;
            if ((PIND & mask) == 0) {
                pressed_digit = i+1;
            }
        }

        // Mostra o dígito no display
        PORTB = digits[pressed_digit];
    }
}
