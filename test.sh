#!/usr/bin/env bash
# ============================================================================
#  Famine — regression test suite
#
#  Run from the project root ON THE PROJECT VM:   ./test.sh
#  Rebuilds Famine if missing;  pass --rebuild to force `make re` first.
#
#  Exit status: 0 if every test passes, 1 otherwise — so it drops straight
#  into a pre-commit hook or `make test` without extra plumbing.
#
#  NOTE: this exercises the real, hard-coded target folders /tmp/test and
#  /tmp/test2. It wipes and recreates them each run. Intended for the project
#  VM, exactly the environment the subject describes — it builds and runs the
#  infector, so only run it there, never on your host.
# ============================================================================

set -u

# ---- config ---------------------------------------------------------------
FAMINE="${FAMINE:-./Famine}"
T1="/tmp/test"
T2="/tmp/test2"
T3="/tmp/test3"                   # a sibling dir that is NOT a target
WORK="/tmp/famine_test_work"      # baselines + fixtures scratch
SIG="cbopp-mvan-wij"              # distinctive slice of the stub signature
CANARY="/tmp/famine_canary"       # lives OUTSIDE the target dirs

# ---- colours / counters ---------------------------------------------------
G='\033[32m'; R='\033[31m'; Y='\033[33m'; B='\033[1m'; D='\033[2m'; N='\033[0m'
PASS=0; FAIL=0; SKIP=0

pass()    { printf "  ${G}✓${N} %s\n" "$1"; PASS=$((PASS+1)); }
fail()    { printf "  ${R}✗ %s${N}\n" "$1"; FAIL=$((FAIL+1)); }
skip()    { printf "  ${Y}~${N} ${D}%s (skipped: %s)${N}\n" "$1" "$2"; SKIP=$((SKIP+1)); }
section() { printf "\n${B}%s${N}\n" "$1"; }   # renamed: do NOT shadow `head`

md5()  { md5sum "$1" 2>/dev/null | awk '{print $1}'; }
mode() { stat -c '%a' "$1" 2>/dev/null; }
sigcount() { strings "$1" 2>/dev/null | grep -o "$SIG" | wc -l | tr -d ' '; }

# assert two strings equal
eq() { # label expected actual
	if [ "$2" = "$3" ]; then pass "$1"; else fail "$1 (expected '$2', got '$3')"; fi
}

declare -A BASE_MD5   # fixture -> md5 before any infection

# ---------------------------------------------------------------------------
#  0. build
# ---------------------------------------------------------------------------
section "Build"
if [ "${1:-}" = "--rebuild" ]; then
	make re >/dev/null 2>&1
fi
if [ ! -x "$FAMINE" ]; then
	make >/dev/null 2>&1
fi
if [ -x "$FAMINE" ]; then pass "Famine binary present"; else
	fail "Famine binary missing — build failed"; printf "\nAborting.\n"; exit 1
fi
if [ "$(basename "$FAMINE")" = "Famine" ]; then pass "executable is named 'Famine'"
else fail "executable must be named 'Famine' (got '$(basename "$FAMINE")')"; fi

# ---------------------------------------------------------------------------
#  1. fixtures
# ---------------------------------------------------------------------------
section "Fixtures"
rm -rf "$T1" "$T2" "$T3" "$WORK" "$CANARY"
mkdir -p "$T1" "$T2" "$T3" "$WORK"

cat > "$WORK/sample.c" <<'EOF'
#include <stdio.h>
int main(void) { printf("Hello, World!\n"); return 0; }
EOF

# PIE (ET_DYN) and non-PIE (ET_EXEC) — both must infect
if cc "$WORK/sample.c" -o "$T1/sample_pie" >/dev/null 2>&1 \
   && cc -no-pie "$WORK/sample.c" -o "$T1/sample_nopie" >/dev/null 2>&1; then
	pass "compiled PIE + non-PIE samples"
	HAVE_SAMPLES=1
else
	fail "could not compile sample fixtures"; HAVE_SAMPLES=0
fi

# a real, stripped, dynamically-linked system binary
cp "$(command -v ls)" "$T2/ls"

# probe dir so infected `ls` has stable output to compare
mkdir -p "$WORK/probe"; touch "$WORK/probe/aaa" "$WORK/probe/bbb" "$WORK/probe/ccc"

# 32-bit ELF — must be REJECTED (mandatory is 64-bit only), left untouched
if cc -m32 "$WORK/sample.c" -o "$T1/sample_32" >/dev/null 2>&1; then
	HAVE_32=1
