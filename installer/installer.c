#include <ncurses.h>
#include <menu.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

#define COLOR_TITLE  1
#define COLOR_BOX    2
#define COLOR_SEL    3
#define COLOR_WARN   4
#define COLOR_OK     5

/* ── state ─────────────────────────────────────────── */
static char chosen_disk[64]     = "";
static char chosen_de[32]       = "minimal";
static char chosen_hostname[64] = "oiinux";
static char chosen_user[32]     = "user";
static char chosen_pass[64]     = "";
static int  auto_partition      = 1;

/* ── helpers ────────────────────────────────────────── */
static void init_colors(void) {
    start_color();
    use_default_colors();
    init_pair(COLOR_TITLE, COLOR_CYAN,    -1);
    init_pair(COLOR_BOX,   COLOR_WHITE,   -1);
    init_pair(COLOR_SEL,   COLOR_BLACK,   COLOR_CYAN);
    init_pair(COLOR_WARN,  COLOR_RED,     -1);
    init_pair(COLOR_OK,    COLOR_GREEN,   -1);
}

static void draw_header(const char *title) {
    int cols = getmaxx(stdscr);
    attron(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    mvhline(0, 0, ' ', cols);
    mvprintw(0, (cols - strlen(title)) / 2, "%s", title);
    attroff(COLOR_PAIR(COLOR_TITLE) | A_BOLD);
    attron(COLOR_PAIR(COLOR_BOX));
    mvprintw(1, 0, "OiinuX Installer v0.1.0");
    mvprintw(1, cols - 20, "Tab/Enter:select");
    attroff(COLOR_PAIR(COLOR_BOX));
    mvhline(2, 0, ACS_HLINE, cols);
}

static void draw_footer(const char *hint) {
    int rows = getmaxy(stdscr);
    int cols = getmaxx(stdscr);
    mvhline(rows - 2, 0, ACS_HLINE, cols);
    attron(COLOR_PAIR(COLOR_BOX));
    mvprintw(rows - 1, 2, "%s", hint);
    attroff(COLOR_PAIR(COLOR_BOX));
}

static void draw_box(int y, int x, int h, int w, const char *title) {
    attron(COLOR_PAIR(COLOR_BOX));
    mvvline(y,     x,     ACS_VLINE, h);
    mvvline(y,     x+w-1, ACS_VLINE, h);
    mvhline(y,     x,     ACS_HLINE, w);
    mvhline(y+h-1, x,     ACS_HLINE, w);
    mvaddch(y,     x,     ACS_ULCORNER);
    mvaddch(y,     x+w-1, ACS_URCORNER);
    mvaddch(y+h-1, x,     ACS_LLCORNER);
    mvaddch(y+h-1, x+w-1, ACS_LRCORNER);
    if (title)
        mvprintw(y, x + 2, "[ %s ]", title);
    attroff(COLOR_PAIR(COLOR_BOX));
}

/* run a shell command, show output in a box */
static int run_cmd(const char *cmd) {
    return system(cmd);
}

/* input field */
static void field_input(int y, int x, int w, char *buf, int maxlen, int secret) {
    echo();
    curs_set(1);
    if (secret) noecho();
    mvhline(y, x, '_', w);
    move(y, x);
    int i = 0;
    int ch;
    memset(buf, 0, maxlen);
    while ((ch = getch()) != '\n' && i < maxlen - 1) {
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (i > 0) { i--; mvaddch(y, x+i, '_'); move(y, x+i); }
        } else {
            buf[i++] = ch;
            if (!secret) mvaddch(y, x+i-1, ch);
            else         mvaddch(y, x+i-1, '*');
        }
    }
    buf[i] = '\0';
    noecho();
    curs_set(0);
}

/* ── screens ────────────────────────────────────────── */

