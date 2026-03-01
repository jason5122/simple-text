#include "app/app.h"
#include <cstdlib>

int main() {
    auto app = app::create_app(app::Backend::kOpenGL);
    if (!app) std::abort();

    app->run();
}
