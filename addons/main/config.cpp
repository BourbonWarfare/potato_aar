#include "script_component.hpp"

class CfgPatches {
    class ADDON {
        units[] = {};
        weapons[] = {};
        requiredVersion = REQUIRED_VERSION;
        requiredAddons[] = {"ACE_COMMON"};
        author = "Bourbon Warfare";
        authorUrl = "https://github.com/BourbonWarfare/POTATO"; // todo: change
        VERSION_CONFIG;
    };
};

class CfgMods {
    class PREFIX {
        dir = "@POTATO_AAR";
        name = "POTATO AAR";
        picture = "";
        hidePicture = "false";
        hideName = "false";
        actionName = "Website";
        action = "https://github.com/BourbonWarfare/POTATO"; // todo: change
        description = "";
    };
};