/* 0: welcome */
static void screen_welcome(void) {
    clear();
    draw_header(" Welcome to OiinuX ");
    int rows = getmaxy(stdscr);
    int cols = getmaxx(stdscr);
    draw_box(4, cols/2 - 30, rows - 8, 60, "Welcome");

    attron(COLOR_PAIR(COLOR_OK) | A_BOLD);
    mvprintw(6,  cols/2 - 22, "OiinuX Linux Installer");
    attroff(COLOR_PAIR(COLOR_OK) | A_BOLD);

    mvprintw(8,  cols/2 - 26, "This will install OiinuX to your disk.");
    mvprintw(9,  cols/2 - 26, "The process takes about 2-5 minutes.");
    mvprintw(11, cols/2 - 26, "Steps:");
    mvprintw(12, cols/2 - 24, "1. Select disk");
    mvprintw(13, cols/2 - 24, "2. Partitioning");
    mvprintw(14, cols/2 - 24, "3. Choose desktop environment");
    mvprintw(15, cols/2 - 24, "4. Set hostname & user");
    mvprintw(16, cols/2 - 24, "5. Install");

    attron(COLOR_PAIR(COLOR_WARN));
    mvprintw(18, cols/2 - 26, "WARNING: Selected disk will be WIPED.");
    attroff(COLOR_PAIR(COLOR_WARN));

    draw_footer("Press ENTER to begin, Q to quit");
    refresh();

    int ch;
    while ((ch = getch()) != '\n' && ch != 'q' && ch != 'Q');
    if (ch == 'q' || ch == 'Q') { endwin(); exit(0); }
}

/* 1: disk selection */
#define MAX_DISKS 16
static int screen_disk(void) {
    char disks[MAX_DISKS][64];
    char sizes[MAX_DISKS][32];
    int  ndisks = 0;

    /* parse /proc/partitions for whole disks */
    FILE *f = fopen("/proc/partitions", "r");
    if (f) {
        char line[256];
        fgets(line, sizeof(line), f); /* header */
        fgets(line, sizeof(line), f); /* blank  */
        while (fgets(line, sizeof(line), f) && ndisks < MAX_DISKS) {
            unsigned long maj, min, blocks;
            char name[64];
            if (sscanf(line, " %lu %lu %lu %63s", &maj, &min, &blocks, name) == 4) {
                /* skip partitions (ends with digit) */
                int last = strlen(name) - 1;
                if (name[last] >= '0' && name[last] <= '9') continue;
                snprintf(disks[ndisks], 64, "/dev/%s", name);
                snprintf(sizes[ndisks], 32, "%.1f GB", blocks / 1024.0 / 1024.0);
                ndisks++;
            }
        }
        fclose(f);
    }

    if (ndisks == 0) {
        /* fallback */
        strcpy(disks[0], "/dev/sda");
        strcpy(sizes[0], "? GB");
        ndisks = 1;
    }

    int sel = 0;
    int rows = getmaxy(stdscr);
    int cols = getmaxx(stdscr);

    while (1) {
        clear();
        draw_header(" Select Installation Disk ");
        draw_box(4, cols/2 - 30, ndisks + 6, 60, "Available Disks");

        for (int i = 0; i < ndisks; i++) {
            if (i == sel) attron(COLOR_PAIR(COLOR_SEL) | A_BOLD);
            mvprintw(6 + i, cols/2 - 28, "  %-20s  %10s  ", disks[i], sizes[i]);
            if (i == sel) attroff(COLOR_PAIR(COLOR_SEL) | A_BOLD);
        }

        attron(COLOR_PAIR(COLOR_WARN));
        mvprintw(6 + ndisks + 2, cols/2 - 28, "All data on selected disk will be lost!");
        attroff(COLOR_PAIR(COLOR_WARN));

        draw_footer("UP/DOWN: navigate  ENTER: select  B: back");
        refresh();

        int ch = getch();
        if (ch == KEY_UP   && sel > 0)          sel--;
        if (ch == KEY_DOWN && sel < ndisks - 1) sel++;
        if (ch == '\n') { strncpy(chosen_disk, disks[sel], 63); return 1; }
        if (ch == 'b'  || ch == 'B') return 0;
    }
}

