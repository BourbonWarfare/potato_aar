#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Send a packet with information about the mission end
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_endMission
 */
params ["_information"];

[GVAR(heartbeatHandle)] call CBA_fnc_removePerFrameHandler;

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_MISSION_END, CBA_missionTime, _information]]];
private _endResult = "potato_aar_extension" callExtension "shutdown";

TRACE_2("end mission",_result,_endResult);

