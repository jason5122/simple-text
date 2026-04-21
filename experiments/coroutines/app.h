#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <corral/corral.h>

namespace app {

namespace internal {
struct AppState;
struct ButtonState;
struct TaskScopeState;
struct WindowState;
}  // namespace internal

struct WindowOptions {
    int width = 800;
    int height = 600;
};

class TaskScope {
public:
    TaskScope() = default;

    void cancel() const;

    template <class Fn>
    void start(Fn&& fn) const {
        start_impl(std::function<corral::Task<void>()>(std::forward<Fn>(fn)));
    }

private:
    explicit TaskScope(std::shared_ptr<internal::TaskScopeState> state);
    void start_impl(std::function<corral::Task<void>()> task_factory) const;

    std::shared_ptr<internal::TaskScopeState> state_;

    friend class Window;
};

class Button {
public:
    Button() = default;

    static Button create(std::string title = "Button");

    void on_click(std::function<void()> handler) const;

private:
    explicit Button(std::shared_ptr<internal::ButtonState> state);

    std::shared_ptr<internal::ButtonState> state_;

    friend class Window;
};

class Window {
public:
    Window() = default;

    void set_title(std::string title) const;
    void add(Button button) const;
    TaskScope task_scope() const;
    void on_close_request(std::function<bool()> handler) const;
    void on_close(std::function<corral::Task<void>()> handler) const;

private:
    explicit Window(std::shared_ptr<internal::WindowState> state);

    std::shared_ptr<internal::WindowState> state_;

    friend class App;
};

class App {
public:
    App() = default;

    Window create_window(WindowOptions options);
    void run();

private:
    explicit App(std::shared_ptr<internal::AppState> state);

    std::shared_ptr<internal::AppState> state_;

    friend App create_app();
};

App create_app();

corral::Task<void> resume_on_ui();
corral::Task<void> sleep_for(std::chrono::milliseconds delay);

}  // namespace app
