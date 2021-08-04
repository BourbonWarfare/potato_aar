#include "script_component.hpp"

GVAR(projectileID) = 0;
GVAR(trackingProjectiles) = [];

["CAManBase", "fired", LINKFUNC(fired)] call CBA_fnc_addClassEventHandler;
[LINKFUNC(trackProjectiles), 0.1] call CBA_fnc_addPerFrameHandler;

GVAR(trackingObjects) = [];
["AllVehicles", "init", LINKFUNC(objectCreated), true, [], true] call CBA_fnc_addClassEventHandler;
addMissionEventHandler ["EntityKilled", LINKFUNC(objectKilled)];
[LINKFUNC(trackObjects), 1] call CBA_fnc_addPerFrameHandler;

addMissionEventHandler ["MarkerCreated", LINKFUNC(markerCreated)];
addMissionEventHandler ["MarkerDeleted", LINKFUNC(markerDestroyed)];
addMissionEventHandler ["MarkerUpdated", LINKFUNC(markerUpdated)];

addMissionEventHandler ["Ended", LINKFUNC(endMission)];

[QGVAR(sendEvent), LINKFUNC(sendEvent)] call CBA_fnc_addEventHandler;
[QGVAR(endMission), LINKFUNC(endMission)] call CBA_fnc_addEventHandler;

private _missionInfo = [
    worldName,
    missionName
];
private _result = "potato_aar_extension" callExtension ["processData", [GAME_EVENT, nil, [EVENT_MISSION_START, CBA_missionTime, _missionInfo]]];

