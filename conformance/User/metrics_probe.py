import hashlib
import json
import os

import sublime
import sublime_plugin


_CASE_SETTING = "metrics_probe_output_path"
_POLL_INTERVAL_MS = 20
_STABLE_SAMPLE_COUNT = 2
_MAX_SAMPLE_ATTEMPTS = 250
_REQUEST_NAME = "metrics_probe_request.json"
_VIEW_SETTING = "is_metrics_probe"
_VIEW_NAME = "Metrics Probe"


def _probe_view(window):
    for view in window.views():
        if view.settings().get(_VIEW_SETTING):
            return view

    view = window.new_file()
    view.set_scratch(True)
    view.set_name(_VIEW_NAME)
    view.settings().set(_VIEW_SETTING, True)
    view.settings().set("gutter", False)
    view.settings().set("margin", 0)
    view.settings().set("line_padding_top", 0)
    view.settings().set("line_padding_bottom", 0)
    view.settings().set("word_wrap", False)
    return view


def _replace_text(view, text):
    current = view.substr(sublime.Region(0, view.size()))
    if current == text:
        return

    view.run_command("select_all")
    view.run_command("right_delete")
    view.run_command("insert", {"characters": text})
    view.sel().clear()


def _vector(value):
    return [float(value[0]), float(value[1])]


def _collect_metrics(view, text, text_path, face, size, font_options):
    line_origins = {}
    points = []
    for point in range(view.size() + 1):
        row, column = view.rowcol(point)
        _, utf8_column = view.rowcol_utf8(point)
        _, utf16_column = view.rowcol_utf16(point)
        if row not in line_origins:
            line_start = view.text_point(row, 0)
            line_origins[row] = _vector(view.text_to_layout(line_start))

        layout_position = _vector(view.text_to_layout(point))
        points.append(
            {
                "point": point,
                "row": row,
                "column": column,
                "utf8_column": utf8_column,
                "utf16_column": utf16_column,
                "x": layout_position[0],
                "y": layout_position[1],
                "line_x": layout_position[0] - line_origins[row][0],
                "layout_roundtrip_point": view.layout_to_text(
                    (layout_position[0], layout_position[1])
                ),
            }
        )

    return {
        "schema_version": 1,
        "sublime_version": sublime.version(),
        "platform": sublime.platform(),
        "arch": sublime.arch(),
        "text_name": os.path.basename(text_path),
        "text_utf8_sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
        "face": face,
        "size": float(size),
        "font_options": list(font_options),
        "line_height": float(view.line_height()),
        "em_width": float(view.em_width()),
        "layout_extent": _vector(view.layout_extent()),
        "line_origins": [
            {"row": row, "x": origin[0], "y": origin[1]}
            for row, origin in sorted(line_origins.items())
        ],
        "points": points,
    }


def _write_result(output_path, payload):
    output_directory = os.path.dirname(output_path)
    if output_directory:
        os.makedirs(output_directory, exist_ok=True)
    temporary_path = f"{output_path}.tmp"
    with open(temporary_path, "w", encoding="utf-8") as output_file:
        json.dump(payload, output_file, ensure_ascii=False, indent=2, sort_keys=True)
        output_file.write("\n")
    os.replace(temporary_path, output_path)


def _sample_when_stable(
    view,
    text,
    text_path,
    face,
    size,
    font_options,
    output_path,
    previous_payload,
    stable_samples,
    attempts,
):
    if not view.is_valid() or view.settings().get(_CASE_SETTING) != output_path:
        return

    try:
        payload = _collect_metrics(view, text, text_path, face, size, font_options)
    except Exception as error:
        _write_result(
            output_path,
            {
                "schema_version": 1,
                "error": f"{type(error).__name__}: {error}",
                "text_name": os.path.basename(text_path),
                "face": face,
                "size": float(size),
            },
        )
        return

    matching_samples = stable_samples + 1 if payload == previous_payload else 1
    if matching_samples >= _STABLE_SAMPLE_COUNT:
        _write_result(output_path, payload)
        return
    if attempts >= _MAX_SAMPLE_ATTEMPTS:
        payload["error"] = "layout metrics did not stabilize"
        _write_result(output_path, payload)
        return

    sublime.set_timeout(
        lambda: _sample_when_stable(
            view,
            text,
            text_path,
            face,
            size,
            font_options,
            output_path,
            payload,
            matching_samples,
            attempts + 1,
        ),
        _POLL_INTERVAL_MS,
    )


class MetricsProbeCommand(sublime_plugin.WindowCommand):
    def run(self, text_path=None, face=None, size=None, output_path=None, font_options=None):
        if text_path is None:
            request_path = os.path.join(sublime.packages_path(), "User", _REQUEST_NAME)
            with open(request_path, encoding="utf-8-sig") as request_file:
                request = json.load(request_file)
            text_path = request["text_path"]
            face = request["face"]
            size = request["size"]
            output_path = request["output_path"]
            font_options = request.get("font_options")
        if face is None or size is None or output_path is None:
            raise ValueError("text_path, face, size, and output_path are required")

        with open(text_path, encoding="utf-8") as text_file:
            text = text_file.read()

        options = list(font_options) if font_options is not None else []
        view = _probe_view(self.window)
        self.window.focus_view(view)
        _replace_text(view, text)
        view.settings().set("font_face", face)
        view.settings().set("font_size", float(size))
        view.settings().set("font_options", options)
        view.settings().set(_CASE_SETTING, output_path)

        sublime.set_timeout(
            lambda: _sample_when_stable(
                view,
                text,
                text_path,
                face,
                size,
                options,
                output_path,
                None,
                0,
                1,
            ),
            0,
        )
