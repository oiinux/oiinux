#include <gtk/gtk.h>
#include <vte/vte.h>

static void on_child_exited(VteTerminal *term, gint status, gpointer data) {
    gtk_window_destroy(GTK_WINDOW(data));
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "OiiNux Terminal");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);

    GtkWidget *terminal = vte_terminal_new();

    // OiiNux color scheme
    GdkRGBA bg = {0.1, 0.1, 0.15, 1.0};
    GdkRGBA fg = {0.9, 0.9, 0.9, 1.0};
    vte_terminal_set_colors(VTE_TERMINAL(terminal), &fg, &bg, NULL, 0);

    // Font
    PangoFontDescription *font = pango_font_description_from_string("Monospace 11");
    vte_terminal_set_font(VTE_TERMINAL(terminal), font);
    pango_font_description_free(font);

    // Scrollback
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(terminal), 10000);

    // Launch shell
    char *argv[] = {"/bin/bash", NULL};
    vte_terminal_spawn_async(VTE_TERMINAL(terminal),
        VTE_PTY_DEFAULT, NULL, argv, NULL,
        G_SPAWN_DEFAULT, NULL, NULL, NULL, -1,
        NULL, NULL, NULL);

    g_signal_connect(terminal, "child-exited",
        G_CALLBACK(on_child_exited), window);

    gtk_window_set_child(GTK_WINDOW(window), terminal);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new(
        "dev.oiinux.terminal", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

