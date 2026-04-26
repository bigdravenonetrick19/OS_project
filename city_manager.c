//Phase 1 proj
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define MAX_NAME 50
#define MAX_CATEGORY 50
#define MAX_DESC 256
#define MAX_PATH 256

// roles: manager=0, inspector=1

//The Struct
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

//Utils
void die(const char *msg) {
    perror(msg);
    exit(1);
}

void build_path(char *out, const char *district, const char *file) {
    snprintf(out, MAX_PATH, "%s/%s", district, file);
}

void print_permissions(mode_t m) {
    char p[10];
    p[0] = (m & S_IRUSR)?'r':'-'; p[1] = (m & S_IWUSR)?'w':'-'; p[2] = (m & S_IXUSR)?'x':'-';
    p[3] = (m & S_IRGRP)?'r':'-'; p[4] = (m & S_IWGRP)?'w':'-'; p[5] = (m & S_IXGRP)?'x':'-';
    p[6] = (m & S_IROTH)?'r':'-'; p[7] = (m & S_IWOTH)?'w':'-'; p[8] = (m & S_IXOTH)?'x':'-';
    p[9] = '\0';
    printf("%s", p);
}

int has_read(mode_t m, int role) {
    if (role == 0) return (m & S_IRUSR);
    return (m & S_IRGRP);
}

int has_write(mode_t m, int role) {
    if (role == 0) return (m & S_IWUSR);
    return (m & S_IWGRP);
}

void check_access_or_abort(const char *path, int role, int want_write) {
    struct stat st;
    if (stat(path, &st) < 0) die("stat");

    if (want_write) {
        if (!has_write(st.st_mode, role)) {
            fprintf(stderr, "Access denied (write) for %s\n", path);
            exit(1);
        }
    } else {
        if (!has_read(st.st_mode, role)) {
            fprintf(stderr, "Access denied (read) for %s\n", path);
            exit(1);
        }
    }
}

void log_action(const char *district, const char *role_s, const char *user, const char *action) {
    char path[MAX_PATH];
    build_path(path, district, "logged_district");

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) die("open log");

    time_t now = time(NULL);
    char buf[512];
    int n = snprintf(buf, sizeof(buf), "%ld role=%s user=%s action=%s\n", now, role_s, user, action);
    if (write(fd, buf, n) != n) die("write log");
    close(fd);
}

//Commands I used AI for
int parse_condition(const char *input, char *field, char *op, char *value) {
    return sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == v;
        if (strcmp(op, "!=") == 0) return r->severity != v;
        if (strcmp(op, ">=") == 0) return r->severity >= v;
        if (strcmp(op, "<=") == 0) return r->severity <= v;
        if (strcmp(op, ">") == 0) return r->severity > v;
        if (strcmp(op, "<") == 0) return r->severity < v;
    }
    if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    }
    if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    }
    if (strcmp(field, "timestamp") == 0) {
        long v = atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == v;
        if (strcmp(op, ">=") == 0) return r->timestamp >= v;
        if (strcmp(op, "<=") == 0) return r->timestamp <= v;
        if (strcmp(op, ">") == 0) return r->timestamp > v;
        if (strcmp(op, "<") == 0) return r->timestamp < v;
    }
    return 0;
}

//District setup + Symlink
void ensure_district(const char *district) {
    if (mkdir(district, 0750) < 0 && errno != EEXIST) die("mkdir");

    char path[MAX_PATH];

    build_path(path, district, "reports.dat");
    int fd = open(path, O_CREAT | O_RDWR, 0664);
    if (fd < 0) die("open reports");
    close(fd);
    chmod(path, 0664);

    build_path(path, district, "district.cfg");
    fd = open(path, O_CREAT | O_RDWR, 0640);
    if (fd < 0) die("open cfg");
    if (lseek(fd, 0, SEEK_END) == 0) {
        const char *init = "threshold=2\n";
        if (write(fd, init, strlen(init)) != (ssize_t)strlen(init)) die("write cfg");
    }
    close(fd);
    chmod(path, 0640);

    build_path(path, district, "logged_district");
    fd = open(path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) die("open log file");
    close(fd);
    chmod(path, 0644);

    char linkname[MAX_PATH];
    snprintf(linkname, sizeof(linkname), "active_reports-%s", district);
    char target[MAX_PATH];
    snprintf(target, sizeof(target), "%s/reports.dat", district);
    unlink(linkname); // refresh
    if (symlink(target, linkname) < 0) die("symlink");
}

//Add 
void add_report(const char *district, const char *user, int role, const char *role_s) {
    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 1);

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) die("open add");

    Report r;
    r.id = rand() % 100000;
    strncpy(r.inspector, user, MAX_NAME);

    printf("Latitude: "); if (scanf("%lf", &r.latitude) != 1) die("scanf lat");
    printf("Longitude: "); if (scanf("%lf", &r.longitude) != 1) die("scanf lon");
    printf("Category: "); if (scanf("%49s", r.category) != 1) die("scanf cat");
    printf("Severity: "); if (scanf("%d", &r.severity) != 1) die("scanf sev");

    r.timestamp = time(NULL);

    printf("Description: ");
    if (scanf(" %255[^\n]", r.description) != 1) die("scanf desc");

    if (write(fd, &r, sizeof(Report)) != sizeof(Report)) die("write report");
    close(fd);

    log_action(district, role_s, user, "add");
}

