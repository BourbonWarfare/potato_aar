#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Called when a unit is killed
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_objectKilled
 */
params ["_object"];

private _eventData = [
    EVENT_OBJECT_KILLED,
    CBA_missionTime,
    [
        [_object] call FUNC(getObjectUID)
    ]
];

GVAR(trackingObjects) = GVAR(trackingObjects) select {
    alive _x
};

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, _eventData]];
TRACE_1("object killed",_result);

