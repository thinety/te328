#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>

/**
 * Novamente, temos que escolher o prescaler do Timer0 adequadamente. Foi utilizado
 * o valor de 64 pois é o menor prescaler para o qual `CPU_CLOCK / 64 / SAMPLING_RATE - 1`
 * cabe em 8 bits. Os dois outros prescalers menores, 1 e 8, levam a um valor de
 * TOP que estoura a capacidade do contador.
 */


// Frequência da CPU (em Hz)
#define CPU_CLOCK 1000000

// Taxa de amostragem do sinal analógico (em Hz)
#define SAMPLING_RATE 125

// Baud rate da comunicação serial (em Hz)
#define BAUD_RATE 9600


// Interrupção que é disparada toda vez que o timer atinge TOP
ISR(TIMER0_COMPA_vect) { }


// Flag que indica se uma nova leitura foi realizada
volatile bool has_new_sample = false;
// Variável para salvar o último valor gerado pelo ADC
volatile uint16_t sample = 0;

// Interrupção que é disparada quando o ADC completa a conversão
ISR(ADC_vect) {
    // Informa que há um novo valor que pode ser transimitido
    has_new_sample = true;
    // Realiza a leitura do valor convertido pelo ADC
    sample = ADC;
}


// Flag que indica se os valores devem ser transmitidos pela serial
volatile bool should_transmit = false;

// Interrupção que é disparada quando é recebido um byte pela serial
ISR(USART_RX_vect) {
    switch (UDR0) {
        case '0':
            // Ao receber '0' pela serial, transmissão é parada
            should_transmit = false;
            break;

        case '1':
            // Ao receber '1' pela serial, transmissão é realizada
            should_transmit = true;
            break;

        default:
            // Ao receber qualquer outro valor, nada é feito
            break;
    }
}


// Caracteres dos dígitos
uint8_t digits[10] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
};

// Função para envio por pooling de um byte pela serial
void USART_transmit(uint8_t data) {
    // Espera que o buffer de transmissão fique vazio
    while ((UCSR0A & (1<<5)) == 0) { }
    // Insere o byte no buffer de transmissão
    UDR0 = data;
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
    // Prescaler de 64
    TCCR0A = 0b00000010;
    TCCR0B = 0b00000011;

    // Habilita a interrupção quando o timer atinge TOP
    TIMSK0 = 0b00000010;

    // Timer começa em 0
    TCNT0 = 0;

    // TOP do timer (64 se refere ao prescaler configurado anteriormente)
    OCR0A = CPU_CLOCK / 64 / SAMPLING_RATE - 1;


    // Configuração do ADC

    // Tensão de referência AREF externa
    // Resultado right-adjusted no registrador ADC
    // Leitura realizada na entrada ADC0
    ADMUX = 0b00000000;

    // Habilita o ADC
    // Habilita a interrupção quando o ADC termina a conversão
    // Prescaler de 16
    // Habilita o auto trigger da conversão do ADC em Timer/Counter0 Compare Match A
    ADCSRA = 0b10101100;
    ADCSRB = 0b00000011;


    // Configuração do protocolo USART

    // Modo assíncrono, velocidade de transmissão dobrada
    // 8 bits de dados por frame, sem bit de paridade, 1 bit de parada
    // Habilita as funções de transmissor e receptor
    // Habilita interrupção ao concluir uma recepção
    UCSR0A = 0b00000010;
    UCSR0B = 0b10011000;
    UCSR0C = 0b00000110;

    // Configura o baud rate (8 se refere ao prescaler quando em
    // modo assíncrono com velocidade de transmissão dobrada)
    UBRR0 = CPU_CLOCK / 8 / BAUD_RATE - 1;


    // Habilita todas as interrupções
    sei();


    // Loop principal
    while (true) {
        if (!should_transmit) {
            continue;
        }

        if (has_new_sample) {
            has_new_sample = false;

            uint16_t current_sample = sample;

            // Calcula os dígitos da representação decimal do valor
            uint8_t chars[4];
            for (uint8_t i = 0; i < 4; ++i) {
                chars[3-i] = digits[current_sample%10];
                current_sample /= 10;
            }

            // Transmite os dígitos pela serial, com uma quebra de linha entre cada um
            for (uint8_t i = 0; i < 4; ++i) {
                USART_transmit(chars[i]);
            }
            USART_transmit('\r');
            USART_transmit('\n');
        }
    }
}
