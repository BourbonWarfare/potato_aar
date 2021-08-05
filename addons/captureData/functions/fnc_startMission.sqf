#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Send a packet with information about the mission start
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_startMission
 */
params ["_information"];

private _missionInfo = [
    worldName,
    missionName,
    OBJECT_UPDATE_RATE,
    PROJECTILE_UPDATE_RATE
];

private _startResult = "potato_aar_extension" callExtension "startup";
private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_MISSION_START, CBA_missionTime, [_missionInfo, _information]]]];
TRACE_2("started mission",_startResult,_result);

