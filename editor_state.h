#include <termios.h>

struct editor_config{
    int screen_rows;
    int screen_columns;
    struct termios orig_termios;
};

struct editor_config e_config;

struct abuf{
    char *b;
    int len;
};