else
	HAVE_32=0   # no multilib on this box — that test will skip
fi

# malformed / non-target inputs — all must be left byte-identical, no crash
printf '#!/bin/sh\necho hi\n'        > "$T1/script.sh"       # non-ELF text
head -c 32 "$T2/ls"                  > "$T1/truncated.elf"   # valid magic, too short to be real
head -c 8  "$T2/ls"                  > "$T1/tiny.elf"        # smaller than Elf64_Ehdr
: > "$T1/empty.elf"                                          # zero bytes

# canary OUTSIDE the target dirs — containment check
cp "$(command -v ls)" "$CANARY"

# a sibling dir that is NOT a target, plus a symlink from a target dir pointing
# into it: the walker must ignore the sibling entirely and must NOT follow the
# symlink to infect the binary it points at.
cp "$(command -v ls)" "$T3/outsider"
ln -sf "$T3/outsider" "$T1/escape_link"

# ---------------------------------------------------------------------------
#  2. baselines (capture BEFORE any infection)
# ---------------------------------------------------------------------------
for f in "$T1/sample_pie" "$T1/sample_nopie" "$T2/ls" "$T1/sample_32" \
         "$T1/script.sh" "$T1/truncated.elf" "$T1/tiny.elf" "$T1/empty.elf" \
         "$CANARY" "$T3/outsider"; do
	[ -e "$f" ] && BASE_MD5["$f"]=$(md5 "$f")
done
BASE_MODE_PIE=$(mode "$T1/sample_pie")
BASE_MODE_LS=$(mode "$T2/ls")

# original program outputs, captured from the clean binaries
[ "$HAVE_SAMPLES" = 1 ] && OUT_PIE_BEFORE=$("$T1/sample_pie")
[ "$HAVE_SAMPLES" = 1 ] && OUT_NOPIE_BEFORE=$("$T1/sample_nopie")
LS_BEFORE=$("$T2/ls" "$WORK/probe")

# ---------------------------------------------------------------------------
#  3. RUN #1  — real run, malformed files present in the folders
# ---------------------------------------------------------------------------
section "Silence (run #1, with malformed inputs present)"
NAMES_BEFORE=$(ls -1a "$T1" "$T2" 2>/dev/null | sort)   # low-profile: name set must not change
"$FAMINE" >"$WORK/out1" 2>"$WORK/err1"; RC1=$?

eq  "exit status 0 (no crash)" "0" "$RC1"
[ $RC1 -ge 128 ] && fail "killed by signal $((RC1-128)) — a malformed file faulted the walker"
eq  "stdout empty"            "0" "$(wc -c < "$WORK/out1" | tr -d ' ')"
eq  "stderr empty"            "0" "$(wc -c < "$WORK/err1" | tr -d ' ')"
if [ "$NAMES_BEFORE" = "$(ls -1a "$T1" "$T2" 2>/dev/null | sort)" ]; then
	pass "no stray files created in target dirs (modifies in place only)"
else fail "Famine added or removed files in the target dirs — not low-profile"; fi

# ---------------------------------------------------------------------------
#  4. infection correctness
# ---------------------------------------------------------------------------
section "Infection"
check_infected() { # label file
	local label="$1" file="$2"
	if [ "$(sigcount "$file")" -ge 1 ]; then pass "$label: signature embedded"
	else fail "$label: signature missing"; fi
	if [ "$(md5 "$file")" != "${BASE_MD5[$file]}" ]; then pass "$label: file rewritten"
	else fail "$label: file unchanged (not infected)"; fi
}

if [ "$HAVE_SAMPLES" = 1 ]; then
	check_infected "PIE sample"     "$T1/sample_pie"
	check_infected "non-PIE sample" "$T1/sample_nopie"
	eq "PIE sample still runs identically"     "$OUT_PIE_BEFORE"   "$("$T1/sample_pie" 2>/dev/null)"
	eq "non-PIE sample still runs identically" "$OUT_NOPIE_BEFORE" "$("$T1/sample_nopie" 2>/dev/null)"
else
	skip "sample infection" "no compiler fixtures"
fi

check_infected "/bin/ls copy" "$T2/ls"
eq "infected ls still runs identically" "$LS_BEFORE" "$("$T2/ls" "$WORK/probe" 2>/dev/null)"

