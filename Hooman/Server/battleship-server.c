#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

// ─── CONFIG ────────────────────────────────────────────────────────────────
#define WIFI_SSID     "HMI"
#define WIFI_PASSWORD "thisismypassword"
#define SERVER_PORT   4242

#define MY_PLAYER 0
// ───────────────────────────────────────────────────────────────────────────

// ─── TCP ───────────────────────────────────────────────────────────────────
static struct tcp_pcb *client_pcb = NULL;
 
static void send_msg(const char *msg) {
    if (!client_pcb) return;
    tcp_write(client_pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
    tcp_output(client_pcb);
}
// ───────────────────────────────────────────────────────────────────────────


#define ROWS 3
#define COLUMNS 3

const int OFFSETS[] = {1, 0};
#define NUM_SHIPS (sizeof(OFFSETS) / sizeof(OFFSETS[0]))
static int SHIP_CELL_COUNT = 0;

bool myTurn = (MY_PLAYER == 0);
bool iAmReady = false;
bool theyAreReady = false;

int myHits = 0;
int theirHits = 0;

int x = 0;
int y = 0;

int xOffset = 0;
int yOffset = OFFSETS[0];

int offsetIndex = 0;

typedef enum {EMPTY, PLACED, HIT, MISS} cellValue;

typedef struct cell {
    cellValue value;
    bool cursor;
    int id;
} cell;

cell lights[2][ROWS][COLUMNS] = {
    [0 ... 1] {
        [0 ... ROWS-1] = {
            [0 ... COLUMNS-1] = {EMPTY, false, 0}
        }
    }
};

#define myBoard lights[MY_PLAYER]
#define theirBoard lights[!MY_PLAYER]

enum modeStates {MODE_START, WAIT, PUSH, POSITION, WAIT_FOR_OTHER, PLAY} modeState;
enum placeStates {PLACE_START, PLACE_IDLE, PLACE_MOVE, ROTATE, PLACE, READY} placeState;
enum playStates {PLAY_START, PLAY_IDLE, PLAY_MOVE, ATTACK, WON, LOST} playState;

void WhichShipsSunk() {
    for (int i = 1; i <= NUM_SHIPS; i++) {
        int total = OFFSETS[i-1] + 1;
        int hits = 0;

        for (int j = 0; j < ROWS; j++) {
            for (int k = 0; k < COLUMNS; k++) {
                if ((lights[!MY_PLAYER][k][j].id == i) && (lights[!MY_PLAYER][k][j].value == HIT)) hits++;
            }
        }

        if (hits >= total) {
            printf("Ship #%d has sunk!\n", i);
        }
    }
}

void parse_msg(char *buf);

void SendAttack() {
    if (lights[!MY_PLAYER][y][x].value == HIT || lights[!MY_PLAYER][y][x].value == MISS) return;

    char msg[16];
    snprintf(msg, sizeof(msg), "ATK:%d,%d\n", x, y);
    printf("Sending attack: %s", msg);
    send_msg(msg);
    myTurn = false;
}

void HandleIncomingAttack(int ax, int ay) {
    printf("Incoming attack at %d,%d\n", ax, ay);
    char msg[16];
    if (lights[MY_PLAYER][ay][ax].value == PLACED) {
        lights[MY_PLAYER][ay][ax].value = HIT;
        myHits++;
        int id = lights[MY_PLAYER][ay][ax].id;
        snprintf(msg, sizeof(msg), "HIT:%d,%d,%d\n", ax, ay, id);
    } else {
        lights[MY_PLAYER][ay][ax].value = MISS;
        snprintf(msg, sizeof(msg), "MISS:%d,%d\n", ax, ay);
    }
    send_msg(msg);
 
    if (myHits >= SHIP_CELL_COUNT) {
        send_msg("WIN\n");
        playState = LOST;
    }
}

void HandleAttackResult(bool isHit, int ax, int ay, int id) {
    printf("Attack result %s at %d,%d\n", isHit ? "HIT" : "MISS", ax, ay);
    theirBoard[ay][ax].value = isHit ? HIT : MISS;
    theirBoard[ay][ax].id = id;
    lights[!MY_PLAYER][ay][ax].cursor = false;
    
    if (isHit) {
        theirHits++;
        WhichShipsSunk();
        if (theirHits >= SHIP_CELL_COUNT) {
            playState = WON;
            return;
        }
    }

    send_msg("NEXT\n");
    playState = PLAY_IDLE;
    myTurn = false;
}

void parse_msg(char *buf) {
    int ax, ay, id;
    printf("Parsing: %s\n", buf);
    if (strcmp(buf, "READY\n") == 0) {
        theyAreReady = true;
        printf("Opponent is ready!\n");
        if (iAmReady) {
            modeState = PLAY;
            printf("Both ready — starting game! myTurn=%d\n", myTurn);
        }
    } else if (sscanf(buf, "ATK:%d,%d", &ax, &ay) == 2) {
        HandleIncomingAttack(ax, ay);
    } else if (sscanf(buf, "HIT:%d,%d,%d", &ax, &ay, &id) == 3) {
        HandleAttackResult(true, ax, ay, id);
    } else if (sscanf(buf, "MISS:%d,%d", &ax, &ay) == 2) {
        HandleAttackResult(false, ax, ay, 0);
    } else if (strcmp(buf, "WIN\n") == 0) {
        playState = LOST;
    } else if (strcmp(buf, "NEXT\n") == 0) {
        myTurn = true;
        playState = PLAY_IDLE;
    }
}

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

void MoveCursor(int board) {

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[board][y + j][x + i].cursor = false;
    }

    if (direction == UP && y > 0) y--;
    else if (direction == DOWN && y < ROWS - 1 - yOffset) y++;
    else if (direction == LEFT && x > 0) x--;
    else if (direction == RIGHT && x < COLUMNS - 1 - xOffset) x++;

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[board][y + j][x + i].cursor = true;
    }
}

