#include "script_component.hpp"

GVAR(projectileID) = 0;
GVAR(trackingProjectiles) = [];

["CAManBase", "fired", LINKFUNC(fired)] call CBA_fnc_addClassEventHandler;
[LINKFUNC(trackProjectiles), 0.1] call CBA_fnc_addPerFrameHandler;