section "Permissions preserved"
eq "sample_pie mode unchanged" "$BASE_MODE_PIE" "$(mode "$T1/sample_pie")"
eq "ls mode unchanged"         "$BASE_MODE_LS"  "$(mode "$T2/ls")"

# ---------------------------------------------------------------------------
#  5. rejection — non-targets left byte-identical
# ---------------------------------------------------------------------------
section "Rejection (must be left untouched)"
assert_untouched() { # label file
	if [ "$(md5 "$2")" = "${BASE_MD5[$2]}" ]; then pass "$1: untouched"
	else fail "$1: MODIFIED — should have been skipped"; fi
}
assert_untouched "non-ELF script"  "$T1/script.sh"
assert_untouched "truncated ELF"   "$T1/truncated.elf"
assert_untouched "sub-header ELF"  "$T1/tiny.elf"
assert_untouched "empty file"      "$T1/empty.elf"
if [ "$HAVE_32" = 1 ]; then assert_untouched "32-bit ELF" "$T1/sample_32"
else skip "32-bit ELF rejection" "no -m32 multilib"; fi

section "Containment"
assert_untouched "file outside /tmp/test* untouched" "$CANARY"
assert_untouched "sibling dir /tmp/test3 untouched"  "$T3/outsider"
if [ "$(sigcount "$T3/outsider")" = "0" ]; then
	pass "symlink in a target dir not followed (binary outside stays clean)"
else fail "symlink followed — infected a binary outside the target dirs"; fi

# ---------------------------------------------------------------------------
#  6. RUN #2 — idempotency / the dedup guard
# ---------------------------------------------------------------------------
section "Idempotency (run #2 — the signature guard)"
declare -A AFTER1_MD5
for f in "$T1/sample_pie" "$T1/sample_nopie" "$T2/ls"; do
	[ -e "$f" ] && AFTER1_MD5["$f"]=$(md5 "$f")
done

"$FAMINE" >"$WORK/out2" 2>"$WORK/err2"; RC2=$?
eq "run #2 silent (stdout)" "0" "$(wc -c < "$WORK/out2" | tr -d ' ')"
eq "run #2 silent (stderr)" "0" "$(wc -c < "$WORK/err2" | tr -d ' ')"
eq "run #2 exit 0"          "0" "$RC2"

for f in "$T1/sample_pie" "$T1/sample_nopie" "$T2/ls"; do
	[ -e "$f" ] || continue
	label=$(basename "$f")
	if [ "$(md5 "$f")" = "${AFTER1_MD5[$f]}" ]; then pass "$label: byte-identical after re-run"
	else fail "$label: CHANGED on re-run — guard not firing (double infection)"; fi
	eq "$label: signature appears exactly once" "1" "$(sigcount "$f")"
done

# ---------------------------------------------------------------------------
#  7. generational — fresh targets dropped in between runs
#     The realistic grader move: infect, add more binaries, infect again.
#     New ones must get infected exactly once; already-signed ones must stay
#     byte-identical (the guard holds while the folder contents change).
# ---------------------------------------------------------------------------
section "Generational (new targets added between runs)"

# snapshot the already-infected files — a later run must NOT touch them again
declare -A PRE_GEN_MD5
for f in "$T1/sample_pie" "$T2/ls"; do
	[ -e "$f" ] && PRE_GEN_MD5["$f"]=$(md5 "$f")
done

# stage fresh, never-seen targets in both dirs
GEN_OK=1
if [ "$HAVE_SAMPLES" = 1 ]; then
	cc "$WORK/sample.c" -o "$T1/gen_sample" >/dev/null 2>&1 || GEN_OK=0
else
	GEN_OK=0
fi
cp "$(command -v cat)" "$T2/gen_cat" 2>/dev/null || GEN_OK=0

