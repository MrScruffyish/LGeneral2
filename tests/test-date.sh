#!/bin/sh
# Verify the local date parser after its std::string conversion.
#
# Builds a tiny probe against lgeneral/src/date.cpp. This checks the parser's
# public behaviour and the fixed char* output boundary used by callers.
#
# Usage: tests/test-date.sh [source-root]

set -eu

root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lgeneral-date.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat > "$tmp/test-date.cpp" <<'EOF'
#include "date.h"

#include <cstdio>
#include <cstring>

static int failures = 0;

static void fail( const char *what )
{
    std::fprintf( stderr, "date: %s\n", what );
    failures++;
}

static void check_parse( const char *text, int day, int month, int year )
{
    char input[64];
    std::snprintf( input, sizeof input, "%s", text );

    Date date{};
    str_to_date( &date, input );
    if ( date.day != day || date.month != month || date.year != year ) {
        std::fprintf( stderr, "date: '%s' parsed as %d.%d.%d\n",
                      text, date.day, date.month, date.year );
        failures++;
    }
}

int main()
{
    check_parse( "12.3.1941", 12, 3, 1941 );
    check_parse( "1.2.39", 1, 2, 39 );
    check_parse( "7.4", 7, 0, 0 );

    Date empty{};
    str_to_date( &empty, nullptr );
    if ( empty.day || empty.month || empty.year )
        fail( "null input did not clear the date" );

    char unused[] = "1.1.1941";
    str_to_date( nullptr, unused );

    Date date{ 12, 3, 1941 };
    char out[64];
    date_to_str( out, date, DIGIT_DATE );
    if ( std::strcmp( out, "12.3.1941" ) != 0 )
        fail( "digit date output changed" );

    if ( failures )
        return 1;
    std::puts( "date: OK" );
    return 0;
}
EOF

cxx=${CXX:-c++}
"$cxx" -std=c++17 -Wall \
    -I"$root/lgeneral/src" -I"$root/lgeneral/util" -I"$root/lgeneral" \
    "$tmp/test-date.cpp" "$root/lgeneral/src/date.cpp" -o "$tmp/test-date"

"$tmp/test-date"
