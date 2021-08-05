#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Destroys a marker
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_markerDestroyed
 */
params ["_marker"];

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_MARKER_DESTROYED, CBA_missionTime, [_marker]]]];
TRACE_1("marker deleted",_result);

