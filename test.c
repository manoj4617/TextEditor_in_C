#include <termios.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    struct termios term;

    if (tcgetattr(STDIN_FILENO, &term) < 0) {
        perror("tcgetattr");
        return 1;
    }

    // Print the current terminal attributes
    printf("Input baud rate: %d\n", cfgetispeed(&term));
    printf("Output baud rate: %d\n", cfgetospeed(&term));
    printf("Character size: %d\n", (term.c_cflag & CSIZE));
    printf("Parity: %d\n", (term.c_cflag & PARENB));
    printf("Stop bits: %d\n", (term.c_cflag & CSTOPB));

    return 0;
}