//List
void list_reports(const char *district, int role) {
    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 0);

    struct stat st;
    if (stat(path, &st) < 0) die("stat list");

    print_permissions(st.st_mode);
    printf(" size=%ld mtime=%ld\n", st.st_size, st.st_mtime);

    int fd = open(path, O_RDONLY);
    if (fd < 0) die("open list");

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("ID:%d | %s | %s | Sev:%d\n", r.id, r.inspector, r.category, r.severity);
    }
    close(fd);
}

//View
void view_report(const char *district, int id, int role) {
    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 0);

    int fd = open(path, O_RDONLY);
    if (fd < 0) die("open view");

    Report r;
    int found = 0;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == id) {
            found = 1;
            printf("Inspector: %s\nCoords: %lf %lf\nCategory: %s\nSeverity: %d\nDesc: %s\n",
                   r.inspector, r.latitude, r.longitude, r.category, r.severity, r.description);
            break;
        }
    }
    if (!found) printf("Not found\n");
    close(fd);
}

//Remove (only for manager)
void remove_report(const char *district, int id, int role, const char *role_s, const char *user) {
    if (role != 0) {
        fprintf(stderr, "Only manager can remove\n");
        exit(1);
    }

    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 1);

    int fd = open(path, O_RDWR);
    if (fd < 0) die("open remove");

    Report r;
    off_t pos = 0;
    int found = 0;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == id) { found = 1; break; }
        pos += sizeof(Report);
    }

    if (!found) { close(fd); printf("Not found\n"); return; }

    off_t next = pos + sizeof(Report);

    while (1) {
        if (lseek(fd, next, SEEK_SET) < 0) die("lseek next");
        if (read(fd, &r, sizeof(Report)) != sizeof(Report)) break;
        if (lseek(fd, pos, SEEK_SET) < 0) die("lseek pos");
        if (write(fd, &r, sizeof(Report)) != sizeof(Report)) die("write shift");
        pos += sizeof(Report);
        next += sizeof(Report);
    }

    if (ftruncate(fd, pos) < 0) die("ftruncate");
    close(fd);

    log_action(district, role_s, user, "remove_report");
}

//Update threshold (manager only, strict perm 640)
void update_threshold(const char *district, int value, int role, const char *role_s, const char *user) {
    if (role != 0) { fprintf(stderr, "Only manager\n"); exit(1);}    

    char path[MAX_PATH];
    build_path(path, district, "district.cfg");

    struct stat st;
    if (stat(path, &st) < 0) die("stat cfg");

    if ((st.st_mode & 0777) != 0640) {
        fprintf(stderr, "Refusing: cfg perms changed (expected 640)\n");
        exit(1);
    }

    check_access_or_abort(path, role, 1);

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0) die("open cfg write");

    char buf[64];
    int n = snprintf(buf, sizeof(buf), "threshold=%d\n", value);
    if (write(fd, buf, n) != n) die("write cfg");
    close(fd);

    log_action(district, role_s, user, "update_threshold");
}

//Filter (multiple conditions and)
void filter_reports(const char *district, int role, int argc, char *conds[], int nconds) {
    char path[MAX_PATH];
    build_path(path, district, "reports.dat");

    check_access_or_abort(path, role, 0);

    int fd = open(path, O_RDONLY);
    if (fd < 0) die("open filter");

    char field[64], op[16], value[128];

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int ok = 1;
        for (int i = 0; i < nconds; i++) {
            if (!parse_condition(conds[i], field, op, value)) { ok = 0; break; }
            if (!match_condition(&r, field, op, value)) { ok = 0; break; }
        }
        if (ok) {
            printf("ID:%d | %s | %s | Sev:%d\n", r.id, r.inspector, r.category, r.severity);
        }
    }
    close(fd);
}

//Main
int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: city_manager --role <manager|inspector> --user <name> <command> ...\n");
        return 1;
    }

    char *role_s = NULL, *user = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) role_s = argv[++i];
        else if (strcmp(argv[i], "--user") == 0) user = argv[++i];
    }
    if (!role_s || !user) { fprintf(stderr, "Missing role/user\n"); return 1; }

    int role = (strcmp(role_s, "manager") == 0) ? 0 : 1;

    // command is next non-flag arg after user
    int cmd_i = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--user") == 0) { cmd_i = i + 2; break; }
    }
    if (cmd_i < 0 || cmd_i >= argc) { fprintf(stderr, "Missing command\n"); return 1; }

    const char *cmd = argv[cmd_i];

    if (strcmp(cmd, "--add") == 0) {
        const char *district = argv[cmd_i + 1];
        ensure_district(district);
        add_report(district, user, role, role_s);
    } else if (strcmp(cmd, "--list") == 0) {
        const char *district = argv[cmd_i + 1];
        list_reports(district, role);
    } else if (strcmp(cmd, "--view") == 0) {
        const char *district = argv[cmd_i + 1];
        int id = atoi(argv[cmd_i + 2]);
        view_report(district, id, role);
    } else if (strcmp(cmd, "--remove_report") == 0) {
        const char *district = argv[cmd_i + 1];
        int id = atoi(argv[cmd_i + 2]);
        remove_report(district, id, role, role_s, user);
    } else if (strcmp(cmd, "--update_threshold") == 0) {
        const char *district = argv[cmd_i + 1];
        int val = atoi(argv[cmd_i + 2]);
        update_threshold(district, val, role, role_s, user);
    } else if (strcmp(cmd, "--filter") == 0) {
        const char *district = argv[cmd_i + 1];
        // remaining args are conditions
        int nconds = argc - (cmd_i + 2);
        char **conds = &argv[cmd_i + 2];
        filter_reports(district, role, argc, conds, nconds);
    } else {
        fprintf(stderr, "Unknown command\n");
        return 1;
    }

    return 0;
}


