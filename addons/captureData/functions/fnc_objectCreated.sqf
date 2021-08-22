#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Called when a unit is spawned
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_objectCreated
 */
params ["_object"];

private _name = "";

// add event handlers for getting in/out of vehicles
if (_object isKindOf "CAManBase") then {
    _object addEventHandler ["GetInMan", LINKFUNC(getIn)];
    _object addEventHandler ["GetOutMan", LINKFUNC(getOut)];
    _name = name _object;
};

private _type = "NONE";
if (_object isKindOf "CAManBase") then {
    _type = "MAN";
};

if (_object isKindOf "Tank") then {
    _type = "TANK";
};

if (_object isKindOf "Car") then {
    _type = "CAR";
};

if (_object isKindOf "Wheeled_APC_F") then {
    _type = "IFV";
};

if (_object isKindOf "Helicopter") then {
    _type = "HELICOPTER";
};

if (_object isKindOf "Plane") then {
    _type = "PLANE";
};

private _eventData = [
    EVENT_OBJECT_CREATED,
    CBA_missionTime,
    [
        [_object] call FUNC(getObjectUID),
        typeOf _object,
        getPosASLVisual _object,
        _name,
        isPlayer _object,
        _type
    ]
];

GVAR(trackingObjects) pushBack _object;

private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, _eventData]];
TRACE_1("object created",_result);

