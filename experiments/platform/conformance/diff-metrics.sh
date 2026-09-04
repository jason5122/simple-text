#!/bin/bash
# Compares font metrics exactly and projects raw shaper X positions into View coordinates.
set -e

cd "$(dirname "$0")"

if ! command -v jq >/dev/null; then
  echo "jq is required" >&2
  exit 2
fi

metrics=(our-metrics/*.json)
if [[ ! -f "${metrics[0]}" ]]; then
  echo "no JSON metrics found under our-metrics/" >&2
  exit 2
fi
if [[ ! -d sublime-metrics ]]; then
  echo "no sublime-metrics/ reference directory found" >&2
  exit 2
fi

jq_program='
  def close($left; $right; $tolerance):
    (($left - $right) | fabs) <= $tolerance;
  def half_up($value):
    (($value + 0.5) | floor);
  def near_half($value):
    close($value - ($value | floor); 0.5; 0.0005);
  def x_matches($raw; $view):
    half_up($raw) == $view or
      (near_half($raw) and
       ($view == ($raw | floor) or $view == (($raw | floor) + 1)));

  $ours[0] as $o |
  $reference[0] as $r |
  (($o.points | length) == ($r.points | length)) as $same_point_count |
  ([
    if ($o.schema_version == $r.schema_version and
        $o.platform == $r.platform and
        $o.text_name == $r.text_name and
        $o.face == $r.face and
        $o.size == $r.size and
        $o.font_options == $r.font_options)
      then empty else "identity" end,
    if close($o.line_height; $r.line_height; 0.000000001)
      then empty else "line_height" end,
    if close($o.em_width; $r.em_width; 0.000000001)
      then empty else "em_width" end,
    if (close($o.layout_extent[0]; $r.layout_extent[0]; 0.005) and
        close($o.layout_extent[1]; $r.layout_extent[1]; 0.005))
      then empty else "layout_extent" end,
    if $o.line_origins == $r.line_origins then empty else "line_origins" end,
    if ($same_point_count and all(range(0; $o.points | length);
        $o.points[.] as $op | $r.points[.] as $rp |
        $op.point == $rp.point and
        $op.row == $rp.row and
        $op.column == $rp.column and
        $op.utf8_column == $rp.utf8_column and
        $op.utf16_column == $rp.utf16_column))
      then empty else "point_indices" end,
    if ($same_point_count and all(range(0; $o.points | length);
        $o.points[.] as $op | $r.points[.] as $rp |
        $op.y == $rp.y))
      then empty else "point_y" end,
    if ($same_point_count and all(range(0; $o.points | length);
        $o.points[.] as $op | $r.points[.] as $rp |
        x_matches($op.x; $rp.x) and x_matches($op.line_x; $rp.line_x)))
      then empty else "point_x" end
  ]) as $failures |
  (if $same_point_count then
     any(range(0; $o.points | length);
       $o.points[.] as $op | $r.points[.] as $rp |
       half_up($op.line_x) != $rp.line_x and
       near_half($op.line_x) and x_matches($op.line_x; $rp.line_x))
   else false end) as $rounding_boundary |
  [
    (if ($failures | length) == 0 then "-" else ($failures | join(",")) end),
    (if $rounding_boundary then "boundary" else "-" end)
  ] | @tsv
'

differences=metrics-diffs.txt
details=metrics-diff-details.txt
boundaries=metrics-rounding-boundaries.txt
correct=metrics-correct.txt
rm -f "$differences" "$details" "$boundaries" "$correct"
touch "$differences" "$details" "$boundaries" "$correct"

for ours in "${metrics[@]}"; do
  name=$(basename "$ours")
  reference="sublime-metrics/$name"
  if [[ ! -f "$reference" ]]; then
    echo "missing reference: $reference" >&2
    printf '%s\n' "$name" >>"$differences"
    printf '%s\t%s\n' "$name" "missing_reference" >>"$details"
    continue
  fi
  if ! jq -e . "$ours" >/dev/null; then
    echo "invalid metrics: $ours" >&2
    printf '%s\n' "$name" >>"$differences"
    printf '%s\t%s\n' "$name" "invalid_ours" >>"$details"
    continue
  fi
  if ! jq -e . "$reference" >/dev/null; then
    echo "invalid metrics: $reference" >&2
    printf '%s\n' "$name" >>"$differences"
    printf '%s\t%s\n' "$name" "invalid_reference" >>"$details"
    continue
  fi
  comparison=$(jq -nr --slurpfile ours "$ours" --slurpfile reference "$reference" "$jq_program")
  failures=${comparison%%$'\t'*}
  boundary=${comparison#*$'\t'}
  if [[ "$failures" != "-" ]]; then
    printf '%s\n' "$name" >>"$differences"
    printf '%s\t%s\n' "$name" "$failures" >>"$details"
  else
    printf '%s\n' "$name" >>"$correct"
  fi
  if [[ "$boundary" == "boundary" ]]; then
    printf '%s\n' "$name" >>"$boundaries"
  fi
done

sort -o "$differences" "$differences"
sort -o "$details" "$details"
sort -o "$boundaries" "$boundaries"
sort -o "$correct" "$correct"
printf "%-12s %5d\n" "Compared:" "${#metrics[@]}"
printf "%-12s %5d\n" "Conformant:" "$(wc -l <"$correct")"
printf "%-12s %5d\n" "Boundaries:" "$(wc -l <"$boundaries")"
printf "%-12s %5d\n" "Different:" "$(wc -l <"$differences")"
if [[ -s "$boundaries" ]]; then
  printf "Boundary cases are included in Conformant.\n"
fi
printf "Saved under %s/metrics-*.txt\n" "$PWD"