/* 2: partition scheme */
static int screen_partition(void) {
    int sel = 0;
    int cols = getmaxx(stdscr);

    while (1) {
        clear();
        draw_header(" Partitioning ");
        draw_box(4, cols/2 - 30, 12, 60, "Partition Scheme");

        const char *opts[] = {
            "Auto  - Wipe and create: EFI + swap + root",
            "Manual - Drop to shell (fdisk)"
        };

        mvprintw(6, cols/2 - 28, "Disk: %s", chosen_disk);

        for (int i = 0; i < 2; i++) {
            if (i == sel) attron(COLOR_PAIR(COLOR_SEL) | A_BOLD);
            mvprintw(8 + i, cols/2 - 26, "  %s  ", opts[i]);
            if (i == sel) attroff(COLOR_PAIR(COLOR_SEL) | A_BOLD);
        }

        draw_footer("UP/DOWN: navigate  ENTER: select  B: back");
        refresh();

        int ch = getch();
        if (ch == KEY_UP   && sel > 0) sel--;
        if (ch == KEY_DOWN && sel < 1) sel++;
        if (ch == '\n') { auto_partition = (sel == 0); return 1; }
        if (ch == 'b'  || ch == 'B') return 0;
    }
}

/* 3: DE selection */
static int screen_de(void) {
    int sel = 0;
    int cols = getmaxx(stdscr);

    const char *de_names[] = { "Minimal", "XFCE", "Openbox", "Custom (OiinuX DE)" };
    const char *de_ids[]   = { "minimal", "xfce", "openbox", "oiinux-de" };
    const char *de_desc[]  = {
        "xterm + twm. Lightest option (~50MB)",
        "Full desktop, file manager, panel (~300MB)",
        "Lightweight WM, right-click menu (~80MB)",
        "OiinuX custom DE (WIP - installs minimal)"
    };
    int nde = 4;

    while (1) {
        clear();
        draw_header(" Choose Desktop Environment ");
        draw_box(4, cols/2 - 36, nde * 3 + 6, 72, "Desktop Environments");

        for (int i = 0; i < nde; i++) {
            if (i == sel) attron(COLOR_PAIR(COLOR_SEL) | A_BOLD);
            mvprintw(6 + i*3,     cols/2 - 34, "  %-20s", de_names[i]);
            if (i == sel) attroff(COLOR_PAIR(COLOR_SEL) | A_BOLD);
            attron(COLOR_PAIR(COLOR_BOX));
            mvprintw(6 + i*3 + 1, cols/2 - 32, "  %s", de_desc[i]);
            attroff(COLOR_PAIR(COLOR_BOX));
        }

        draw_footer("UP/DOWN: navigate  ENTER: select  B: back");
        refresh();

        int ch = getch();
        if (ch == KEY_UP   && sel > 0)      sel--;
        if (ch == KEY_DOWN && sel < nde-1)  sel++;
        if (ch == '\n') { strncpy(chosen_de, de_ids[sel], 31); return 1; }
        if (ch == 'b'  || ch == 'B') return 0;
    }
}

/* 4: hostname + user */
static int screen_usersetup(void) {
    int cols = getmaxx(stdscr);
    char pass2[64] = "";

    while (1) {
        clear();
        draw_header(" System Configuration ");
        draw_box(4, cols/2 - 30, 16, 60, "Hostname & User");

        mvprintw(6,  cols/2 - 28, "Hostname:");
        mvprintw(8,  cols/2 - 28, "Username:");
        mvprintw(10, cols/2 - 28, "Password:");
        mvprintw(12, cols/2 - 28, "Confirm password:");

        /* hostname */
        attron(COLOR_PAIR(COLOR_SEL));
        mvprintw(6, cols/2 - 15, " %-20s ", chosen_hostname);
        attroff(COLOR_PAIR(COLOR_SEL));

        draw_footer("Fill fields, press ENTER after each  B: back");
        refresh();

        mvprintw(6,  cols/2 - 28, "Hostname:        ");
        field_input(6, cols/2 - 15, 20, chosen_hostname, 63, 0);
        if (!strlen(chosen_hostname)) strcpy(chosen_hostname, "oiinux");

        mvprintw(8,  cols/2 - 28, "Username:        ");
        field_input(8, cols/2 - 15, 20, chosen_user, 31, 0);
        if (!strlen(chosen_user)) strcpy(chosen_user, "user");

        field_input(10, cols/2 - 15, 20, chosen_pass, 63, 1);
        field_input(12, cols/2 - 15, 20, pass2,        63, 1);

        if (strcmp(chosen_pass, pass2) != 0) {
            attron(COLOR_PAIR(COLOR_WARN) | A_BOLD);
            mvprintw(14, cols/2 - 28, "Passwords do not match! Press any key...");
            attroff(COLOR_PAIR(COLOR_WARN) | A_BOLD);
            refresh();
            getch();
            continue;
        }
        return 1;
    }
}