if [ "$GEN_OK" = 1 ]; then
	GEN_SAMPLE_OUT=$("$T1/gen_sample")
	GEN_SAMPLE_MD5=$(md5 "$T1/gen_sample")
	GEN_CAT_MD5=$(md5 "$T2/gen_cat")

	"$FAMINE" >"$WORK/outg" 2>"$WORK/errg"; RCG=$?
	eq "generational run silent (stdout)" "0" "$(wc -c < "$WORK/outg" | tr -d ' ')"
	eq "generational run silent (stderr)" "0" "$(wc -c < "$WORK/errg" | tr -d ' ')"
	eq "generational run exit 0"          "0" "$RCG"

	# the NEW targets must now be infected, exactly once, still working
	if [ "$(sigcount "$T1/gen_sample")" = "1" ] && [ "$(md5 "$T1/gen_sample")" != "$GEN_SAMPLE_MD5" ]; then
		pass "new sample infected exactly once"
	else fail "new sample not infected or double-signed"; fi
	eq "new sample still runs identically" "$GEN_SAMPLE_OUT" "$("$T1/gen_sample" 2>/dev/null)"

	if [ "$(sigcount "$T2/gen_cat")" = "1" ] && [ "$(md5 "$T2/gen_cat")" != "$GEN_CAT_MD5" ]; then
		pass "new system binary infected exactly once"
	else fail "new system binary not infected or double-signed"; fi
	if echo hi | "$T2/gen_cat" >/dev/null 2>&1; then pass "infected cat still runs"
	else fail "infected cat broken"; fi

	# the PREVIOUSLY infected files must be byte-identical — guard held
	for f in "$T1/sample_pie" "$T2/ls"; do
		[ -e "$f" ] || continue
		label=$(basename "$f")
		if [ "$(md5 "$f")" = "${PRE_GEN_MD5[$f]}" ]; then pass "$label: untouched by generational run"
		else fail "$label: re-infected when new targets were added"; fi
		eq "$label: signature still exactly once" "1" "$(sigcount "$f")"
	done
else
	skip "generational test" "could not stage fresh targets"
fi

