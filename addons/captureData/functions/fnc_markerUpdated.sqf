#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Updates marker information
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_markerUpdated
 */
params ["_marker"];

private _markerInfo = [
    _marker,
    markerAlpha _marker,
    markerBrush _marker,
    markerChannel _marker,
    markerColor _marker,
    markerDir _marker,
    markerPos _marker,
    markerShape _marker,
    markerSize _marker,
    markerText _marker,
    markerType _marker
];

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_MARKER_UPDATED, CBA_missionTime, _markerInfo]]];
