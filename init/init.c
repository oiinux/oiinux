#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#define VERSION "0.1.0"

static void mount_vfs(void) {
    mount("proc",    "/proc",     "proc",    MS_NODEV|MS_NOSUID|MS_NOEXEC, NULL);
    mount("sysfs",   "/sys",      "sysfs",   MS_NODEV|MS_NOSUID|MS_NOEXEC, NULL);
    mount("devtmpfs","/dev",      "devtmpfs",MS_NOSUID|MS_STRICTATIME,     NULL);
    mkdir("/dev/pts", 0755);
    mount("devpts",  "/dev/pts",  "devpts",  MS_NOSUID|MS_NOEXEC,          "gid=5,mode=620");
    mount("tmpfs",   "/dev/shm",  "tmpfs",   MS_NOSUID|MS_NODEV,           NULL);
    mount("tmpfs",   "/tmp",      "tmpfs",   MS_NOSUID|MS_NODEV,           NULL);
    mount("tmpfs",   "/run",      "tmpfs",   MS_NOSUID|MS_NODEV,           NULL);
}

static void network_up(void) {
    pid_t pid = fork();
    if (pid == 0) {
        char *ifup[] = {"/sbin/ip", "link", "set", "eth0", "up", NULL};
        execve("/sbin/ip", ifup, NULL);
        exit(1);
    }
    if (pid > 0) waitpid(pid, NULL, 0);
    pid = fork();
    if (pid == 0) {
        char *dhcp[] = {"/sbin/udhcpc", "-i", "eth0", "-q", NULL};
        execve("/sbin/udhcpc", dhcp, NULL);
        exit(1);
    }
    if (pid > 0) waitpid(pid, NULL, 0);
}

static int is_installed(void) {
    char buf[512] = {0};
    FILE *f = fopen("/proc/cmdline", "r");
    if (!f) return 0;
    fgets(buf, sizeof(buf), f);
    fclose(f);
    return strstr(buf, "installed") != NULL;
}

static void spawn_shell(void) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int fd = open("/dev/tty1", O_RDWR);
        if (fd < 0) fd = open("/dev/console", O_RDWR);
        if (fd >= 0) { dup2(fd,0); dup2(fd,1); dup2(fd,2); if(fd>2)close(fd); }
        char *envp[] = {
            "HOME=/root", "TERM=xterm-256color",
            "PATH=/bin:/sbin:/usr/bin:/usr/sbin",
            "PS1=[oiinux \\w]\\$ ", NULL
        };
        if (!is_installed()) {
            char *argv[] = {"/bin/sh", "-c",
                "/usr/bin/oiinux-install; /bin/sh", NULL};
            execve("/bin/sh", argv, envp);
        } else {
            char *argv[] = {"/bin/sh", NULL};
            execve("/bin/sh", argv, envp);
        }
        exit(1);
    }
}

static pid_t xorg_pid = -1;

static void start_xorg(void) {
    mkdir("/run/user",   0755);
    mkdir("/run/user/0", 0700);

    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int fd = open("/dev/tty1", O_RDWR);
        if (fd < 0) fd = open("/dev/console", O_RDWR);
        if (fd >= 0) { dup2(fd,0); dup2(fd,1); dup2(fd,2); if(fd>2)close(fd); }

        char *envp[] = {
            "HOME=/root",
            "USER=root",
            "TERM=xterm-256color",
            "DISPLAY=:0",
            "XAUTHORITY=/root/.Xauthority",
            "XDG_RUNTIME_DIR=/run/user/0",
            "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/X11R7/bin",
            "HOSTNAME=oiinux",
            NULL
        };

        char *argv[] = {"/usr/bin/xinit", "/root/.xinitrc", "--", ":0", "-retro", NULL};
        execve("/usr/bin/xinit", argv, envp);

        /* fallback: explicit Xorg path */
        char *argv2[] = {"/usr/bin/xinit", "/root/.xinitrc", "--", "/usr/bin/Xorg", ":0", "-retro", NULL};
        execve("/usr/bin/xinit", argv2, envp);

        exit(1);
    }
    xorg_pid = pid;
}

static void reap(int s) {
    (void)s;
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

int main(void) {
    mount_vfs();
    network_up();
    puts(
        "\n"
        "   ____  _  _                  __  __\n"
        "  / __ \\(_|_)_ __  _   ___  __|  \\/  |\n"
        " | |  | || | '_ \\| | | \\ \\/ /| |\\/| |\n"
        " | |__| || | | | | |_| |>  < | |  | |\n"
        "  \\____/ |_|_| |_|\\__,_/_/\\_\\|_|  |_|\n"
        "\n"
        "  version " VERSION "\n"
        "  oiia oiia oiia\n"
    );
    fflush(stdout);
    signal(SIGCHLD, reap);
    spawn_shell();
    start_xorg();
    while (1) {
        int status;
        pid_t p = wait(&status);
        if (p <= 0) { sleep(1); continue; }
        spawn_shell();
    }
}
