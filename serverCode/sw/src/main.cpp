#include "spi_display.h"
#include "lv_conf.h"
#include "lvgl_example_grid.h"
#include "battleship.h"
#include <pico/stdio_usb.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <string.h>

#ifndef WIFI_SSID
#define WIFI_SSID "HMI"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "thisismypassword"
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 4242
#endif

ucr::bcoe::SPIDisplay spi_display(480, 272, 10000000, 20);
static ucr::bcoe::cs::cs122::LVGL_Example_Grid *app = nullptr;
static struct tcp_pcb *client_pcb = nullptr;
static struct tcp_pcb *server_pcb = nullptr;

/*Return the elapsed milliseconds since startup.
 *It needs to be implemented by the user*/
uint32_t cs122_get_millis(void) {
    return to_ms_since_boot(get_absolute_time());
}

static uint8_t buffer[OLEDRGB_WIDTH * OLEDRGB_HEIGHT / 10];

/*Copy the rendered image to the screen. */
void cs122_flush_cb_direct(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf) {
    ucr::bcoe::SPIDisplay *spi_display = reinterpret_cast<ucr::bcoe::SPIDisplay *>(lv_display_get_user_data(disp));
	uint32_t i = 0;
	for (uint32_t y = area->y1; y <= area->y2; y++) {
		for(uint32_t x = area->x1; x <= area->x2; x++) {
			uint32_t px_buf_idx = x * 2 + y * (spi_display->getWidth() * 2);
		    buffer[i++] =  (px_buf[px_buf_idx+1] & 0xE0) | ((px_buf[px_buf_idx+1] & 0x7) << 2) | (px_buf[px_buf_idx] & 0x1f) >> 3;
		}
	}

    /*Show the rendered image on the display*/
    spi_display->drawBitmap(area->x1, area->y1, area->x2, area->y2, buffer);

    /*Indicate that the buffer is available.
     *If DMA were used, call in the DMA complete interrupt*/
    lv_display_flush_ready(disp);
}

/*It needs to be implemented by the user*/
void cs122_flush_cb_partial(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf) {
	uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

    /*Show the rendered image on the display*/
    ucr::bcoe::SPIDisplay *spi_display = reinterpret_cast<ucr::bcoe::SPIDisplay *>(lv_display_get_user_data(disp));
    spi_display->drawBitmap(2 * area->x1, area->y1, 2 * area->x2+1, area->y2, px_buf);

    /*Indicate that the buffer is available.
     *If DMA were used, call in the DMA complete interrupt*/
    lv_display_flush_ready(disp);
}

void DisplayPlacements(bool boardIndex) {
    app->DisplayPlacements(playerBoard[boardIndex], boardIndex);
}

void DisplayAttacks(bool boardIndex) {
    app->DisplayAttacks(playerBoard[boardIndex], player);
}

void DisplayDefense(bool boardIndex) {
    app->DisplayDefense(playerBoard[boardIndex], boardIndex);
}

void DisplayWin() {
    app->DisplayWin(MY_PLAYER);
}

void DisplayLose() {
    app->DisplayLose(MY_PLAYER);
}

void SetShipSunk(bool player, int shipId, bool sunk) {
    app->SetShipSunk(player, shipId, sunk);
}

static void SendMessage(const char *msg) {
    if (client_pcb == nullptr) {
        printf("No TCP client, dropping message: %s", msg);
        return;
    }

    err_t err = tcp_write(client_pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        tcp_output(client_pcb);
    } else {
        printf("tcp_write failed: %d\n", err);
    }
}

void SendReady() {
    SendMessage("READY\n");
}

void SendAttack() {
    bool target = !MY_PLAYER;

    if (playerBoard[target][y][x].value == HIT ||
        playerBoard[target][y][x].value == MISS) {
        return;
    }

    char msg[24];
    snprintf(msg, sizeof(msg), "ATK:%d,%d\n", x, y);
    printf("Sending attack: %s", msg);
    SendMessage(msg);
    myTurn = false;
    playState = ATTACK;
}

static int CountShipHits(bool boardIndex, int shipId) {
    int hits = 0;
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLUMNS; col++) {
            if (playerBoard[boardIndex][row][col].id == shipId &&
                playerBoard[boardIndex][row][col].value == HIT) {
                hits++;
            }
        }
    }

    return hits;
}

static bool IsShipSunk(bool boardIndex, int shipId) {
    if (shipId < 1 || shipId > ShipCount()) {
        return false;
    }

    return CountShipHits(boardIndex, shipId) >= OFFSETS[shipId - 1] + 1;
}

static void MarkShipSunkIfNeeded(bool boardIndex, int shipId) {
    if (shipId < 1 || shipId > ShipCount()) {
        return;
    }

    int shipIndex = shipId - 1;
    if (!sunkShips[boardIndex][shipIndex] && IsShipSunk(boardIndex, shipId)) {
        sunkShips[boardIndex][shipIndex] = true;
        SetShipSunk(boardIndex, shipId, true);
        printf("Player %d ship #%d has sunk!\n", boardIndex ? 2 : 1, shipId);
    }
}