void DisplayPlacements() {
    gpio_put(2, lights[MY_PLAYER][0][0].cursor || lights[MY_PLAYER][0][0].value == PLACED);
    gpio_put(3, lights[MY_PLAYER][0][1].cursor || lights[MY_PLAYER][0][1].value == PLACED);
    gpio_put(4, lights[MY_PLAYER][0][2].cursor || lights[MY_PLAYER][0][2].value == PLACED);

    gpio_put(5, lights[MY_PLAYER][1][0].cursor || lights[MY_PLAYER][1][0].value == PLACED);
    gpio_put(6, lights[MY_PLAYER][1][1].cursor || lights[MY_PLAYER][1][1].value == PLACED);
    gpio_put(7, lights[MY_PLAYER][1][2].cursor || lights[MY_PLAYER][1][2].value == PLACED);

    gpio_put(8, lights[MY_PLAYER][2][0].cursor || lights[MY_PLAYER][2][0].value == PLACED);
    gpio_put(9, lights[MY_PLAYER][2][1].cursor || lights[MY_PLAYER][2][1].value == PLACED);
    gpio_put(10, lights[MY_PLAYER][2][2].cursor || lights[MY_PLAYER][2][2].value == PLACED);
}

void DisplayAttacks() {
    gpio_put(2, lights[!MY_PLAYER][0][0].cursor || lights[!MY_PLAYER][0][0].value == HIT || lights[!MY_PLAYER][0][0].value == MISS);
    gpio_put(3, lights[!MY_PLAYER][0][1].cursor || lights[!MY_PLAYER][0][1].value == HIT || lights[!MY_PLAYER][0][1].value == MISS);
    gpio_put(4, lights[!MY_PLAYER][0][2].cursor || lights[!MY_PLAYER][0][2].value == HIT || lights[!MY_PLAYER][0][2].value == MISS);

    gpio_put(5, lights[!MY_PLAYER][1][0].cursor || lights[!MY_PLAYER][1][0].value == HIT || lights[!MY_PLAYER][1][0].value == MISS);
    gpio_put(6, lights[!MY_PLAYER][1][1].cursor || lights[!MY_PLAYER][1][1].value == HIT || lights[!MY_PLAYER][1][1].value == MISS);
    gpio_put(7, lights[!MY_PLAYER][1][2].cursor || lights[!MY_PLAYER][1][2].value == HIT || lights[!MY_PLAYER][1][2].value == MISS);

    gpio_put(8, lights[!MY_PLAYER][2][0].cursor || lights[!MY_PLAYER][2][0].value == HIT || lights[!MY_PLAYER][2][0].value == MISS);
    gpio_put(9, lights[!MY_PLAYER][2][1].cursor || lights[!MY_PLAYER][2][1].value == HIT || lights[!MY_PLAYER][2][1].value == MISS);
    gpio_put(10, lights[!MY_PLAYER][2][2].cursor || lights[!MY_PLAYER][2][2].value == HIT || lights[!MY_PLAYER][2][2].value == MISS);
}

void PrintBoard(int board) {
    printf("\nBoard %d\n", board);

    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLUMNS; col++) {
            printf("(%d,%d) ",
                lights[board][row][col].value,
                lights[board][row][col].cursor);
        }
        printf("\n");
    }
    printf("\n");
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
            if (lights[MY_PLAYER][y + j][x + i].value == PLACED) return;
        }
    }

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) {
            lights[MY_PLAYER][y + j][x + i].value = PLACED;
            lights[MY_PLAYER][y + j][x + i].id = offsetIndex + 1;
        }
    }

    offsetIndex++;
    
    // Clear Cursor
    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) {
            lights[MY_PLAYER][y + j][x + i].cursor = false;
        }
    }
    x = 0;
    y = 0;

    if (offsetIndex < (int)NUM_SHIPS) {
        xOffset = 0;
        yOffset = OFFSETS[offsetIndex];
    } else {
        xOffset = 0;
        yOffset = 0;
    }
}

void RotateShip() {
    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[MY_PLAYER][y + j][x + i].cursor = false;
    }

    if (x + yOffset > COLUMNS - 1) x = COLUMNS - 1 - yOffset;
    if (y + xOffset > ROWS - 1) y = ROWS - 1 - xOffset;
    
    int temp = xOffset;
    xOffset = yOffset;
    yOffset = temp;

    for (int i = 0; i <= xOffset; i++) {
        for (int j = 0; j <= yOffset; j++) lights[MY_PLAYER][y + j][x + i].cursor = true;
    }
}


