// phase 1 + phase 2
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_NAME 50
#define MAX_CATEGORY 50
#define MAX_DESC 256
#define MAX_PATH 256

typedef struct {
    int id;
    char inspector[MAX_NAME];
    double latitude;
    double longitude;
    char category[MAX_CATEGORY];
    int severity;
    time_t timestamp;
    char description[MAX_DESC];
} Report;

// utils

void die(const char *msg) {
    perror(msg);
    exit(1);
}

void build_path(char *out, const char *district, const char *file) {
    snprintf(out, MAX_PATH, "%s/%s", district, file);
}

int has_read(mode_t m, int role) {
    return (role == 0) ? (m & S_IRUSR) : (m & S_IRGRP);
}

int has_write(mode_t m, int role) {
    return (role == 0) ? (m & S_IWUSR) : (m & S_IWGRP);
}

void check_access_or_abort(const char *path, int role, int want_write) {
    struct stat st;
    if (stat(path, &st) < 0) die("stat");

    if (want_write && !has_write(st.st_mode, role)) {
        fprintf(stderr, "Access denied write\n");
        exit(1);
    }
    if (!want_write && !has_read(st.st_mode, role)) {
        fprintf(stderr, "Access denied read\n");
        exit(1);
    }
}

void log_action(const char *district, const char *role_s, const char *user, const char *action) {
    char path[MAX_PATH];
    build_path(path, district, "logged_district");

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) return;

    time_t now = time(NULL);
    char buf[512];
    int n = snprintf(buf, sizeof(buf),
        "%ld role=%s user=%s action=%s\n",
        now, role_s, user, action);

    write(fd, buf, n);
    close(fd);
}

// monitor thingy

void notify_monitor(const char *district, const char *role_s, const char *user) {
    int fd = open(".monitor_pid", O_RDONLY);
    char msg[128];

    if (fd < 0) {
        snprintf(msg, sizeof(msg), "monitor_not_found");
        log_action(district, role_s, user, msg);
        return;
    }

    char buf[32];
    int n = read(fd, buf, sizeof(buf)-1);
    close(fd);

    if (n <= 0) {
        snprintf(msg, sizeof(msg), "monitor_pid_error");
        log_action(district, role_s, user, msg);
        return;
    }

    buf[n] = '\0';
    pid_t pid = atoi(buf);

    if (kill(pid, SIGUSR1) < 0)
        snprintf(msg, sizeof(msg), "monitor_signal_failed");
    else
        snprintf(msg, sizeof(msg), "monitor_notified");

    log_action(district, role_s, user, msg);
}

// district

void ensure_district(const char *district) {
    mkdir(district, 0750);

    char path[MAX_PATH];

    build_path(path, district, "reports.dat");
    int fd = open(path, O_CREAT | O_RDWR, 0664);
    close(fd);

    build_path(path, district, "district.cfg");
    fd = open(path, O_CREAT | O_RDWR, 0640);
    close(fd);

    build_path(path, district, "logged_district");
    fd = open(path, O_CREAT | O_RDWR, 0644);
    close(fd);

    char linkname[MAX_PATH];
    snprintf(linkname, sizeof(linkname), "active_reports-%s", district);

    char target[MAX_PATH];
    snprintf(target, sizeof(target), "%s/reports.dat", district);

    unlink(linkname);
    symlink(target, linkname);
}

// commands

void add_report(const char *district, const char *user, int role, const char *role_s) {
    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 1);

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) die("open");

    Report r;
    r.id = rand() % 100000;
    strcpy(r.inspector, user);

    printf("Latitude: "); scanf("%lf", &r.latitude);
    printf("Longitude: "); scanf("%lf", &r.longitude);
    printf("Category: "); scanf("%s", r.category);
    printf("Severity: "); scanf("%d", &r.severity);

    r.timestamp = time(NULL);

    printf("Description: ");
    scanf(" %[^\n]", r.description);

    write(fd, &r, sizeof(r));
    close(fd);

    log_action(district, role_s, user, "add");
    notify_monitor(district, role_s, user);
}

void list_reports(const char *district, int role) {
    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 0);

    int fd = open(path, O_RDONLY);
    if (fd < 0) die("open");

    Report r;
    while (read(fd, &r, sizeof(r)) == sizeof(r)) {
        printf("ID:%d | %s | %s | Sev:%d\n",
            r.id, r.inspector, r.category, r.severity);
    }
    close(fd);
}

void remove_district(const char *district, int role, const char *role_s, const char *user) {
    if (role != 0) {
        fprintf(stderr, "Only manager\n");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        execlp("rm", "rm", "-rf", district, NULL);
        exit(1);
    } else {
        wait(NULL);

        char linkname[MAX_PATH];
        snprintf(linkname, sizeof(linkname),
                 "active_reports-%s", district);
        unlink(linkname);

        log_action(district, role_s, user, "remove_district");
    }
}

// main

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Usage...\n");
        return 1;
    }

    char *role_s = NULL, *user = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--role")) role_s = argv[++i];
        else if (!strcmp(argv[i], "--user")) user = argv[++i];
    }

    int role = (!strcmp(role_s, "manager")) ? 0 : 1;

    int cmd_i = 5;
    char *cmd = argv[cmd_i];

    if (!strcmp(cmd, "--add")) {
        ensure_district(argv[cmd_i+1]);
        add_report(argv[cmd_i+1], user, role, role_s);
    }
    else if (!strcmp(cmd, "--list")) {
        list_reports(argv[cmd_i+1], role);
    }
    else if (!strcmp(cmd, "--remove_district")) {
        remove_district(argv[cmd_i+1], role, role_s, user);
    }
    else {
        printf("Unknown command\n");
    }

    return 0;
}