static void HandleIncomingAttack(int ax, int ay) {
    if (ax < 0 || ax >= COLUMNS || ay < 0 || ay >= ROWS) {
        return;
    }

    printf("Incoming attack at %d,%d\n", ax, ay);

    char msg[32];
    cell *target = &playerBoard[MY_PLAYER][ay][ax];
    if (target->value == PLACED) {
        target->value = HIT;
        myHits++;
        snprintf(msg, sizeof(msg), "HIT:%d,%d,%d\n", ax, ay, target->id);
        MarkShipSunkIfNeeded(MY_PLAYER, target->id);
    } else {
        target->value = MISS;
        snprintf(msg, sizeof(msg), "MISS:%d,%d\n", ax, ay);
    }

    SendMessage(msg);
    RequestDisplayUpdate();

    if (myHits >= shipCellCount) {
        SendMessage("WIN\n");
        playState = LOST;
        printf("Lose\n");
        RequestDisplayUpdate();
    }
}

static void HandleAttackResult(bool isHit, int ax, int ay, int id) {
    if (ax < 0 || ax >= COLUMNS || ay < 0 || ay >= ROWS) {
        return;
    }

    printf("Attack result %s at %d,%d\n", isHit ? "HIT" : "MISS", ax, ay);

    cell *target = &playerBoard[!MY_PLAYER][ay][ax];
    target->value = isHit ? HIT : MISS;
    target->id = id;
    target->cursor = false;

    ClearCursor(!MY_PLAYER);

    if (isHit) {
        theirHits++;
        MarkShipSunkIfNeeded(!MY_PLAYER, id);
        if (theirHits >= shipCellCount) {
            SendMessage("LOSE\n");
            playState = WON;
            printf("Win\n");
            RequestDisplayUpdate();
            return;
        }

        myTurn = true;
        playState = PLAY_IDLE;
        MoveCursor(!MY_PLAYER);
        RequestDisplayUpdate();
        return;
    }

    SendMessage("NEXT\n");
    playState = PLAY_IDLE;
    myTurn = false;
    RequestDisplayUpdate();
}

static void ParseMessage(char *buf) {
    int ax = 0;
    int ay = 0;
    int id = 0;

    printf("Parsing: %s\n", buf);

    if (strcmp(buf, "READY") == 0) {
        theyAreReady = true;
        printf("Opponent is ready!\n");
        if (iAmReady) {
            modeState = PLAY;
            playState = PLAY_START;
            RequestDisplayUpdate();
            printf("Both ready, starting game. myTurn=%d\n", myTurn);
        }
    } else if (sscanf(buf, "ATK:%d,%d", &ax, &ay) == 2) {
        HandleIncomingAttack(ax, ay);
    } else if (sscanf(buf, "HIT:%d,%d,%d", &ax, &ay, &id) == 3) {
        HandleAttackResult(true, ax, ay, id);
    } else if (sscanf(buf, "MISS:%d,%d", &ax, &ay) == 2) {
        HandleAttackResult(false, ax, ay, 0);
    } else if (strcmp(buf, "WIN") == 0) {
        playState = WON;
        printf("Win\n");
        RequestDisplayUpdate();
    } else if (strcmp(buf, "LOSE") == 0) {
        playState = LOST;
        printf("Lose\n");
        RequestDisplayUpdate();
    } else if (strcmp(buf, "NEXT") == 0) {
        myTurn = true;
        playState = PLAY_IDLE;
        MoveCursor(!MY_PLAYER);
        RequestDisplayUpdate();
    }
}

static void ParseMessages(char *buf) {
    char *line = buf;
    while (*line != '\0') {
        char *next = strchr(line, '\n');
        if (next != nullptr) {
            *next = '\0';
        }

        if (*line != '\0') {
            ParseMessage(line);
        }

        if (next == nullptr) {
            break;
        }
        line = next + 1;
    }
}

static err_t OnReceive(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (p == nullptr) {
        tcp_close(pcb);
        if (client_pcb == pcb) {
            client_pcb = nullptr;
        }
        return ERR_OK;
    }

    char buf[128] = {};
    size_t len = p->tot_len < sizeof(buf) - 1 ? p->tot_len : sizeof(buf) - 1;
    pbuf_copy_partial(p, buf, len, 0);
    pbuf_free(p);
    tcp_recved(pcb, len);

    ParseMessages(buf);
    return ERR_OK;
}

static err_t OnAccept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == nullptr) {
        return ERR_VAL;
    }

    printf("Client connected!\n");
    client_pcb = newpcb;
    tcp_recv(newpcb, OnReceive);
    return ERR_OK;
}

