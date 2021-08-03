#include "script_component.hpp"

class CfgPatches {
    class ADDON {
        units[] = {};
        weapons[] = {};
        requiredVersion = REQUIRED_VERSION;
        requiredAddons[] = {"ACE_COMMON"};
        author = "Bourbon Warfare";
        authorUrl = "https://github.com/BourbonWarfare/potato_aar"; // todo: change
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
        action = "https://github.com/BourbonWarfare/potato_aar"; // todo: change
        description = "";
    };
};