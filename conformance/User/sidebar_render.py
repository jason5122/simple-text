import json
import os
import sublime
import sublime_plugin


_THEME_NAME = "Sidebar Rasterizer.sublime-theme"
_EMPTY_FOLDER_NAME = "sidebar-rasterizer-empty"
_READY_DELAY_MS = 100


def _write_ready(ready_path, ready_token):
    ready_directory = os.path.dirname(ready_path)
    if ready_directory:
        os.makedirs(ready_directory, exist_ok=True)
    temporary_path = f"{ready_path}.tmp"
    with open(temporary_path, "w", encoding="utf-8") as ready_file:
        ready_file.write(str(ready_token))
    os.replace(temporary_path, ready_path)


class SidebarRenderCommand(sublime_plugin.WindowCommand):
    def run(self, text_path, face, size, ready_path=None, ready_token=None):
        user_package_path = os.path.join(sublime.packages_path(), "User")
        empty_folder_root = os.path.join(user_package_path, _EMPTY_FOLDER_NAME)
        # Sublime's Windows and Linux theme sizes are points, while the renderer API takes
        # logical pixels. Convert at 96 DPI so both sides exercise the same requested size.
        # Windows quantizes theme sizes to whole points before creating the font.
        if sublime.platform() == "osx":
            theme_size = float(size)
        elif sublime.platform() == "windows":
            theme_size = round(float(size) * (4.0 / 3.0))
        else:
            theme_size = float(size) * (4.0 / 3.0)
        with open(text_path, encoding="utf-8") as text_file:
            labels = text_file.read().splitlines()
        folder_paths = [os.path.join(empty_folder_root, str(i)) for i in range(len(labels))]
        for folder_path in folder_paths:
            os.makedirs(folder_path, exist_ok=True)

        theme = {
            "extends": "Default.sublime-theme",
            "variables": {
                "sidebar_bg": [255, 255, 255],
                "sidebar_row_selected": [0, 0, 0, 0.0],
                "sidebar_label_selected": [0, 0, 0],
            },
            "rules": [
                {
                    "class": "sidebar_label",
                    "font.face": face,
                    "font.size": theme_size,
                    "color": [0, 0, 0],
                },
                {
                    "class": "sidebar_heading",
                    "font.face": face,
                    "font.size": theme_size,
                    "font.bold": False,
                    "color": [0, 0, 0],
                },
                {"class": "icon_file_type", "content_margin": [0, 0]},
                {"class": "icon_folder", "content_margin": [0, 0]},
                {"class": "icon_folder_loading", "content_margin": [0, 0]},
                {"class": "disclosure_button_control", "content_margin": [0, 0]},
            ],
        }
        theme_path = os.path.join(user_package_path, _THEME_NAME)
        with open(theme_path, "w", encoding="utf-8") as theme_file:
            json.dump(theme, theme_file, ensure_ascii=False, indent=2)

        sublime.load_settings("Preferences.sublime-settings").set("theme", _THEME_NAME)
        self.window.set_project_data(
            {
                "folders": [
                    {"path": folder_path, "name": label}
                    for folder_path, label in zip(folder_paths, labels)
                ]
            }
        )
        self.window.set_sidebar_visible(True, animate=False)
        if ready_path is not None:
            # Theme resources reload asynchronously after the preference changes. Delay the
            # acknowledgement to a later UI turn, then let the native capturer perform its own
            # pixel-settle check before saving the image.
            sublime.set_timeout(
                lambda: _write_ready(ready_path, ready_token),
                _READY_DELAY_MS,
            )
