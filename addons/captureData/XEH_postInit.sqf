#include "script_component.hpp"

[] call FUNC(startMission);

GVAR(projectileID) = 0;
GVAR(trackingProjectiles) = [];

["CAManBase", "fired", LINKFUNC(fired)] call CBA_fnc_addClassEventHandler;
[LINKFUNC(trackProjectiles), PROJECTILE_UPDATE_RATE] call CBA_fnc_addPerFrameHandler;

GVAR(trackingObjects) = [];
["AllVehicles", "init", LINKFUNC(objectCreated), true, ["Animal"], true] call CBA_fnc_addClassEventHandler;
addMissionEventHandler ["EntityKilled", LINKFUNC(objectKilled)];
[LINKFUNC(trackObjects), OBJECT_UPDATE_RATE] call CBA_fnc_addPerFrameHandler;

addMissionEventHandler ["MarkerCreated", LINKFUNC(markerCreated)];
addMissionEventHandler ["MarkerDeleted", LINKFUNC(markerDestroyed)];
addMissionEventHandler ["MarkerUpdated", LINKFUNC(markerUpdated)];

addMissionEventHandler ["Ended", LINKFUNC(endMission)];

[QGVAR(sendEvent), LINKFUNC(sendEvent)] call CBA_fnc_addEventHandler;
[QGVAR(endMission), LINKFUNC(endMission)] call CBA_fnc_addEventHandler;

