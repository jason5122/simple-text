import json
import os
import sublime
import sublime_plugin


_THEME_NAME = "Sidebar Rasterizer.sublime-theme"
_EMPTY_FOLDER_NAME = "sidebar-rasterizer-empty"


class SidebarRenderCommand(sublime_plugin.WindowCommand):
    def run(self, text_path, face, size):
        user_package_path = os.path.join(sublime.packages_path(), "User")
        empty_folder_root = os.path.join(user_package_path, _EMPTY_FOLDER_NAME)
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
                    "font.size": float(size),
                    "color": [0, 0, 0],
                },
                {
                    "class": "sidebar_heading",
                    "font.face": face,
                    "font.size": float(size),
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
