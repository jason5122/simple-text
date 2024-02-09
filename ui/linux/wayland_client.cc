#include "wayland_client.h"

bool WaylandClient::connectToDisplay() {
    display = wl_display_connect(NULL);
    return display;
}