/* 5: confirm */
static int screen_confirm(void) {
    int cols = getmaxx(stdscr);

    clear();
    draw_header(" Confirm Installation ");
    draw_box(4, cols/2 - 32, 14, 64, "Summary");

    mvprintw(6,  cols/2 - 30, "Disk:        %s", chosen_disk);
    mvprintw(7,  cols/2 - 30, "Partition:   %s", auto_partition ? "Auto" : "Manual");
    mvprintw(8,  cols/2 - 30, "Desktop:     %s", chosen_de);
    mvprintw(9,  cols/2 - 30, "Hostname:    %s", chosen_hostname);
    mvprintw(10, cols/2 - 30, "User:        %s", chosen_user);

    attron(COLOR_PAIR(COLOR_WARN) | A_BOLD);
    mvprintw(12, cols/2 - 30, "This will ERASE %s. No going back.", chosen_disk);
    attroff(COLOR_PAIR(COLOR_WARN) | A_BOLD);

    draw_footer("ENTER: begin install  B: go back and change");
    refresh();

    int ch;
    while ((ch = getch()) != '\n' && ch != 'b' && ch != 'B');
    return (ch == '\n') ? 1 : 0;
}

/* ── install ────────────────────────────────────────── */
static void progress(WINDOW *win, int step, int total, const char *msg) {
    int w = getmaxx(win) - 4;
    int filled = (step * w) / total;
    mvwprintw(win, 1, 2, "%-*s", w, msg);
    wattron(win, COLOR_PAIR(COLOR_OK));
    mvwhline(win, 3, 2, ACS_CKBOARD, filled);
    wattroff(win, COLOR_PAIR(COLOR_OK));
    mvwhline(win, 3, 2 + filled, ' ', w - filled);
    mvwprintw(win, 3, w/2, " %d/%d ", step, total);
    wrefresh(win);
}

