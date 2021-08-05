#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Called by PFH. Reads all projectile data and sends update packets to AAR
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_trackProjectiles
 */

GVAR(trackingProjectiles) = GVAR(trackingProjectiles) select {
    alive (_x select 0);
};

private _packets = [];
{
    _x params ["_projectile", "_projectileUID"];

    private _projectileData = [
        CBA_missionTime,
        _projectileUID,
        getPosASLVisual _projectile,
        velocity _projectile
    ];

    _packets pushBack _projectileData;

} forEach GVAR(trackingProjectiles);

if (_packets isNotEqualTo []) then {
    private _result = "potato_aar_extension" callExtension ["processData", [UPDATE_PROJECTILE, nil, _packets]];
    TRACE_1("track projectiles",_result);
};

