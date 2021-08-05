#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * CAlled by PFH. Sends a periodic "heartbeat" to the server observer to ensure that we are still alive. If we aren't alive, it will dump what it has
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_heartbeat
 */

private _result = "potato_aar_extension" callExtension ["processData", [HEARTBEAT, nil, [CBA_missionTime]]];
TRACE_1("heartbeat",_result);
