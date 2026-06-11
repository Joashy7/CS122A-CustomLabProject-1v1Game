#ifndef _LVGL_EXAMPLE_GRID_H_
#define _LVGL_EXAMPLE_GRID_H_
        
#include <cs122_app.h>
#include "cells.h"

namespace ucr { namespace bcoe { namespace cs { namespace cs122 {
    class LVGL_Example_Grid : CS122_App {
    public:
        LVGL_Example_Grid(SPIDisplay *spi_disp,
                        lv_display_flush_cb_t fcallback,
                        lv_tick_get_cb_t tcallback,
                        const cell board[ROWS][COLUMNS]);

        uint32_t run() override;

        void DisplayPlacements(const cell board[ROWS][COLUMNS], bool player);
        void DisplayAttacks(const cell board[ROWS][COLUMNS], bool player);
        void DisplayDefense(const cell board[ROWS][COLUMNS], bool player);
        void DisplayWin(bool player);
        void DisplayLose(bool player);
        void SetShipSunk(bool player, int shipId, bool sunk);

    private:
        enum class ViewMode { Placement, Attack, Defense, Win, Lose };

        const cell (*board_)[COLUMNS];
        lv_obj_t *cells_[ROWS][COLUMNS];
        lv_obj_t *playerBanner_;
        lv_obj_t *turnBanner_;
        lv_obj_t *player1ShipsPanel_;
        lv_obj_t *player2ShipsPanel_;
        lv_obj_t *playerBannerLabel_;
        lv_obj_t *turnBannerLabel_;
        lv_obj_t *player1ShipsLabel_;
        lv_obj_t *player2ShipsLabel_;
        lv_obj_t *player1ShipSegments_[5][5];
        lv_obj_t *player2ShipSegments_[5][5];

        ViewMode viewMode_ = ViewMode::Placement;
        bool player_ = false;
        volatile bool redraw_pending_ = false;

        static void redrawTimerCallback(lv_timer_t *timer);
        void applyPendingRender();

        void createGrid();
        lv_obj_t *createInfoSection(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h);
        void createShipStatusGraphics();
        void createShipSegments(lv_obj_t *parent, lv_obj_t *segments[5][5]);
        void setShipSegmentsColor(lv_obj_t *segments[5][5], int shipIndex, uint32_t color);
        void renderBoard(const cell board[ROWS][COLUMNS], ViewMode viewMode);
        void renderStatus();
    };
}}}}
#endif