////////////////////////////////////////////////////////////////////////////////////


bool Tick(struct repeating_timer *t) {

    switch (modeState) {
        case MODE_START:
            modeState = WAIT;
            break;

        case WAIT:
            GetPress();
            if (place) modeState = PUSH;
            break;
        
        case PUSH:
            GetPress();
            if (!place) modeState = POSITION;
            break;

        case POSITION:
            switch (placeState) {

                case PLACE_START:
                    xOffset = 0;
                    yOffset = OFFSETS[0];
                    MoveCursor(MY_PLAYER);
                    placeState = PLACE_IDLE;
                    break;

                case PLACE_IDLE:
                    GetDirections();
                    GetPress();
                    if(direction && !rotate && !place) {
                        MoveCursor(MY_PLAYER);
                        placeState = PLACE_MOVE;
                    } else if (!direction && rotate && !place) {
                        RotateShip();
                        placeState = ROTATE;
                    } else if (!direction && !rotate && place) {
                        SetCell();
                        MoveCursor(MY_PLAYER);
                        placeState = PLACE;
                    }

                    if (offsetIndex >= (int)NUM_SHIPS) placeState = READY;
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
                    iAmReady = true;
                    send_msg("READY\n");
                    printf("I am ready. Waiting for opponent...\n");
                    modeState = WAIT_FOR_OTHER;
                    if (theyAreReady) {
                        modeState = PLAY;
                        printf("Both ready — starting game! myTurn=%d\n", myTurn);
                    }
                    break;

                default:
                    placeState = PLACE_START;
                    break;
            }
            DisplayPlacements();
            break;
        
        case WAIT_FOR_OTHER:
            DisplayPlacements();
            break;

        case PLAY:
            switch(playState) {
                case PLAY_START:
                    xOffset = 0;
                    yOffset = 0;
                    x = 0;
                    y = 0;
                    MoveCursor(!MY_PLAYER);
                    playState = PLAY_IDLE;
                    break;

                case PLAY_IDLE:
                    if (!myTurn) break;
                    GetDirections();
                    GetPress();
                    if(direction && !place) {
                        MoveCursor(!MY_PLAYER);
                        playState = PLAY_MOVE;
                    } else if (!direction && place) {
                        
                        SendAttack();
                        playState = ATTACK;
                    }
                    break;

                case PLAY_MOVE:
                    GetDirections();
                    if (!direction) playState = PLAY_IDLE;
                    break;

                case ATTACK:
                    break;

                case WON:
                    printf("YOU WON!\n");
                    break;

                case LOST:
                    printf("YOU LOST!\n");
                    break;   

                default:
                    playState = PLAY_START;
                    break;
            }
            DisplayAttacks();
            break;
        default:
            modeState = MODE_START;
            break;
    }

    return true;
}

// ─── TCP callbacks ─────────────────────────────────────────────────────────
static err_t on_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(pcb);
        client_pcb = NULL;
        return ERR_OK;
    }
 
    char buf[64] = {0};
    size_t len = p->tot_len < sizeof(buf) - 1 ? p->tot_len : sizeof(buf) - 1;
    pbuf_copy_partial(p, buf, len, 0);
    pbuf_free(p);
    tcp_recved(pcb, len);
 
    parse_msg(buf);
    return ERR_OK;
}
 
static err_t on_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || !newpcb) return ERR_VAL;
    printf("Client connected!\n");
    client_pcb = newpcb;
    tcp_recv(newpcb, on_recv);
    return ERR_OK;
}

////////////////////////////////////////////////////////////////////////////////////

int main() {
    
    stdio_init_all();

    sleep_ms(5000);
    printf("STARTING\n");

    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    gpio_init(22); gpio_set_dir(22, GPIO_IN);
    gpio_init(21); gpio_set_dir(21, GPIO_IN);

    for (int i = 2; i <= 10; i++) { 
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
    }

    // WiFi init
    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi...\n");

    int wifi_result = cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000);
    if (wifi_result != 0) {
        printf("WiFi failed, error: %d\n", wifi_result);
        return 1;
    }

    printf("Connected! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));

    // Start TCP server
    struct tcp_pcb *server = tcp_new_ip_type(IPADDR_TYPE_ANY);
    tcp_bind(server, IP_ADDR_ANY, SERVER_PORT);
    server = tcp_listen_with_backlog(server, 1);
    tcp_accept(server, on_accept);
    printf("Listening on port %d\n", SERVER_PORT);
    
    modeState = MODE_START;
    placeState = PLACE_START;
    playState = PLAY_START;

    for (int i = 0; i < (int)NUM_SHIPS; i++) {
        SHIP_CELL_COUNT += OFFSETS[i] + 1;
    }

    struct repeating_timer timer;
    add_repeating_timer_ms(-100, Tick, NULL, &timer);
    while (1) {
        cyw43_arch_poll();
        sleep_ms(1);
    }

    cyw43_arch_deinit();
    return 0;
}