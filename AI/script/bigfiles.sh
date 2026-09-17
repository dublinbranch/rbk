#!/usr/bin/env bash
# List the biggest files an AI tool can see in this repo.
# Uses git to find tracked + untracked files that .gitignore does not exclude
# (the same set that Claude Code / ripgrep search).
#
# Usage: script/ai-bigfiles.sh [-n COUNT] [-a] [-t] [-w SECONDS]
#   -n COUNT    number of files to show (default 30)
#   -a          include skipped paths (vendor/, drop/)
#   -t          text files only (skip images and other binary files)
#   -w SECONDS  watch mode: refresh every SECONDS

set -euo pipefail

count=30
all=0
textonly=0
watch=0

while getopts "n:atw:h" opt; do
	case "$opt" in
		n) count="$OPTARG" ;;
		a) all=1 ;;
		t) textonly=1 ;;
		w) watch="$OPTARG" ;;
		*) sed -n '2,12p' "$0"; exit 0 ;;
	esac
done

cd "$(git rev-parse --show-toplevel)"

# Paths from .claude/rules/diter-skip-vendor.md
skip_re='(^|/)vendor/|^drop/'

report() {
	local files
	files=$(git ls-files -co --exclude-standard | while IFS= read -r f; do
		[[ -f "$f" ]] || continue
		if [[ $all -eq 0 && "$f" =~ $skip_re ]]; then continue; fi
		echo "$f"
	done)

	local total_bytes=0 total_files=0
	printf '%-10s %-8s %-9s %-4s %s\n' "SIZE" "LINES" "~TOKENS" "TYPE" "FILE"
	while IFS= read -r f; do
		local bytes type lines tokens
		bytes=$(stat -c %s "$f")
		if grep -Iq . "$f" 2>/dev/null || [[ ! -s "$f" ]]; then
			type=txt
			lines=$(wc -l < "$f")
			tokens=$((bytes / 4))
		else
			[[ $textonly -eq 1 ]] && continue
			type=bin
			lines=-
			tokens=-
		fi
		total_bytes=$((total_bytes + bytes))
		total_files=$((total_files + 1))
		printf '%s\t%s\t%s\t%s\t%s\n' "$bytes" "$lines" "$tokens" "$type" "$f"
	done <<< "$files" | sort -t$'\t' -k1,1rn | head -n "$count" |
		while IFS=$'\t' read -r bytes lines tokens type f; do
			printf '%-10s %-8s %-9s %-4s %s\n' \
				"$(numfmt --to=iec "$bytes")" "$lines" "$tokens" "$type" "$f"
		done || true # head closes the pipe early: SIGPIPE is expected

	local nfiles tbytes
	nfiles=$(grep -c . <<< "$files" || true)
	tbytes=$(tr '\n' '\0' <<< "$files" | xargs -0 stat -c %s 2>/dev/null | paste -sd+ | bc)
	echo
	echo "Files visible: $nfiles  Total: $(numfmt --to=iec "${tbytes:-0}")"
	[[ $all -eq 0 ]] && echo "Skipped: vendor/, drop/ (use -a to include)"
	return 0
}

if [[ "$watch" -gt 0 ]]; then
	while true; do
		clear
		echo "$(date '+%F %T')  refresh every ${watch}s  (Ctrl+C to stop)"
		echo
		report
		sleep "$watch"
	done
else
	report
fi
