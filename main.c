#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>

struct termios orig_termios;

//Leaves terminal attributes as they were
void disable_rawmode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_rawmode(){
    tcgetattr(STDIN_FILENO, &orig_termios);
    //on exit from main or by calling exit() call disable_rawmode function
    atexit(disable_rawmode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(){
    enable_rawmode();
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && 
           c != 'q'){

            if(iscntrl(c)){
                printf("%d\n",c);
            }
            else{
                printf("%d ('%c')\n", c, c);
            }

           };

    
    return 0;
}