#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Called when a unit gets out of a vehicle
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_getOut
 */
params ["_unit", "_role", "_vehicle"];

private _eventData = [
    EVENT_OBJECT_GET_OUT,
    CBA_missionTime,
    [
        [_unit] call FUNC(getObjectUID),
        [vehicle _vehicle] call FUNC(getObjectUID),
        _role
    ]
];

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, _eventData]];
TRACE_1("get out",_result);

