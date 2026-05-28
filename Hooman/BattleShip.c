#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define ROWS 3
#define COLUMNS 3

const int OFFSETS[] = {1};

int x = 0;
int y = 0;

int xOffset = 0;
int yOffset = OFFSETS[0];

bool player = 0;
int p1Index[2] = {0, 0};

typedef enum {EMPTY, PLACED, HIT, MISS} cellValue;

typedef struct cell {
    cellValue value;
    bool cursor;
} cell;

cell lights[2][ROWS][COLUMNS] = {
    [0 ... 1] {
        [0 ... ROWS-1] = {
            [0 ... COLUMNS-1] = {EMPTY, false}
        }
    }
};

unsigned int VRx;
unsigned int VRy;

enum directions {REST, UP, DOWN, LEFT, RIGHT} direction;

void GetDirections() {
    adc_select_input(0); 
    VRx = adc_read();

    adc_select_input(1); 
    VRy = adc_read();

    if (VRy > 2500 && VRx > 250 && VRx < 2500) direction = UP;
    else if (VRy < 250 && VRx > 250 && VRx < 2500) direction = DOWN;
    else if (VRx < 250 && VRy > 250 && VRy < 2500) direction = LEFT;
    else if (VRx > 2500 && VRy > 250 && VRy < 2500) direction = RIGHT;
    else direction = REST;
}

void UpdateBoard() {

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[player][y + j][x + i].cursor = false;
    }

    if (direction == UP && y > 0) y--;
    else if (direction == DOWN && y < ROWS - 1 - yOffset) y++;
    else if (direction == LEFT && x > 0) x--;
    else if (direction == RIGHT && x < COLUMNS - 1 - xOffset) x++;

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[player][y + j][x + i].cursor = true;
    }
}


void UpdateLights1() {
    gpio_put(2, lights[player][0][0].cursor || lights[player][0][0].value);
    gpio_put(3, lights[player][0][1].cursor || lights[player][0][1].value);
    gpio_put(4, lights[player][0][2].cursor || lights[player][0][2].value);

    gpio_put(5, lights[player][1][0].cursor || lights[player][1][0].value);
    gpio_put(6, lights[player][1][1].cursor || lights[player][1][1].value);
    gpio_put(7, lights[player][1][2].cursor || lights[player][1][2].value);

    gpio_put(8, lights[player][2][0].cursor || lights[player][2][0].value);
    gpio_put(9, lights[player][2][1].cursor || lights[player][2][1].value);
    gpio_put(10, lights[player][2][2].cursor || lights[player][2][2].value);
}

void UpdateLights2() {
    gpio_put(2, lights[player][0][0].cursor);
    gpio_put(3, lights[player][0][1].cursor);
    gpio_put(4, lights[player][0][2].cursor);

    gpio_put(5, lights[player][1][0].cursor);
    gpio_put(6, lights[player][1][1].cursor);
    gpio_put(7, lights[player][1][2].cursor);

    gpio_put(8, lights[player][2][0].cursor);
    gpio_put(9, lights[player][2][1].cursor);
    gpio_put(10, lights[player][2][2].cursor);
}

bool rotate;
bool place;

void GetPress() {
    place = gpio_get(21);
    rotate = gpio_get(22);
}

void SetCell() {
    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) {
            if (lights[player][y + j][x + i].value == PLACED) return;
        }
    }

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) {
            lights[player][y + j][x + i].value = PLACED;
        }
    }

    p1Index[player]++;
    x = 0;
    y = 0;
    
    if(xOffset) xOffset = OFFSETS[p1Index[player]];
    if(yOffset) yOffset = OFFSETS[p1Index[player]];
}

void Attack() {
    if (lights[!player][y][x].value == HIT || lights[!player][y][x].value == MISS) return;

    if (lights[!player][y][x].value == EMPTY) {
        lights[!player][y][x].value = MISS;
        gpio_put(18, 0);
    }
    else if (lights[!player][y][x].value == PLACED) {
        lights[!player][y][x].value = HIT;
        gpio_put(18, 1);
    }

    x = 0;
    y = 0;
}

bool CheckForWinner() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLUMNS; j++) {
            if (lights[!player][i][j].value == PLACED) return false;
        }
    }

    return true;
}

void RotateShip() {
    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[player][y + j][x + i].cursor = false;
    }

    if (x + yOffset > COLUMNS - 1) x = x - ((x + yOffset) - (COLUMNS - 1));
    if (y + xOffset > ROWS - 1) y = y - ((y + xOffset) - (ROWS - 1));
    
    int temp = xOffset;
    xOffset = yOffset;
    yOffset = temp;

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[player][y + j][x + i].cursor = true;
    }
}

