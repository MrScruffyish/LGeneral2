#!/usr/bin/env python3
"""Add LG_TRACE logging calls to a copied LGeneral source tree.

Only used by the top-level Makefile with TRACE=1; the real sources in
lgeneral/ are never modified. Each insertion is guarded so running the
script twice on the same tree is harmless.
"""

import sys
from pathlib import Path

MARKER = "[trace]"


def ensure_include(path):
    """Make lgtrace() visible without polluting configure's feature tests."""
    text = path.read_text()
    if '#include "lgtrace.h"' not in text:
        path.write_text('#include "lgtrace.h"\n' + text)


def insert_after(path, anchor, additions):
    text = path.read_text()
    if any(MARKER in line for line in additions) and MARKER in text:
        # Already instrumented (check per file below for finer control).
        if any(line.strip() in text for line in additions if MARKER in line):
            return
    lines = text.splitlines()
    matches = [i for i, line in enumerate(lines) if line.strip() == anchor.strip()]
    if len(matches) != 1:
        raise SystemExit(f"{path}: expected one {anchor!r}, found {len(matches)}")
    i = matches[0]
    indent = lines[i][: len(lines[i]) - len(lines[i].lstrip())]
    block = [indent + line.strip() if line.strip() else "" for line in additions]
    lines[i + 1 : i + 1] = block
    path.write_text("\n".join(lines) + "\n")


