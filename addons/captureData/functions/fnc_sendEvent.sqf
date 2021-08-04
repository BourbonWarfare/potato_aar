#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Send an arbitrary event to the AAR program
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_sendEvent
 */
params ["_information"];

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_CUSTOM, CBA_missionTime, _information]]];

