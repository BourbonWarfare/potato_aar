#include "script_component.hpp"
/*
 * Author: Brandon (TCVM)
 * Captures unit data periodically and sends it across network. Should be called within a CBA per frame handler
 *
 * Examples:
 * [] call potato_aar_captureData_fnc_captureUnitData
 */
params ["_args", "_handle"];
_args params ["_lastUpdate", "_unitsToParse", "_lastParseIndex", "_unitData"];

private _lastIndex = _lastParseIndex + UNITS_TO_CAPTURE_PER_FRAME;
private _overrun = _lastParseIndex > count _unitsToParse;

if !(_overrun) then {
	private _lastIncrement = (_lastIndex min count _unitsToParse) - 1;

	for "_i" from _lastParseIndex to _lastIncrement do {
		private _unit = _unitsToParse select _i;
		private _uid = if (isPlayer _unit) then { 
			getPlayerUID _unit 
		} else {
			_unit call BIS_fnc_netId;
		};

		/*
			[player1] : {
				[1] : {}
				[2] : {}
				[3] : {}
			}
			[player2] : {
				[1] : {}
				[2] : {}
				[3] : {}
			}
			[bullet] : {
				[10] : {}
			}

			[0] : {
				// bullet data
			}
			[10]

			{
				"bullet created" -> 0,
				"player created" -> 10
			}
		*/
		
		_unitData pushBack [_uid, getPosASL _unit, getDir _unit];
	};

	_args set [3, _unitData];
};

_args set [2, _lastIndex];
if (CBA_missionTime - _lastUpdate > CAPTURE_FREQUENCY) then {
	"potato_aar_extension" callExtension ["processData", _unitData];

	_args set [0, CBA_missionTime];
	_args set [1, allUnits];
	_args set [2, 0];
	_args set [3, [UNIT_STATE_UPDATE, 0]];
};

 