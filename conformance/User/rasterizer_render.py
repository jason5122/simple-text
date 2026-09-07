import sublime_plugin

# Module globals persist across command invocations (the plugin host stays alive), which is how we
# reuse one scratch view for the whole run. ST plugins have no other place to keep this state.
view = None
loaded_path = None  # corpus currently displayed, so a size-only step can skip the costly re-layout


class RasterizerRenderCommand(sublime_plugin.WindowCommand):
    def run(self, text_path, face, size):
        global view, loaded_path
        if view is None or not view.is_valid():
            view = self.window.new_file()
            view.set_scratch(True)
            view.settings().set("margin", 0)
            view.settings().set("gutter", False)
            view.settings().set("rulers", [])
            view.settings().set("word_wrap", False)
            view.settings().set("overlay_scroll_bars", "enabled")
            view.settings().set("color_scheme", "Packages/User/Rasterizer.sublime-color-scheme")
            view.set_name("Rasterizer Render")
            loaded_path = None
        self.window.focus_view(view)
        self.window.set_sidebar_visible(False, animate=False)
        self.window.set_minimap_visible(False)
        self.window.set_status_bar_visible(False)
        self.window.set_tabs_visible(False)
        self.window.set_menu_visible(False)

        if text_path != loaded_path:
            with open(text_path, encoding="utf-8") as file:
                text = file.read()
            view.run_command("select_all")
            view.run_command("right_delete")
            view.run_command("insert", {"characters": text})
            view.sel().clear()
            loaded_path = text_path

        view.settings().set("font_face", face)
        view.settings().set("font_size", float(size))
