#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Called when a unit shoots. Has to be within range of Headless Client module to register shot, but POTATO does that already.
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_fired
 */
params ["", "", "", "", "_ammo", "", "_projectile"];

private _eventData = [EVENT_FIRED, CBA_missionTime, [GVAR(projectileID), getPosASLVisual _projectile, velocity _projectile, _ammo]];
GVAR(trackingProjectiles) pushBack [_projectile, GVAR(projectileID)];

GVAR(projectileID) = GVAR(projectileID) + 1;
private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, _eventData]];
TRACE_1("fired",_result);

