#!/usr/bin/env python3
"""Reads an editor scroll log (EDITOR_SCROLL_TRACE=1) and either reports on it or turns it into a
replayable px-scroll-trace-v1 file.

  scroll_log_report.py report LOG [--trace TRACE] [--window SECONDS] [--all]
  scroll_log_report.py trace LOG OUT.tsv

The log has one line per scroll event, display tick, presented frame and mark (a click in the
document while tracing). The report lists every frame around each mark, flagging the ones that
would look wrong: a frame the window server never showed, a frame that landed a refresh late,
a step out of line with its neighbours, and input that reached the main thread late. With no
marks, or with --all, it lists every flagged frame in the log. --trace takes the marks from a
trace file's "# mark" comments instead, aligned to the log's first scroll event, which is how a
replay is compared with its recording.
"""

import argparse
import statistics
import sys

REFRESH_TOLERANCE = 0.5  # ms either side of the refresh interval still counts as on time
STEP_DEVIATION = 0.35  # fraction of the neighbours' mean step that counts as a wrong step
VISIBLE_STEP = 2.0  # pt per frame below which nothing is worth flagging
LATE_INPUT_MS = 4.0  # main-thread delay beyond the typical one that counts as late
DEFAULT_WINDOW = 1.5  # seconds before a mark to report


def parse_value(text):
    for suffix in ("ms", "pt"):
        if text.endswith(suffix):
            text = text[: -len(suffix)]
    try:
        return float(text)
    except ValueError:
        return text


def read_log(path):
    records = []
    with open(path) as handle:
        for line in handle:
            parts = line.split()
            if not parts or parts[0] not in ("scroll", "tick", "present", "mark"):
                continue
            fields = {}
            for part in parts[1:]:
                if "=" in part:
                    key, value = part.split("=", 1)
                    fields[key] = parse_value(value)
            records.append((parts[0], fields))
    return records


def scroll_records(records):
    return [f for kind, f in records if kind == "scroll"]


def present_records(records):
    return [f for kind, f in records if kind == "present"]


def refresh_interval(presents):
    shown = [p["interval"] for p in presents if p["t"] > 0 and 1.0 < p["interval"] < 100.0]
    return statistics.median(shown) if shown else 8.333


def flag_presents(presents, refresh):
    """Attaches a list of flags to every present record. Frames moving under VISIBLE_STEP are
    left alone: a momentum tail at 30 Hz input legitimately shows one-point steps 33 ms apart."""
    for index, frame in enumerate(presents):
        flags = []
        if frame["t"] == 0:
            if abs(frame["step"]) >= VISIBLE_STEP:
                flags.append("never displayed")
        elif index > 0 and presents[index - 1]["t"] > 0 and abs(frame["step"]) >= VISIBLE_STEP:
            late = frame["interval"] / refresh
            if late > 1.0 + REFRESH_TOLERANCE / refresh and late < 12:
                plural = "" if late < 2.5 else "es"
                flags.append(f"late {late - 1:.0f} refresh{plural}")
        if 0 < index < len(presents) - 1:
            before = presents[index - 1]["step"]
            after = presents[index + 1]["step"]
            reference = (before + after) / 2.0
            if abs(reference) >= VISIBLE_STEP:
                deviation = frame["step"] - reference
                if abs(deviation) > STEP_DEVIATION * abs(reference):
                    what = "reverse" if frame["step"] * reference < 0 else (
                        "short" if abs(frame["step"]) < abs(reference) else "long")
                    flags.append(f"{what} step ({frame['step']:.1f} vs {reference:.1f})")
        frame["flags"] = flags


def flag_scrolls(scrolls):
    intervals = [s["dt"] for s in scrolls[1:] if 0 < s["dt"] < 200]
    typical = statistics.median(intervals) if intervals else 8.333
    typical_lag = statistics.median(s["lag"] for s in scrolls)
    for event in scrolls:
        flags = []
        if event["lag"] > typical_lag + LATE_INPUT_MS:
            flags.append(f"reached main thread {event['lag']:.1f} ms late")
        if 0 < event["dt"] < 200 and event["dt"] > 2.5 * typical and abs(event["delta"]) >= 1:
            flags.append(f"{event['dt']:.1f} ms since previous event")
        event["flags"] = flags


def marks_from_log(records):
    return [f["t"] for kind, f in records if kind == "mark"]


def marks_from_trace(path, first_scroll_time):
    marks = []
    with open(path) as handle:
        for line in handle:
            parts = line.split()
            if len(parts) == 3 and parts[0] == "#" and parts[1] == "mark":
                marks.append(first_scroll_time + int(parts[2]) / 1e9)
    return marks


def timeline(records, marks):
    """Every record with a time, in time order. Frames that never reached the glass have no time
    of their own and are placed just before the next frame that did."""
    entries = []
    presents = present_records(records)
    for index, frame in enumerate(presents):
        time = frame["t"]
        if time == 0:
            later = [f["t"] for f in presents[index + 1:] if f["t"] > 0]
            earlier = [f["t"] for f in presents[:index] if f["t"] > 0]
            if later:
                time = later[0] - 1e-6
            elif earlier:
                time = earlier[-1] + 1e-6
            else:
                continue
        entries.append((time, 1, "present", frame))
    for event in scroll_records(records):
        entries.append((event["ts"], 0, "scroll", event))
    for mark in marks:
        entries.append((mark, 2, "mark", None))
    entries.sort(key=lambda entry: (entry[0], entry[1]))
    return entries