static bool StartTcpServer() {
    server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (server_pcb == nullptr) {
        printf("tcp_new_ip_type failed\n");
        return false;
    }

    err_t err = tcp_bind(server_pcb, IP_ADDR_ANY, SERVER_PORT);
    if (err != ERR_OK) {
        printf("tcp_bind failed: %d\n", err);
        return false;
    }

    server_pcb = tcp_listen_with_backlog(server_pcb, 1);
    if (server_pcb == nullptr) {
        printf("tcp_listen_with_backlog failed\n");
        return false;
    }

    tcp_accept(server_pcb, OnAccept);
    printf("Listening on port %d\n", SERVER_PORT);
    return true;
}

void DisplayCurrentState() {
    switch (modeState) {
        case WAIT:
            DisplayPlacements(MY_PLAYER);
            break;
        case POSITION:
            DisplayPlacements(MY_PLAYER);
            break;
        case WAIT_FOR_OTHER:
            DisplayPlacements(MY_PLAYER);
            break;
        case PLAY:
            if (playState == WON) {
                DisplayWin();
            } else if (playState == LOST) {
                DisplayLose();
            } else if (myTurn || playState == ATTACK) {
                DisplayAttacks(!MY_PLAYER);
            } else {
                DisplayDefense(MY_PLAYER);
            }
            break;
        default:
            DisplayPlacements(MY_PLAYER);
            break;
    }
}

void Tick() {

    switch (modeState) {
        case MODE_START:
            player = MY_PLAYER;
            modeState = WAIT;
            RequestDisplayUpdate();
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
                    LoadCurrentShipSize(player);
                    MoveCursor(player);
                    placeState = PLACE_IDLE;
                    break;
                case PLACE_IDLE:
                    GetDirections();
                    GetPress();
                    if(direction && !rotate && !place) {
                        placeState = PLACE_MOVE;
                        MoveCursor(player);
                    } else if (!direction && rotate && !place) {
                        placeState = ROTATE;
                        RotateShip();
                    } else if (!direction && !rotate && place) {
                        placeState = PLACE;
                        SetCell();
                        if (ShipsRemaining(player)) MoveCursor(player);
                    } else if (!ShipsRemaining(player)) {                       
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
                    if (!iAmReady) {
                        iAmReady = true;
                        SendReady();
                        printf("I am ready. Waiting for opponent...\n");
                    }

                    if (theyAreReady) {
                        modeState = PLAY;
                        playState = PLAY_START;
                        printf("Both ready, starting game. myTurn=%d\n", myTurn);
                    } else {
                        modeState = WAIT_FOR_OTHER;
                    }
                    RequestDisplayUpdate();
                    break;
                default:
                    placeState = PLACE_START;
                    break;
            }
            break;
        case WAIT_FOR_OTHER:
            if (theyAreReady) {
                modeState = PLAY;
                playState = PLAY_START;
                RequestDisplayUpdate();
            }
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
                    if (!myTurn) {
                        break;
                    }

                    GetDirections();
                    GetPress();
                    if(direction && !place) {
                        playState = PLAY_MOVE;
                        MoveCursor(!MY_PLAYER);
                    } else if (!direction && place) {
                        SendAttack();
                    }
                    break;
                case PLAY_MOVE:
                    GetDirections();
                    if (!direction) playState = PLAY_IDLE;
                    break;
                case ATTACK:
                    break;
                case WON:
                    break;
                case LOST:
                    break;
                default:
                    playState = PLAY_START;
                    break;
            }
            break;
        default:
            modeState = MODE_START;
            break;
    }

    if (ConsumeDisplayUpdate()) {
        printf("Display update: mode=%d placeState=%d playState=%d player=%d place=%d direction=%d\n",
               modeState, placeState, playState, player, place, direction);
        DisplayCurrentState();
    }
}

void TickTimerCallback(lv_timer_t *timer) {
    Tick();
}

void NetworkPollTimerCallback(lv_timer_t *timer) {
    cyw43_arch_poll();
}

int main(void) {
    // Init drivers
	stdio_init_all();
    adc_init();

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");
    int wifiResult = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (wifiResult != 0) {
        printf("WiFi failed, error: %d\n", wifiResult);
        return 1;
    }

    printf("Connected! IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    if (!StartTcpServer()) {
        return 1;
    }

    adc_gpio_init(26);
    adc_gpio_init(27);

    gpio_init(22); gpio_set_dir(22, false); gpio_pull_down(22);
    gpio_init(21); gpio_set_dir(21, false); gpio_pull_down(21);

	spi_display.begin();
	spi_display.clear();

    modeState = MODE_START;
    placeState = PLACE_START;
    playState = PLAY_START;
    shipCellCount = TotalShipCellCount();

    static ucr::bcoe::cs::cs122::LVGL_Example_Grid grid_app(
        &spi_display,
        cs122_flush_cb_partial,
        cs122_get_millis,
        playerBoard[0]
    );

    app = &grid_app;

    lv_timer_create(TickTimerCallback, 20, NULL);
    lv_timer_create(NetworkPollTimerCallback, 1, NULL);
    uint32_t result = grid_app.run();
    cyw43_arch_deinit();
    return result;

}