def replace_once(path, old, new):
    text = path.read_text()
    needle = old.strip("\n")
    replacement = new.strip("\n")
    if replacement in text:
        return
    count = text.count(needle)
    if count != 1:
        raise SystemExit(f"{path}: expected one anchor block, found {count}")
    path.write_text(text.replace(needle, new.strip("\n")))


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: instrument.py <lgeneral-build-dir>")
    root = Path(sys.argv[1]) / "src"

    for name in ("config.cpp", "scenario.cpp", "engine.cpp", "gui.cpp", "player.cpp"):
        ensure_include(root / name)

    config = root / "config.cpp"
    insert_after(
        config,
        "reset_config();",
        ['lgtrace("[trace] config dir=%s defaults purchase=%d deploy=%d\\n",',
         '        config.dir_name, config.purchase, config.deploy_turn);'],
    )
    insert_after(
        config,
        "parser_free( &pd );",
        ['lgtrace("[trace] config loaded purchase=%d deploy=%d\\n",',
         '        config.purchase, config.deploy_turn);'],
    )

    scenario = root / "scenario.cpp"
    insert_after(
        scenario,
        "char log_str[256];",
        ['lgtrace("[trace] scen_load start %s\\n", fname);'],
    )
    insert_after(
        scenario,
        "deploy_turn = config.deploy_turn;",
        ['lgtrace("[trace] scenario file=%s name=%s domain=%s deploy_turn=%d purchase=%d units=%d reinf=%d avail=%d\\n",',
         '        scen_info->fname, scen_info->name, scen_domain ? scen_domain : "",',
         '        deploy_turn, config.purchase,',
         '        units ? units->count : -1, reinf ? reinf->count : -1,',
         '        avail_units ? avail_units->count : -1);'],
    )
    insert_after(
        scenario,
        "update_nations_purchase_flag();",
        ['{ int pi;',
         '  for ( pi = 0; players && pi < players->count; pi++ ) {',
         '      Player *pp = (Player *)list_get( players, pi );',
         '      lgtrace("[trace] scenario player[%d] id=%s ctrl=%d no_init_deploy=%d\\n",',
         '              pi, pp->id ? pp->id : "?", pp->ctrl, pp->no_init_deploy);',
         '  } }'],
    )

    map_cpp = root / "map.cpp"
    replace_once(
        map_cpp,
        """
void map_remove_unit( Unit *unit )
{
    if ( unit->sel_prop->flags & FLYING )
        map_tile( unit->x, unit->y )->a_unit = 0;
    else
        map_tile( unit->x, unit->y )->g_unit = 0;
}
""",
        """
void map_remove_unit( Unit *unit )
{
    if ( unit->sel_prop->flags & FLYING )
        map_tile( unit->x, unit->y )->a_unit = 0;
    else
        map_tile( unit->x, unit->y )->g_unit = 0;
}

int lgtrace_count_map_refs( Unit *unit )
{
    int x, y, count = 0;
    for ( x = 0; x < map_w; x++ )
        for ( y = 0; y < map_h; y++ ) {
            Map_Tile *tile = map_tile( x, y );
            if ( tile->g_unit == unit || tile->a_unit == unit )
                count++;
        }
    return count;
}
""",
    )

    engine = root / "engine.cpp"
    replace_once(
        engine,
        """
    list_reset( reinf );
    for ( i = 0; i < reinf->count; i++ ) {
        unit = (Unit*)list_next( reinf );
        if ( !(unit->sel_prop->flags & FLYING) && unit->player == cur_player && unit->delay <= turn ) {
            list_transfer( reinf, avail_units, unit );
            /* index must be reset if unit was added */
            i--;
        }
    }
}
""",
        """
    list_reset( reinf );
    for ( i = 0; i < reinf->count; i++ ) {
        unit = (Unit*)list_next( reinf );
        if ( !(unit->sel_prop->flags & FLYING) && unit->player == cur_player && unit->delay <= turn ) {
            list_transfer( reinf, avail_units, unit );
            /* index must be reset if unit was added */
            i--;
        }
    }
    lgtrace("[trace] reinf update turn=%d player=%s reinf=%d avail=%d\\n", turn,
            cur_player && cur_player->id ? cur_player->id : "?",
            reinf ? reinf->count : -1, avail_units ? avail_units->count : -1);
}
""",
    )
    replace_once(
        engine,
        """
        if ( deploy_turn
             && cur_player && cur_player->ctrl == PLAYER_CTRL_HUMAN )
            engine_handle_button( ID_DEPLOY );
""",
        """
        lgtrace("[trace] turn setup turn=%d deploy_turn=%d player=%s ctrl=%d units=%d reinf=%d avail=%d\\n",
                turn, deploy_turn, cur_player && cur_player->id ? cur_player->id : "?",
                cur_player ? cur_player->ctrl : -1,
                units ? units->count : -1, reinf ? reinf->count : -1,
                avail_units ? avail_units->count : -1);
        if ( deploy_turn
             && cur_player && cur_player->ctrl == PLAYER_CTRL_HUMAN )
            engine_handle_button( ID_DEPLOY );
""",
    )
    replace_once(
        engine,
        """
            while ( ( unit = (Unit*)list_next( units ) ) ) {
                if ( unit->player == cur_player )
                if ( mask[unit->x][unit->y].deploy )
                if ( unit_supports_deploy(unit) ) {
""",
        """
            while ( ( unit = (Unit*)list_next( units ) ) ) {
                if ( unit->player == cur_player )
                lgtrace("[trace] deploy candidate %s pos=%d,%d mask=%d supports=%d\\n",
                        unit->name, unit->x, unit->y,
                        mask[unit->x][unit->y].deploy, unit_supports_deploy(unit));
                if ( mask[unit->x][unit->y].deploy )
                if ( unit_supports_deploy(unit) ) {
""",
    )

    insert_after(
        engine,
        "old_unit = map_swap_unit( deploy_unit );",
        ['lgtrace("[trace] deploy place %s at=%d,%d old=%s left=%d avail=%d\\n",',
         '        deploy_unit ? deploy_unit->name : "(none)", mx, my,',
         '        old_unit ? old_unit->name : "(none)",',
         '        left_deploy_units ? left_deploy_units->count : -1,',
         '        avail_units ? avail_units->count : -1);'],
    )
    insert_after(
        engine,
        "engine_undeploy_unit( unit );",
        ['lgtrace("[trace] deploy undeploy %s at=%d,%d left=%d avail=%d\\n",',
         '        unit ? unit->name : "(none)", mx, my,',
         '        left_deploy_units ? left_deploy_units->count : -1,',
         '        avail_units ? avail_units->count : -1);'],
    )

    gui = root / "gui.cpp"
    insert_after(
        gui,
        "deploy_unit = (Unit*)list_get( avail_units, 0 );",
        ['lgtrace("[trace] deploy window avail=%d first=%s\\n",',
         '        avail_units ? avail_units->count : -1,',
         '        deploy_unit ? deploy_unit->name : "(none)");'],
    )
    replace_once(
        gui,
        """
    while ( ( unit = (Unit*)list_next( avail_units ) ) ) {
        if ( unit->x >= 0 && unit->y >= 0 )
            map_remove_unit( unit );
        unit->x = -1;
        unit->y = -1;
        list_add( left_deploy_units, unit );
    }
""",
        """
    while ( ( unit = (Unit*)list_next( avail_units ) ) ) {
        if ( unit->x >= 0 && unit->y >= 0 ) {
            lgtrace("[trace] deploy window clear %s from=%d,%d\\n",
                    unit->name, unit->x, unit->y);
            map_remove_unit( unit );
        }
        unit->x = -1;
        unit->y = -1;
        list_add( left_deploy_units, unit );
    }
    {
        extern int lgtrace_count_map_refs( Unit *unit );
        int stale = 0;
        Unit *stale_unit;
        list_reset( avail_units );
        while ( ( stale_unit = (Unit*)list_next( avail_units ) ) )
            stale += lgtrace_count_map_refs( stale_unit );
        lgtrace("[trace] deploy window stale map refs=%d\\n", stale);
    }
""",
    )

    player = root / "player.cpp"
    insert_after(
        player,
        "player = (Player*)list_get( players, cur_player_id );",
        ['lgtrace("[trace] next player id=%s ctrl=%d no_init_deploy=%d deploy_turn=%d\\n",',
         '        player->id ? player->id : "?", player->ctrl,',
         '        player->no_init_deploy, deploy_turn);'],
    )


if __name__ == "__main__":
    main()
