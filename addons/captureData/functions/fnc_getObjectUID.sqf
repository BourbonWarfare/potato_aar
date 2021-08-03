#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Returns a given object's UID for tracking purposes
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_getObjectUID
 */
params ["_object"];
if (isPlayer _object) then { 
    getPlayerUID _object 
} else {
    _object call BIS_fnc_netId;
}
