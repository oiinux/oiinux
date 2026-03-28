#define _POSIX_C_SOURCE 200112L
#define WLR_USE_UNSTABLE
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <wayland-server-core.h>
#include <stdlib.h>

struct oiinux_server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_seat *seat;
};

int main(int argc, char *argv[]) {
    wlr_log_init(WLR_DEBUG, NULL);
    struct oiinux_server server = {0};

    server.display = wl_display_create();
    server.backend = wlr_backend_autocreate(
        wl_display_get_event_loop(server.display), NULL);
    server.renderer = wlr_renderer_autocreate(server.backend);
    wlr_renderer_init_wl_display(server.renderer, server.display);
    server.allocator = wlr_allocator_autocreate(
        server.backend, server.renderer);
    server.scene = wlr_scene_create();
    server.output_layout = wlr_output_layout_create(server.display);
    wlr_scene_attach_output_layout(server.scene, server.output_layout);
    wlr_compositor_create(server.display, 5, server.renderer);
    server.xdg_shell = wlr_xdg_shell_create(server.display, 3);
    server.seat = wlr_seat_create(server.display, "seat0");

    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) { return 1; }

    if (!wlr_backend_start(server.backend)) { return 1; }

    setenv("WAYLAND_DISPLAY", socket, 1);
    wlr_log(WLR_INFO, "OiiNux running on %s", socket);
    wl_display_run(server.display);

    wl_display_destroy_clients(server.display);
    wl_display_destroy(server.display);
    return 0;
}


