#include <lvgl_example_grid.h>
#include "lv_conf.h"
#include <lvgl.h>

namespace ucr { namespace bcoe { namespace cs { namespace cs122 {

#if LV_USE_FLOAT
    #define my_PRIprecise "f"
#else
    #define my_PRIprecise LV_PRId32
#endif

LVGL_Example_Grid::LVGL_Example_Grid(SPIDisplay *spi_disp,
                                     lv_display_flush_cb_t fcallback,
                                     lv_tick_get_cb_t tcallback,
                                     const cell board[ROWS][COLUMNS])                               
    : CS122_App(spi_disp, fcallback, tcallback),
      board_(board),
      playerBanner_(nullptr),
      turnBanner_(nullptr),
      player1ShipsPanel_(nullptr),
      player2ShipsPanel_(nullptr),
      playerBannerLabel_(nullptr),
      turnBannerLabel_(nullptr),
      player1ShipsLabel_(nullptr),
      player2ShipsLabel_(nullptr) {
        for(int ship = 0; ship < 5; ship++) {
            for(int segment = 0; segment < 5; segment++) {
                player1ShipSegments_[ship][segment] = nullptr;
                player2ShipSegments_[ship][segment] = nullptr;
            }
        }
    }

    lv_obj_t *LVGL_Example_Grid::createInfoSection(lv_obj_t *parent,
                                                   int32_t x,
                                                   int32_t y,
                                                   int32_t w,
                                                   int32_t h) {
        lv_obj_t *section = lv_obj_create(parent);
        lv_obj_set_pos(section, x, y);
        lv_obj_set_size(section, w, h);
        lv_obj_set_style_radius(section, 0, 0);
        lv_obj_set_style_pad_all(section, 0, 0);
        lv_obj_set_style_margin_all(section, 0, 0);
        lv_obj_set_style_border_color(section, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_border_width(section, 1, 0);
        lv_obj_set_style_bg_color(section, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(section, LV_OPA_COVER, 0);
        lv_obj_set_scrollbar_mode(section, LV_SCROLLBAR_MODE_OFF);
        return section;
    }

    void LVGL_Example_Grid::createShipSegments(lv_obj_t *parent, lv_obj_t *segments[5][5]) {
        static const int shipSizes[5] = {5, 4, 3, 3, 2};
        static const int shipY[5] = {17, 51, 85, 119, 153};

        for(int ship = 0; ship < 5; ship++) {
            for(int segment = 0; segment < 5; segment++) {
                if(segment >= shipSizes[ship]) {
                    segments[ship][segment] = nullptr;
                    continue;
                }

                lv_obj_t *box = lv_obj_create(parent);
                lv_obj_set_pos(box, 10 + segment * 17, shipY[ship]);
                lv_obj_set_size(box, 17, 17);
                lv_obj_set_style_radius(box, 0, 0);
                lv_obj_set_style_pad_all(box, 0, 0);
                lv_obj_set_style_margin_all(box, 0, 0);
                lv_obj_set_style_border_color(box, lv_color_hex(0x000000), 0);
                lv_obj_set_style_border_width(box, 1, 0);
                lv_obj_set_style_bg_color(box, lv_color_hex(0x6F7D8C), 0);
                lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
                lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
                segments[ship][segment] = box;
            }
        }
    }

    void LVGL_Example_Grid::createShipStatusGraphics() {
        createShipSegments(player1ShipsPanel_, player1ShipSegments_);
        createShipSegments(player2ShipsPanel_, player2ShipSegments_);
    }

    void LVGL_Example_Grid::setShipSegmentsColor(lv_obj_t *segments[5][5],
                                                 int shipIndex,
                                                 uint32_t color) {
        if(shipIndex < 0 || shipIndex >= 5) {
            return;
        }

        for(int segment = 0; segment < 5; segment++) {
            if(segments[shipIndex][segment] != nullptr) {
                lv_obj_set_style_bg_color(segments[shipIndex][segment], lv_color_hex(color), 0);
            }
        }
    }

    void LVGL_Example_Grid::createGrid() {
        lv_obj_t * screen = lv_screen_active();
        lv_obj_set_style_pad_all(screen, 0, 0);
        lv_obj_set_style_border_width(screen, 0, 0);
        lv_obj_set_style_margin_all(screen, 0, 0);
        lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

        playerBanner_ = createInfoSection(screen, 270, 0, 210, 42);
        turnBanner_ = createInfoSection(screen, 270, 42, 210, 42);
        player1ShipsPanel_ = createInfoSection(screen, 270, 84, 105, 187);
        player2ShipsPanel_ = createInfoSection(screen, 375, 84, 105, 187);
        createShipStatusGraphics();

        playerBannerLabel_ = lv_label_create(playerBanner_);
        lv_obj_center(playerBannerLabel_);
        lv_obj_set_style_text_color(playerBannerLabel_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(playerBannerLabel_, &lv_font_montserrat_18, 0);

        turnBannerLabel_ = lv_label_create(turnBanner_);
        lv_obj_center(turnBannerLabel_);
        lv_obj_set_style_text_color(turnBannerLabel_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(turnBannerLabel_, &lv_font_montserrat_18, 0);

        player1ShipsLabel_ = lv_label_create(screen);
        lv_obj_set_pos(player1ShipsLabel_, 345, 242);
        lv_obj_set_size(player1ShipsLabel_, 25, 25);
        lv_label_set_text(player1ShipsLabel_, "P1");
        lv_obj_set_style_text_color(player1ShipsLabel_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(player1ShipsLabel_, &lv_font_montserrat_18, 0);

        player2ShipsLabel_ = lv_label_create(screen);
        lv_obj_set_pos(player2ShipsLabel_, 450, 242);
        lv_obj_set_size(player2ShipsLabel_, 25, 25);
        lv_label_set_text(player2ShipsLabel_, "P2");
        lv_obj_set_style_text_color(player2ShipsLabel_, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(player2ShipsLabel_, &lv_font_montserrat_18, 0);

        /* Main grid container with fixed descriptors */
        lv_obj_t * container = lv_obj_create(screen);
        static const int32_t container_style_grid_column_dsc_array_0[] = {27, 27, 27, 27, 27, 27, 27, 27, 27, 27, LV_GRID_TEMPLATE_LAST};
        lv_obj_set_style_grid_column_dsc_array(container, container_style_grid_column_dsc_array_0, 0);
        static const int32_t container_style_grid_row_dsc_array_1[] = {27, 27, 27, 27, 27, 27, 27, 27, 27, 27, LV_GRID_TEMPLATE_LAST};
        lv_obj_set_style_grid_row_dsc_array(container, container_style_grid_row_dsc_array_1, 0);
        lv_obj_set_pos(container, 0, 0);
        lv_obj_set_style_pad_all(container, 0, 0);
        lv_obj_set_style_pad_row(container, 0, 0);
        lv_obj_set_style_pad_column(container, 0, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_style_margin_all(container, 0, 0);
        lv_obj_set_style_radius(container, 0, 0);
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_layout(container, LV_LAYOUT_GRID, 0);
        lv_obj_set_size(container, 270, 270);

        for(int row = 0; row < ROWS; row++) {
            for(int col = 0; col < COLUMNS; col++) {
                lv_obj_t * cell_obj = lv_obj_create(container);
                cells_[row][col] = cell_obj;
                
                lv_obj_set_style_radius(cell_obj, 0, 0);
                lv_obj_set_style_pad_all(cell_obj, 0, 0);
                lv_obj_set_style_margin_all(cell_obj, 0, 0);
                lv_obj_set_style_outline_width(cell_obj, 0, 0);
                lv_obj_set_style_shadow_width(cell_obj, 0, 0);
                
                lv_obj_set_style_grid_cell_x_align(cell_obj, LV_GRID_ALIGN_STRETCH, 0);
                lv_obj_set_style_grid_cell_column_pos(cell_obj, col, 0);

                lv_obj_set_style_grid_cell_y_align(cell_obj, LV_GRID_ALIGN_STRETCH, 0);
                lv_obj_set_style_grid_cell_row_pos(cell_obj, row, 0);

                lv_obj_set_style_bg_color(cell_obj, lv_color_hex(0x0B4F8A), 0);
                lv_obj_set_style_border_color(cell_obj, lv_color_hex(0x000000), 0);
                lv_obj_set_style_border_width(cell_obj, 1, 0);
                lv_obj_set_style_bg_opa(cell_obj, LV_OPA_COVER, 0);
            }
        }
    }

    void LVGL_Example_Grid::renderBoard(const cell board[ROWS][COLUMNS], ViewMode viewMode) {
        for(int row = 0; row < ROWS; row++) {
            for(int col = 0; col < COLUMNS; col++) {
                lv_obj_t * cell_obj = cells_[row][col];

                uint32_t color = 0x0B4F8A;
                cellValue value = board[row][col].value;

                if (viewMode == ViewMode::Win) {
                    color = 0x1F9D55;
                }
                else if (viewMode == ViewMode::Lose) {
                    color = 0xD62828;
                }
                else if (value == PLACED &&
                    (viewMode == ViewMode::Placement || viewMode == ViewMode::Defense)) {
                    color = 0x6F7D8C;
                }
                else if (value == HIT) {
                    color = 0xD62828;
                }
                else if (value == MISS) {
                    color = 0xE9EEF2;
                }

                lv_obj_set_style_bg_color(cell_obj, lv_color_hex(color), 0);

                if (board[row][col].cursor) {
                    lv_obj_set_style_border_color(cell_obj, lv_color_hex(0xFFFF00), 0);
                    lv_obj_set_style_border_width(cell_obj, 3, 0);
                }
                else {
                    lv_obj_set_style_border_color(cell_obj, lv_color_hex(0x000000), 0);
                    lv_obj_set_style_border_width(cell_obj, 1, 0);
                }
            }
        }
    }   

    void LVGL_Example_Grid::renderStatus() {
        if(playerBannerLabel_ == nullptr || turnBannerLabel_ == nullptr) {
            return;
        }

        int attackingPlayer = player_ ? 2 : 1;
        const char *modeText = "PLACE";
        if (viewMode_ == ViewMode::Placement) {
            modeText = "PLACE";
        } else if (viewMode_ == ViewMode::Attack) {
            modeText = "ATTACK";
            attackingPlayer = player_ ? 2 : 1;
        } else if (viewMode_ == ViewMode::Defense) {
            modeText = "ATTACK";
            attackingPlayer = player_ ? 1 : 2;
        } else if (viewMode_ == ViewMode::Win) {
            modeText = "WIN";
        } else if (viewMode_ == ViewMode::Lose) {
            modeText = "LOSE";
        }

        lv_label_set_text_fmt(playerBannerLabel_, "PLAYER %d", player_ ? 2 : 1);
        if (viewMode_ == ViewMode::Attack || viewMode_ == ViewMode::Defense) {
            lv_label_set_text_fmt(turnBannerLabel_, "P%d %s", attackingPlayer, modeText);
        } else {
            lv_label_set_text(turnBannerLabel_, modeText);
        }
        lv_obj_center(playerBannerLabel_);
        lv_obj_center(turnBannerLabel_);
    }

    void LVGL_Example_Grid::DisplayPlacements(const cell board[ROWS][COLUMNS], bool player) {
        board_ = board;
        viewMode_ = ViewMode::Placement;
        player_ = player;
        redraw_pending_ = true;
    }

    void LVGL_Example_Grid::DisplayAttacks(const cell board[ROWS][COLUMNS], bool player) {
        board_ = board;
        viewMode_ = ViewMode::Attack;
        player_ = player;
        redraw_pending_ = true;
    }

    void LVGL_Example_Grid::DisplayDefense(const cell board[ROWS][COLUMNS], bool player) {
        board_ = board;
        viewMode_ = ViewMode::Defense;
        player_ = player;
        redraw_pending_ = true;
    }

    void LVGL_Example_Grid::DisplayWin(bool player) {
        viewMode_ = ViewMode::Win;
        player_ = player;
        redraw_pending_ = true;
    }

    void LVGL_Example_Grid::DisplayLose(bool player) {
        viewMode_ = ViewMode::Lose;
        player_ = player;
        redraw_pending_ = true;
    }

    void LVGL_Example_Grid::SetShipSunk(bool player, int shipId, bool sunk) {
        uint32_t color = sunk ? 0x8B0000 : 0x6F7D8C;
        lv_obj_t *(*segments)[5] = player ? player2ShipSegments_ : player1ShipSegments_;
        setShipSegmentsColor(segments, shipId - 1, color);
    }

    void LVGL_Example_Grid::redrawTimerCallback(lv_timer_t *timer) {
        LVGL_Example_Grid *self =
            static_cast<LVGL_Example_Grid *>(lv_timer_get_user_data(timer));

        self->applyPendingRender();
    }

    void LVGL_Example_Grid::applyPendingRender() {
        if(!redraw_pending_) {
            return;
        }

        redraw_pending_ = false;
        renderStatus();
        renderBoard(board_, viewMode_);
    }

    uint32_t LVGL_Example_Grid::run() {
        createGrid();
        renderStatus();
        renderBoard(board_, viewMode_);

        lv_timer_create(redrawTimerCallback, 10, this);

        return loop();
    }

}}}}
