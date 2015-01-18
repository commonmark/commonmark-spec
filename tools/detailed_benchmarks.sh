sudo echo ""
for x in samples/*.md; do make benchjs BENCHINP=$x; done | awk -f tools/format_benchmarks.awk
