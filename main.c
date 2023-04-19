/*** inculdes ***/
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#include "editor_state.h"
/*** macros ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define KILO_VERSION "0.0.1"


/*** data ***/
struct termios orig_termios;

/*** terminal ***/
void die(const char *s){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disable_rawmode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &e_config.orig_termios);
    die("tcsetattr");
}

// Flags which are removed or terminal features which are unset
// IXON: When set, this flag enables software flow control using CTRL-S (XOFF) and CTRL-Q (XON) characters. By clearing this flag, we disable this feature.
// ICRNL: When set, the CR (carriage return) character is translated to NL (newline). By clearing this flag, we disable this translation.
// ECHO: When set, the characters that the user types are echoed back to the terminal. By clearing this flag, we disable this behavior.
// ICANON: When set, canonical input processing is enabled. This means that the input is line-buffered, and input is not sent to the program until the user hits ENTER. By clearing this flag, we disable this behavior.
// ISIG: When set, signals such as CTRL-C and CTRL-Z are sent to the program. By clearing this flag, we disable this behavior.
// IEXTEN: This flag is used to enable implementation-defined input processing. By clearing this flag, we disable this feature.
void enable_rawmode(){
    if(tcgetattr(STDIN_FILENO, &e_config.orig_termios) == -1) 
        die("tcgetattr");

    //on exit from main or by calling exit() call disable_rawmode function
    atexit(disable_rawmode);

    struct termios raw = e_config.orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON | BRKINT | ISTRIP | INPCK);
    raw.c_oflag &= ~(OPOST);
    raw.c_oflag |= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

char editor_keys(){
    int n_read;
    char char_read;

    while((n_read = read(STDIN_FILENO, &char_read, 1)) != 1){
        if(n_read == -1 && errno != EAGAIN)
            die("read");
    }

    return char_read;
}

int get_cursor_position(int *row, int *cols){

    char buffer[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\xb[6n", 4) != 4)
        return -1;
    
    while(i < sizeof(buffer)){

        if(read(STDIN_FILENO, &buffer[i], 1) != 1)
            break;
        
        if(buffer[i] == 'R')
            break;
        i++;
    }

    buffer[i] = '\0';

    if(buffer[0] != '\x1b' || buffer[1] != '[') return -1;
    if(sscanf(&buffer[2], "%d;%d", row, cols) != 2) return -1;

    // /printf("\r\n&buffer[1]: '%s'\r\n", & buffer[1]);

    return 0;
}

int get_window_size(int *rows, int *columns){
    struct winsize ws;

    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){

        //\x1b[999C command moves the cursor to the right and 
        //\x1b[999B command moves the cursor to down
        //999 value in each command is used as a large value
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) 
            return -1;

        return get_cursor_position(rows, columns);
    }
    else{
        *columns = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** append buffer ***/
#define ABUF_INIT {NULL, 0}

void ab_Append(struct abuf *ab, const char *s, int len){
    char *new_char = (char*)realloc(ab->b, ab->len + len);

    if(new_char == NULL)
        return;
    memcpy(&new_char[ab->len], s, len);
    ab->b = new_char;
    ab->len += len;
}

void ab_Free(struct abuf *ab){
    free(ab->b);
}

/*** input ***/
void editor_process_keypress(){
    char input_char = editor_keys();

    switch(input_char){
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/*** output ***/
void editor_draw_rows(struct abuf *ab){
    int y_axis;
    for(y_axis =0; y_axis < e_config.screen_rows; ++y_axis){
        if( y_axis == e_config.screen_rows / 3){
            char welcome[80];

            int welcome_len = snprintf(welcome, sizeof(welcome), "Kilo Editor -- version %s", KILO_VERSION);
            if(welcome_len > e_config.screen_columns)
                welcome_len = e_config.screen_columns;
            
            int padding = (e_config.screen_columns - welcome_len) / 2;

            if(padding){
                ab_Append(ab, "~", 1);
                padding--;
            }
            while(padding--){
                ab_Append(ab, " ", 1);
            }
            ab_Append(ab, welcome, welcome_len);
        }   
        else{
            ab_Append(ab, "~", 1);
        }
        //write(STDOUT_FILENO, "~", 1);
        ab_Append(ab, "~" ,1);

        ab_Append(ab, "\x1b[K", 3);
        if(y_axis < e_config.screen_rows - 1){
            //write(STDOUT_FILENO, "\r\n", 2);
            ab_Append(ab, "\r\n" ,2);  
        }
    }
}
void editor_refresh_screen(){
    struct abuf ab = ABUF_INIT;

    ab_Append(&ab, "\x1b[?25l", 6);
    //Writing 4 bytes to terminal 
    //first byte is \x1b, which is the escape sequence or 27 in decimal
    //J is a command which takes arguments
    ab_Append(&ab, "\x1b[H", 3);

    //draw tildes at the left most column of the screen
    editor_draw_rows(&ab);

    ab_Append(&ab, "\x1b[H", 3);
    ab_Append(&ab, "\x1b[?25h", 6);

    //H command is used to clear the screen and set the sursor position
    //it takes 2 arguments Ex:[12;12H where 12;12 is row and column  
    write(STDOUT_FILENO, ab.b, ab.len);
    ab_Free(&ab);
}




/*** init ***/
void init_editor(){

    if(get_window_size(&e_config.screen_rows, &e_config.screen_columns) == -1)
        die("get_window_screensize");

}
int main(){
    enable_rawmode();
    init_editor();
    while(1){
        editor_refresh_screen();
        editor_process_keypress();
    };
    return 0;
}