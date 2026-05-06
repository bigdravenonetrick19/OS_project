#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

volatile sig_atomic_t running = 1;

void handle_sigint(int sig) {
    printf("Monitor exiting (SIGINT)\n");
    running = 0;
}

void handle_sigusr1(int sig) {
    printf("Monitor: new report added\n");
}

int main() {
    // write pid
    int fd = open(".monitor_pid",
        O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char buf[32];
    int n = snprintf(buf, sizeof(buf),
        "%d\n", getpid());

    write(fd, buf, n);
    close(fd);

    struct sigaction sa1 = {0}, sa2 = {0};

    sa1.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa1, NULL);

    sa2.sa_handler = handle_sigusr1;
    sigaction(SIGUSR1, &sa2, NULL);

    while (running)
        pause();

    unlink(".monitor_pid");

    return 0;
}