# ---------------------------------------------------------------------------
#  8. best-effort: a real binary with TWO PT_NOTE segments
#     (the case the accidental guard misses; needs the explicit memmem check)
# ---------------------------------------------------------------------------
section "Double-PT_NOTE target (best effort)"
TWONOTE=""
if command -v readelf >/dev/null 2>&1; then
	for cand in /usr/bin/* /bin/*; do
		[ -f "$cand" ] || continue
		n=$(readelf -l "$cand" 2>/dev/null | grep -c '  NOTE')
		if [ "${n:-0}" -ge 2 ]; then TWONOTE="$cand"; break; fi
	done
fi
if [ -n "$TWONOTE" ]; then
	cp "$TWONOTE" "$T2/twonote"
	"$FAMINE" >/dev/null 2>&1          # infect once
	m=$(md5 "$T2/twonote")
	"$FAMINE" >/dev/null 2>&1          # second pass must not touch it
	if [ "$(md5 "$T2/twonote")" = "$m" ] && [ "$(sigcount "$T2/twonote")" = "1" ]; then
		pass "two-PT_NOTE binary infected once, idempotent ($(basename "$TWONOTE"))"
	else
		fail "two-PT_NOTE binary re-infected — explicit signature guard needed"
	fi
else
	skip "double-PT_NOTE test" "no 2×NOTE binary found / no readelf"
fi

# ---------------------------------------------------------------------------
#  9. self-propagation — running an infected binary infects the folder
#     The usage example shows a freshly recompiled binary gaining the
#     signature after an infected binary is EXECUTED, with no ./Famine in
#     between. That is the self-duplication the subject describes.
#
#     Attribution is the whole game here: the carrier is infected by ./Famine
#     BEFORE the victim exists, the victim is dropped in AFTER, and then only
#     the carrier is run — never ./Famine. So any signature the victim gains
#     can only have come from executing the carrier.
#
#     Expect RED until the stub carries the propagation payload — that red IS
#     the progress signal for the feature.
# ---------------------------------------------------------------------------
section "Self-propagation (infected binary infects on execution)"

if [ "$HAVE_SAMPLES" != 1 ]; then
	skip "self-propagation" "no compiler fixtures"
else
	# 1. stage a carrier and let ./Famine infect it — victim does NOT exist yet
	cc "$WORK/sample.c" -o "$T2/prop_carrier" >/dev/null 2>&1
	"$FAMINE" >/dev/null 2>&1
	if [ "$(sigcount "$T2/prop_carrier")" != "1" ]; then
		skip "self-propagation" "carrier didn't get infected — fix base infection first"
	else
		# 2. NOW drop a fresh, unseen victim (./Famine never touched it)
		cc "$WORK/sample.c" -o "$T1/prop_victim" >/dev/null 2>&1
		VIC_OUT=$("$T1/prop_victim")
		VIC_MD5=$(md5 "$T1/prop_victim")

		# 3. run ONLY the carrier. No ./Famine. Any infection now is the payload's.
		#    Exact-match on output doubles as a payload-silence check: if the
		#    stub printed anything while propagating, this would not equal the
		#    program's own single line.
		eq "carrier still runs silently/normally" "Hello, World!" "$("$T2/prop_carrier" 2>/dev/null)"

		# 4. THE key assertion — did executing the carrier infect the victim?
		if [ "$(sigcount "$T1/prop_victim")" = "1" ] && [ "$(md5 "$T1/prop_victim")" != "$VIC_MD5" ]; then
			pass "running an infected binary infected a fresh target"
		else
			fail "victim not infected by running the carrier (propagation not implemented yet)"
		fi
		eq "propagated victim still runs identically" "$VIC_OUT" "$("$T1/prop_victim" 2>/dev/null)"

		# 5. double-signature under propagation: run the carrier again; an
		#    already-signed victim must stay byte-identical (guard holds in the
		#    payload too, not just in ./Famine).
		VIC_MD5_2=$(md5 "$T1/prop_victim")
		"$T2/prop_carrier" >/dev/null 2>&1
		if [ "$(md5 "$T1/prop_victim")" = "$VIC_MD5_2" ]; then
			pass "re-running carrier does not re-infect victim (single signature)"
		else
			fail "victim re-infected on second carrier run (guard missing from payload)"
		fi
		eq "victim signature count still exactly once" "1" "$(sigcount "$T1/prop_victim")"

		# 6. second generation — an infected victim should itself propagate.
		#    Only meaningful if generation 1 actually happened.
		if [ "$(sigcount "$T1/prop_victim")" = "1" ] && [ "$(md5 "$T1/prop_victim")" != "$VIC_MD5" ]; then
			cc "$WORK/sample.c" -o "$T2/prop_gen2" >/dev/null 2>&1
			G2_MD5=$(md5 "$T2/prop_gen2")
			"$T1/prop_victim" >/dev/null 2>&1
			if [ "$(sigcount "$T2/prop_gen2")" = "1" ] && [ "$(md5 "$T2/prop_gen2")" != "$G2_MD5" ]; then
				pass "second-generation propagation (infected victim infects another)"
			else
				fail "no second-generation propagation"
			fi
		else
			skip "second-generation propagation" "generation 1 didn't propagate"
		fi
	fi
fi

# ---------------------------------------------------------------------------
# 10. robustness — must never crash and never speak, even on bad input
#     ("no output ... even in case of a crash"), including the offset/size
#     bounds-checking from the README hardening TODO.
# ---------------------------------------------------------------------------
section "Robustness (never crash, never speak)"

# A. a missing target dir must not fault the walker
rm -rf "$T1"
"$FAMINE" >"$WORK/outr" 2>"$WORK/errr"; RCR=$?
eq "missing target dir: exit 0" "0" "$RCR"
[ "$RCR" -ge 128 ] && fail "crashed on missing target dir (signal $((RCR-128)))"
eq "missing target dir: silent" "0" "$(( $(wc -c < "$WORK/outr") + $(wc -c < "$WORK/errr") ))"
mkdir -p "$T1"

# B. a valid-looking ELF with a bogus e_phnum must not fault the parser.
#    Maps to the README hardening TODO; expect RED until offsets are range-checked.
if [ "$HAVE_SAMPLES" = 1 ]; then
	cc "$WORK/sample.c" -o "$T2/badhdr" >/dev/null 2>&1
	printf '\377\377' | dd of="$T2/badhdr" bs=1 seek=56 count=2 conv=notrunc >/dev/null 2>&1
	"$FAMINE" >"$WORK/outb" 2>"$WORK/errb"; RCB=$?
	if [ "$RCB" -ge 128 ]; then fail "crashed on bogus e_phnum (signal $((RCB-128))) — needs offset/size bounds checks"
	else pass "survived bogus e_phnum without crashing"; fi
	eq "bogus-header run silent" "0" "$(( $(wc -c < "$WORK/outb") + $(wc -c < "$WORK/errb") ))"
else
	skip "malformed-header robustness" "no compiler fixtures"
fi

# ---------------------------------------------------------------------------
# 11. memory management — valgrind over a CLEAN, valid fixture set.
#     Measured on the normal path (happy path + the post-mmap reject path),
#     so the known e_phnum OOB (section 10) doesn't pollute the report.
#     Covers leaks, invalid read/write, double-free/UAF (ERROR SUMMARY) and
#     leaked file descriptors.
# ---------------------------------------------------------------------------
section "Memory management (valgrind)"
if ! command -v valgrind >/dev/null 2>&1; then
	skip "valgrind leak/error check" "valgrind not installed (apt-get install valgrind)"
elif [ "$HAVE_SAMPLES" != 1 ]; then
	skip "valgrind leak/error check" "no compiler fixtures"
else
	# clean fixtures: valid binaries (happy path) + a non-ELF and, if available,
	# a 32-bit ELF — both rejected AFTER mmap, exercising the munmap cleanup path.
	rm -rf "$T1" "$T2"; mkdir -p "$T1" "$T2"
	cc "$WORK/sample.c"         -o "$T1/vg_pie"   >/dev/null 2>&1
	cc -no-pie "$WORK/sample.c" -o "$T1/vg_nopie" >/dev/null 2>&1
	cp "$(command -v ls)"  "$T2/vg_ls"
	cp "$(command -v cat)" "$T2/vg_cat" 2>/dev/null
	dd if=/dev/zero of="$T1/vg_notelf" bs=256 count=1 >/dev/null 2>&1   # >64B, not an ELF
	[ "$HAVE_32" = 1 ] && cc -m32 "$WORK/sample.c" -o "$T1/vg_32" >/dev/null 2>&1

	VGLOG="$WORK/valgrind.log"
	valgrind --leak-check=full --show-leak-kinds=all \
	         --errors-for-leak-kinds=definite,indirect \
	         --track-fds=yes --error-exitcode=42 \
	         --log-file="$VGLOG" \
	         "$FAMINE" >/dev/null 2>&1

	# valgrind prints "ERROR SUMMARY:" only when it ran the program to
	# completion. If it bailed at startup (e.g. it can't set up ld.so
	# redirections because glibc/ld.so debug symbols are missing) there is no
	# summary — and the figures below would default to 0 and FALSE-PASS. So
	# gate every valgrind assertion on proof that valgrind actually ran.
	if ! grep -q "ERROR SUMMARY:" "$VGLOG"; then
		reason=$(grep -m1 -i 'cannot continue\|cannot be set up\|not found whilst' "$VGLOG" \
		         | sed 's/^valgrind:[[:space:]]*//')
		skip "valgrind leak/error check" "valgrind did not run — ${reason:-see $VGLOG}"
	else
		def=$(awk '/definitely lost:/ { n=$4; gsub(/,/,"",n); print n+0; exit }' "$VGLOG"); def=${def:-0}
		ind=$(awk '/indirectly lost:/ { n=$4; gsub(/,/,"",n); print n+0; exit }' "$VGLOG"); ind=${ind:-0}
		errs=$(awk '/ERROR SUMMARY:/  { print $4+0; exit }' "$VGLOG"); errs=${errs:-0}
		fds=$(awk '/FILE DESCRIPTORS:/ { print $4+0; exit }' "$VGLOG")

		if [ "$def" -eq 0 ] && [ "$ind" -eq 0 ]; then
			pass "no memory leaked (definite + indirect = 0 bytes)"
		else fail "leak: $def bytes definite + $ind bytes indirect lost — see $VGLOG"; fi

		if [ "$errs" -eq 0 ]; then pass "no memory errors on valid input (ERROR SUMMARY: 0)"
		else fail "$errs valgrind error(s) on valid input — see $VGLOG"; fi

		if [ -n "$fds" ]; then
			if [ "$fds" -le 3 ]; then pass "no leaked file descriptors ($fds open at exit)"
			else fail "$fds fds open at exit (expected 3: stdin/out/err) — see $VGLOG"; fi
		fi

		# guard against a false clean: confirm it actually infected under valgrind
		if [ "$(sigcount "$T1/vg_pie")" = "1" ]; then pass "infection confirmed under valgrind"
		else fail "valgrind ran but vg_pie wasn't infected — investigate"; fi
	fi
fi

# ---------------------------------------------------------------------------
#  summary
# ---------------------------------------------------------------------------
printf "\n${B}────────────────────────────────────────${N}\n"
printf "${B}Summary:${N} ${G}%d passed${N}, ${R}%d failed${N}, ${Y}%d skipped${N}\n" "$PASS" "$FAIL" "$SKIP"
printf "${D}Artifacts left in %s, %s, %s for inspection.${N}\n" "$T1" "$T2" "$WORK"
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