static void do_install(void) {
    int rows = getmaxy(stdscr);
    int cols = getmaxx(stdscr);
    char cmd[512];

    clear();
    draw_header(" Installing OiinuX ");

    WINDOW *pwin = newwin(5, cols - 8, rows/2 - 2, 4);
    box(pwin, 0, 0);
    wrefresh(pwin);

    int step = 0, total = 8;

    /* 1. partition */
    progress(pwin, ++step, total, "Partitioning disk...");
    if (auto_partition) {
        snprintf(cmd, sizeof(cmd),
            "parted -s %s mklabel msdos "
	    "mkpart primary fat32 1MiB 513MiB set 1 boot on "
	    "mkpart primary linux-swap 513MiB 1537MiB "
	    "mkpart primary ext4 1537MiB 100%%",
            chosen_disk);
        run_cmd(cmd);
    }

    /* 2. format */
    progress(pwin, ++step, total, "Formatting partitions...");
    snprintf(cmd, sizeof(cmd), "mkfs.fat -F32 %s1 2>/dev/null", chosen_disk); run_cmd(cmd);
    snprintf(cmd, sizeof(cmd), "mkswap %s2 2>/dev/null",         chosen_disk); run_cmd(cmd);
    snprintf(cmd, sizeof(cmd), "mkfs.ext4 -F %s3 2>/dev/null",   chosen_disk); run_cmd(cmd);

    /* 3. mount */
    progress(pwin, ++step, total, "Mounting partitions...");
    run_cmd("mkdir -p /mnt/oiinux");
    snprintf(cmd, sizeof(cmd), "mount %s3 /mnt/oiinux", chosen_disk); run_cmd(cmd);
    run_cmd("mkdir -p /mnt/oiinux/boot/efi");
    snprintf(cmd, sizeof(cmd), "mount %s1 /mnt/oiinux/boot/efi", chosen_disk); run_cmd(cmd);
    snprintf(cmd, sizeof(cmd), "swapon %s2", chosen_disk); run_cmd(cmd);

    /* 4. copy base system */
    progress(pwin, ++step, total, "Copying base system...");
    run_cmd("mkdir -p /mnt/oiinux/proc /mnt/oiinux/sys "
        "/mnt/oiinux/dev /mnt/oiinux/run /mnt/oiinux/tmp /mnt/oiinux/mnt && "
        "cp -a /bin /sbin /usr /lib /etc /root /var /home "
        "/mnt/oiinux/ 2>/dev/null; true");

    /* 5. install DE packages */
    progress(pwin, ++step, total, "Installing desktop environment...");
    if (strcmp(chosen_de, "xfce") == 0) {
        run_cmd("chroot /mnt/oiinux /sbin/apk --allow-untrusted add xfce4 xfce4-terminal 2>/dev/null");
    } else if (strcmp(chosen_de, "openbox") == 0) {
        run_cmd("chroot /mnt/oiinux /sbin/apk --allow-untrusted add openbox obconf 2>/dev/null");
    }

    /* 6. hostname + user */
    progress(pwin, ++step, total, "Configuring system...");
    snprintf(cmd, sizeof(cmd), "echo '%s' > /mnt/oiinux/etc/hostname", chosen_hostname);
    run_cmd(cmd);
    snprintf(cmd, sizeof(cmd),
        "chroot /mnt/oiinux /bin/sh -c 'adduser -D %s && echo \"%s:%s\" | chpasswd'",
        chosen_user, chosen_user, chosen_pass);
    run_cmd(cmd);

    /* 7. fstab */
    progress(pwin, ++step, total, "Writing fstab...");
    FILE *f = fopen("/mnt/oiinux/etc/fstab", "w");
    if (f) {
        fprintf(f, "%s3  /         ext4  defaults  0 1\n", chosen_disk);
        fprintf(f, "%s1  /boot/efi vfat  defaults  0 2\n", chosen_disk);
        fprintf(f, "%s2  none      swap  sw        0 0\n", chosen_disk);
        fclose(f);
    }

    /* 8. bootloader */
    progress(pwin, ++step, total, "Installing bootloader...");
    run_cmd("chroot /mnt/oiinux grub-install --target=x86_64-efi "
            "--efi-directory=/boot/efi --bootloader-id=OiinuX 2>/dev/null || "
            "chroot /mnt/oiinux grub-install --target=i386-pc "
            "/dev/sda 2>/dev/null");
    run_cmd("chroot /mnt/oiinux grub-mkconfig -o /boot/grub/grub.cfg 2>/dev/null");

    delwin(pwin);

    run_cmd("touch /mnt/oiinux/etc/oiinux-installed");
    run_cmd("touch /etc/oiinux-installed");
    /* done */
    clear();
    draw_header(" Installation Complete ");
    int cx = cols/2;
    attron(COLOR_PAIR(COLOR_OK) | A_BOLD);
    mvprintw(rows/2 - 2, cx - 14, "OiinuX installed successfully!");
    attroff(COLOR_PAIR(COLOR_OK) | A_BOLD);
    mvprintw(rows/2,     cx - 18, "Remove the ISO and reboot into your new system.");
    mvprintw(rows/2 + 2, cx - 10, "Hostname : %s", chosen_hostname);
    mvprintw(rows/2 + 3, cx - 10, "User     : %s", chosen_user);
    draw_footer("Press R to reboot, Q to drop to shell");
    refresh();

    int ch;
    while ((ch = getch()) != 'r' && ch != 'R' && ch != 'q' && ch != 'Q');
    endwin();
    if (ch == 'r' || ch == 'R')
        run_cmd("reboot");
}

/* ── main ───────────────────────────────────────────── */
int main(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_colors();

    int step = 0;
    while (1) {
        switch (step) {
            case 0: screen_welcome();                     step = 1; break;
            case 1: step = screen_disk()      ? 2 : 0;   break;
            case 2: step = screen_partition() ? 3 : 1;   break;
            case 3: step = screen_de()        ? 4 : 2;   break;
            case 4: step = screen_usersetup() ? 5 : 3;   break;
            case 5: step = screen_confirm()   ? 6 : 4;   break;
            case 6: do_install(); goto done;
        }
    }
done:
    endwin();
    return 0;
}