enum modeStates {MODE_START, WAIT, PUSH, POSITION, PLAY} modeState;
enum placeStates {PLACE_START, PLACE_IDLE, PLACE_MOVE, ROTATE, PLACE, READY} placeState;
enum playStates {PLAY_START, PLAY_IDLE, PLAY_MOVE, ATTACK, WON} playState;

bool Tick(struct repeating_timer *t) {



    switch (modeState) {
        case MODE_START:
            modeState = WAIT;
            if (!player) gpio_put(18, 1);
            else gpio_put(16, 1);
            break;
        case WAIT:
            GetPress();
            if (place) {
                modeState = PUSH;
            }
            break;
        case PUSH:
            GetPress();
            if (!place) {
                modeState = POSITION;
            }
            break;
        case POSITION:
            switch (placeState) {
                case PLACE_START:
                    xOffset = 0;
                    yOffset = OFFSETS[0];
                    UpdateBoard();
                    placeState = PLACE_IDLE;
                    break;
                case PLACE_IDLE:
                    GetDirections();
                    GetPress();
                    if(direction && !rotate && !place) {
                        placeState = PLACE_MOVE;
                        UpdateBoard();
                    } else if (!direction && rotate && !place) {
                        placeState = ROTATE;
                        RotateShip();
                    } else if (!direction && !rotate && place) {
                        placeState = PLACE;
                        SetCell();
                        if (p1Index[player] < sizeof(OFFSETS)/sizeof(OFFSETS[0])) UpdateBoard();
                    } else if (p1Index[player] >= sizeof(OFFSETS)/sizeof(OFFSETS[0])) {                       
                        placeState = READY;
                    }
                    break;
                case PLACE_MOVE:
                    GetDirections();
                    if (!direction) placeState = PLACE_IDLE;
                    break;
                case ROTATE:
                    GetPress();
                    if (!rotate) placeState = PLACE_IDLE;
                    break;
                case PLACE:
                    GetPress();
                    if (!place) placeState = PLACE_IDLE;
                    break;
                case READY:
                    if (!player) {
                        player = 1;
                        modeState = MODE_START;
                        placeState = PLACE_START;
                        gpio_put(18, 0);
                    } else {
                        modeState = PLAY;
                        gpio_put(16, 0);
                    }
                    break;
                default:
                    placeState = PLACE_START;
                    break;
            }
            UpdateLights1();
            break;
        case PLAY:
            
            switch(playState) {
                case PLAY_START:
                    xOffset = 0;
                    yOffset = 0;
                    UpdateBoard();
                    playState = PLAY_IDLE;
                    break;
                case PLAY_IDLE:
                    GetDirections();
                    GetPress();
                    if(direction && !place) {
                        playState = PLAY_MOVE;
                        UpdateBoard();
                    } else if (!direction && place) {
                        playState = ATTACK;
                        Attack();
                        
                        if (CheckForWinner()) {
                            playState = WON;
                        }
                    }
                    break;
                case PLAY_MOVE:
                    GetDirections();
                    if (!direction) playState = PLAY_IDLE;
                    break;
                case ATTACK:
                    GetPress();
                    if (!place) {
                        
                        player = !player;

                        UpdateBoard();
                        playState = PLAY_IDLE;
                    }
                    break;
                case WON:
                    gpio_put(18, 0);
                    gpio_put(16, 0);
                    break;
                default:
                    playState = PLAY_START;
                    break;
            }
            UpdateLights2();
            break;
        default:
            modeState = MODE_START;
            break;
    }

    return true;
}

int main()
{
    stdio_init_all();
    adc_init();

    adc_gpio_init(26);
    adc_gpio_init(27);

    gpio_init(22); gpio_set_dir(22, false);
    gpio_init(21); gpio_set_dir(21, false);

    gpio_init(2); gpio_set_dir(2, true);
    gpio_init(3); gpio_set_dir(3, true);
    gpio_init(4); gpio_set_dir(4, true);
    gpio_init(5); gpio_set_dir(5, true);
    gpio_init(6); gpio_set_dir(6, true);
    gpio_init(7); gpio_set_dir(7, true);
    gpio_init(8); gpio_set_dir(8, true);
    gpio_init(9); gpio_set_dir(9, true);
    gpio_init(10); gpio_set_dir(10, true);

    gpio_init(16); gpio_set_dir(16, true);
    gpio_init(18); gpio_set_dir(18, true);

    modeState = MODE_START;
    placeState = PLACE_START;
    playState = PLAY_START;

    struct repeating_timer timer;
    add_repeating_timer_ms(-100, Tick, NULL, &timer);
    while (1) {}
}
