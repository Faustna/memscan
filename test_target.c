#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>

int main(void) {
    int ser = 21;
    printf("PID: %d, Address: %p\n", getpid(), &ser);
    fflush(stdout);

    char buf[64];
    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        tv.tv_sec = 1; 
        tv.tv_usec = 0;

        int ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

        if (ret > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buf, sizeof(buf), stdin) != NULL) {
                ser = atoi(buf);
            }
        }

        printf("Current value: %d\n", ser);
        fflush(stdout);
    }
    return 0;
}
