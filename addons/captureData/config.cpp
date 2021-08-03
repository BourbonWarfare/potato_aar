#include "script_component.hpp"

class CfgPatches {
    class ADDON {
        units[] = {};
        weapons[] = {};
        requiredVersion = REQUIRED_VERSION;
        requiredAddons[] = {"potato_aar_main"};
        author = "Bourbon Warfare";
        authorUrl = "https://github.com/BourbonWarfare/potato_aar";
        VERSION_CONFIG;
    };
};

#include "CfgEventHandlers.hpp"

