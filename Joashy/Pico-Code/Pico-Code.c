#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "pixels.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

void EmptyAll() {
    gpio_put(PIN_CS, 0);
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    for (int i = 0; i < 27; i++){
        for (int j = 0; j < 27; j++){
            spi_write16_blocking(spi0, &emptyPixel[i][j], 1);
        }
    }
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    gpio_put(PIN_CS, 1);
}

void FilledAll() {
    gpio_put(PIN_CS, 0);
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    for (int i = 0; i < 27; i++){
        for (int j = 0; j < 27; j++){
            spi_write16_blocking(spi0, &filledPixel[i][j], 1);
        }
    }
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    gpio_put(PIN_CS, 1);
}

void MissedAll() {
    gpio_put(PIN_CS, 0);
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    for (int i = 0; i < 27; i++){
        for (int j = 0; j < 27; j++){
            spi_write16_blocking(spi0, &missedPixel[i][j], 1);
        }
    }
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    gpio_put(PIN_CS, 1);
}

void HitAll() {
    gpio_put(PIN_CS, 0);
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    for (int i = 0; i < 27; i++){
        for (int j = 0; j < 27; j++){
            spi_write16_blocking(spi0, &hitPixel[i][j], 1); 
        }
    }
    //for (int i = 0; i < 27; i++){ spi_write16_blocking(spi0, &whiteLine[i], 1);}
    gpio_put(PIN_CS, 1);
}
enum States {Start, emptyAll, filledAll, missedAll, hitAll} state;
bool Tick(struct repeating_timer *t) {
    switch(state) {
        case Start:
            state = emptyAll;
            break;
        case emptyAll:
            EmptyAll();
            //state = filledAll;
            break;
        case filledAll:
            FilledAll();
            state = missedAll;
            break;
        case missedAll:
            MissedAll();
            state = hitAll;
            break;
        case hitAll:
            HitAll();
            state = emptyAll;
            break;
    }
    return true;
}


int main()
{
    stdio_init_all();

    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000 * 1000); // 1MHz
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS); gpio_set_dir(PIN_CS, GPIO_OUT); gpio_put(PIN_CS, 1);

    // Example to turn on the Pico W LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    state = Start;
    struct repeating_timer timer;
    add_repeating_timer_ms(-1000, Tick, NULL, &timer);
    while (1) {}
}

