#pragma once

class App;

class Window2 {
private:
    friend class App;
    App& app;

    Window2(App& app);
};
