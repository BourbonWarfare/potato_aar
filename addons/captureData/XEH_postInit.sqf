#include "script_component.hpp"

GVAR(projectileID) = 0;
GVAR(trackingProjectiles) = [];

["CAManBase", "fired", LINKFUNC(fired)] call CBA_fnc_addClassEventHandler;
[LINKFUNC(trackProjectiles), 0.1] call CBA_fnc_addPerFrameHandler;

GVAR(trackingObjects) = [];
["CAManBase", "init", LINKFUNC(objectCreated), true, [], true] call CBA_fnc_addClassEventHandler;
addMissionEventHandler ["EntityKilled", LINKFUNC(objectKilled)];
[LINKFUNC(trackObjects), 1] call CBA_fnc_addPerFrameHandler;

addMissionEventHandler ["MarkerCreated", LINKFUNC(markerCreated)];
addMissionEventHandler ["MarkerDeleted", LINKFUNC(markerDestroyed)];
addMissionEventHandler ["MarkerUpdated", LINKFUNC(markerUpdated)];

