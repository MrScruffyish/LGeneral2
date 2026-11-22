#!/bin/sh
# Verify that the compiled gettext catalogs are found and used at runtime.
#
# Links a tiny program against the game's libutil.a and checks that trd()
# returns the German translation for a known msgid from the "pg" domain.
# Must be run from the game's data directory (the catalogs are staged under
# share/locale relative to it in uninstalled builds).
#
# Usage: tests/test-translations.sh [lgeneral-build-dir]

set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build=${1:-$root/.build/lgeneral}
data="$build/src"
util_src="$root/lgeneral/util"

for dom in lgeneral pg; do
    if [ ! -f "$data/share/locale/de/LC_MESSAGES/$dom.mo" ]; then
        echo "translations: missing staged German $dom catalog" >&2
        exit 1
    fi
done

tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-i18n.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/test-i18n.c" <<'EOF'
#include <stdio.h>
#include <string.h>
#include "localize.h"

static int check(const char *domain, const char *msgid, const char *expected)
{
    const char *s = trd(domain, msgid);
    printf("%s translation: %s\n", domain, s ? s : "(null)");
    return s && strcmp(s, expected) == 0;
}

int main(void)
{
    int ok = 1;
    locale_init(0);
    locale_load_domain("lgeneral", 0);
    locale_load_domain("pg", 0);
    /* Catalogs are ISO-8859-1: the acute accent is the single byte 0xe9. */
    ok &= check("lgeneral", "Out of memory\n", "Kein Speicher\n");
    ok &= check("pg", "Conde-sur-Noireau", "Cond\xe9-sur-Noireau");
    return ok ? 0 : 1;
}
EOF

# libutil.a contains C++ objects, so link the probe with the C++ compiler.
cxx=${CXX:-c++}
"$cxx" -DHAVE_CONFIG_H -I"$build" -I"$build/util" -I"$util_src" \
    "$tmp/test-i18n.c" "$build/util/libutil.a" -lm -o "$tmp/test-i18n"

(cd "$data" && LANGUAGE=de "$tmp/test-i18n")
echo "translations: OK"
