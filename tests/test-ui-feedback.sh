#!/bin/sh
# Source-level regression check for the reinforcement/deploy UI feedback.
#
# The visibility of the prestige value and the attention state of the two
# reinforcement buttons are easy to lose during later GUI edits. This probe
# checks that all pieces are still wired together; the full smoke run also
# builds and launches the game to catch compile/runtime regressions.

set -eu

root=${1:-$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)}
src="$root/lgeneral/src"

check() {
    file=$1
    text=$2
    if ! grep -qF -- "$text" "$src/$file"; then
        echo "test-ui-feedback: missing '$text' in $file" >&2
        exit 1
    fi
}

check windows.h 'int highlight;'
check windows.h 'void group_set_highlight( Group *group, int id, int value );'
check windows.h 'void group_refresh_button( Button *button, int focused );'
check windows.cpp 'void group_set_highlight( Group *group, int id, int value )'
check windows.cpp 'void group_refresh_button( Button *button, int focused )'
check engine.cpp 'group_refresh_button( last_button,'
check engine.cpp 'group_set_highlight( gui->base_menu, ID_PURCHASE,'
check engine.cpp 'group_set_highlight( gui->base_menu, ID_DEPLOY,'
check engine.cpp 'player_can_purchase_any_unit( cur_player )'
check purchase_dlg.h 'int player_can_purchase_any_unit( Player *p );'
check purchase_dlg.cpp 'int player_can_purchase_any_unit( Player *p )'
check purchase_dlg.cpp 'if (e->nation < 0 || e->nation >= nation_count ||'
check purchase_dlg.cpp 'tr("Prestige: %d")'

echo "test-ui-feedback: OK"
