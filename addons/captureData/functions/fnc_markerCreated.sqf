#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Creates a marker on the server
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_markerCreated
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

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_MARKER_CREATED, CBA_missionTime, _markerInfo]]];
TRACE_1("marker created",_result);