def describe(kind, fields, origin, time):
    at = f"  {time - origin:8.3f}"
    if kind == "present":
        flags = "  <-- " + "; ".join(fields["flags"]) if fields["flags"] else ""
        frame = f"frame {fields['frame']:4.0f}"
        when = ("never displayed        " if fields["t"] == 0
                else f"interval {fields['interval']:6.3f} ms")
        return f"{at}  {frame}  {when}  step {fields['step']:7.2f} pt{flags}"
    if kind == "scroll":
        return f"{at}  input delta {fields['delta']:6.2f}  <-- {'; '.join(fields['flags'])}"
    return f"{at}  MARK"


def print_window(entries, origin, start, end):
    """Prints frames and flagged input between start and end, times relative to origin."""
    for time, _, kind, fields in entries:
        if time < start or time > end:
            continue
        if kind == "scroll" and not fields["flags"]:
            continue
        print(describe(kind, fields, origin, time))


def report(args):
    records = read_log(args.log)
    scrolls = scroll_records(records)
    presents = present_records(records)
    if not scrolls:
        print(f"no scroll events in {args.log}")
        return 1
    refresh = refresh_interval(presents)
    flag_presents(presents, refresh)
    flag_scrolls(scrolls)

    origin = scrolls[0]["ts"]
    shown = [p for p in presents if p["t"] > 0]
    on_time = sum(1 for p in shown if abs(p["interval"] - refresh) <= REFRESH_TOLERANCE)
    print(f"{args.log}: {len(scrolls)} scroll events, {len(shown)} frames presented, "
          f"{len(presents) - len(shown)} never displayed, refresh {refresh:.3f} ms")
    flagged_frames = sum(1 for p in presents if p["flags"])
    late_inputs = sum(1 for s in scrolls if s["flags"])
    print(f"frames on the refresh cadence: {on_time} of {len(shown) - 1}; "
          f"flagged frames: {flagged_frames}; late input events: {late_inputs}")

    marks = marks_from_trace(args.trace, origin) if args.trace else marks_from_log(records)
    if args.trace:
        with open(args.trace) as handle:
            expected = sum(1 for line in handle if line.strip() and not line.startswith("#"))
        if len(scrolls) > expected:
            print(f"WARNING: {len(scrolls)} scroll events reached the editor but the trace has "
                  f"{expected}: other input got in during the replay, so marks may not line up")
    entries = timeline(records, marks)
    if marks and not args.all:
        for number, mark in enumerate(marks, 1):
            print(f"\nmark {number} at {mark - origin:.3f} s "
                  f"(showing the {args.window:.1f} s before it):")
            print_window(entries, origin, mark - args.window, mark + 0.05)
    else:
        print("\nevery flagged frame and input:" if marks else "\nno marks; every flagged frame and input:")
        for time, _, kind, fields in entries:
            if kind != "mark" and fields["flags"]:
                print(describe(kind, fields, origin, time))
    return 0


def write_trace(args):
    records = read_log(args.log)
    scrolls = scroll_records(records)
    if not scrolls:
        print(f"no scroll events in {args.log}", file=sys.stderr)
        return 1
    origin = scrolls[0]["ts"]
    with open(args.out, "w") as out:
        out.write("# px-scroll-trace-v1\n")
        out.write("# time_ns\tdelta_x\tdelta_y\tscrolling_delta_x\tscrolling_delta_y\tprecise\t"
                  "phase\tmomentum_phase\tline_delta_x\tline_delta_y\tfixed_delta_x\t"
                  "fixed_delta_y\tpoint_delta_x\tpoint_delta_y\tcontinuous\n")
        out.write("# recorded from an editor scroll log; phases are not recorded, so motion is\n"
                  "# tagged as a changing drag and the rest as phase-less\n")
        for mark in marks_from_log(records):
            out.write(f"# mark {round((mark - origin) * 1e9)}\n")
        previous_ns = -1
        for event in scrolls:
            time_ns = max(previous_ns + 1, round((event["ts"] - origin) * 1e9))
            previous_ns = time_ns
            # The editor logs the negated scrolling delta.
            delta = -event["delta"]
            phase = 2 if delta != 0 else 0
            out.write(f"{time_ns}\t0\t{delta!r}\t0\t{delta!r}\t1\t{phase}\t0\t0\t"
                      f"{round(delta)}\t0\t{delta!r}\t0\t{round(delta)}\t1\n")
    print(f"wrote {len(scrolls)} samples and {len(marks_from_log(records))} marks to {args.out}")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    report_parser = commands.add_parser("report")
    report_parser.add_argument("log")
    report_parser.add_argument("--trace", help="take marks from this trace's comments")
    report_parser.add_argument("--window", type=float, default=DEFAULT_WINDOW)
    report_parser.add_argument("--all", action="store_true", help="list every flagged frame")
    report_parser.set_defaults(run=report)
    trace_parser = commands.add_parser("trace")
    trace_parser.add_argument("log")
    trace_parser.add_argument("out")
    trace_parser.set_defaults(run=write_trace)
    args = parser.parse_args()
    return args.run(args)


if __name__ == "__main__":
    sys.exit(main())
