#include "client.h"

bool Client::connectToDisplay() {
    display = wl_display_connect(NULL);
    return display;
}
