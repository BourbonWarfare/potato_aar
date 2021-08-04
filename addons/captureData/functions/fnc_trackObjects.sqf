#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Called per frame. Iterates through all objects and updates their state for the server
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_trackObjects
 */

private _packets = [];
{
    private _objectInfo = [
        getPosASLVisual _x,
        [_x, currentWeapon _x] call CBA_fnc_viewDir
    ];

    _packets pushBack [[_x] call FUNC(getObjectUID), _objectInfo];
} forEach GVAR(trackingObjects);

if (_packets isNotEqualTo []) then {
    private _result = "potato_aar_extension" callExtension ["processData", [UPDATE_OBJECT, nil, _packets]];
};