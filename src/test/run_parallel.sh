#!/bin/bash
# run_parallel.sh STEPS — the steps of a test list as parallel jobs
#
# STEPS: a file of lines "title|command" (# comments, blank lines skipped).
# Up to TEST_JOBS commands run at once (default: one per core); each one's
# output goes to a log of its own, shown after the title in the list's order
# once all have finished. The exit status is 1 when any command failed (they
# are named at the end), else 0.
steps="$1"
[ -f "$steps" ] || { echo "run_parallel.sh: no step list '$steps'" >&2; exit 2; }
JOBS=${TEST_JOBS:-$(nproc 2>/dev/null || echo 2)}
LD=build/test/steps
rm -rf "$LD"; mkdir -p "$LD"
titles=(); n=0
while IFS= read -r line || [ -n "$line" ]; do
    case "$line" in ''|'#'*) continue ;; esac
    title="${line%%|*}"; cmd="${line#*|}"
    titles+=("$title")
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || true; done
    ( bash -c "$cmd" > "$LD/$n.log" 2>&1; echo $? > "$LD/$n.rc" ) &
    n=$((n + 1))
done < "$steps"
wait
bad=()
for ((i = 0; i < n; i++)); do
    echo "${titles[$i]}"
    cat "$LD/$i.log"
    [ "$(cat "$LD/$i.rc" 2>/dev/null)" = 0 ] || bad+=("${titles[$i]}")
done
if [ ${#bad[@]} -gt 0 ]; then
    echo ""
    echo "FAILED steps (${#bad[@]} of $n):"
    for t in "${bad[@]}"; do echo "  $t"; done
    exit 1
fi
exit 0
