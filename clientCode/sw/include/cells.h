#ifndef _CELLS_H_
#define _CELLS_H_

#define ROWS 10
#define COLUMNS 10

typedef enum {EMPTY, PLACED, HIT, MISS} cellValue;

typedef struct cell {
    cellValue value;
    bool cursor;
    int id;
} cell;

#endif
