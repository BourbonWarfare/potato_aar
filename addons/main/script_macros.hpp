//Add all ACE and CBA macros:
#include "\z\ace\addons\main\script_macros.hpp"

#define ACE_PREFIX ace

#define ACEGVAR(module,var) TRIPLES(ACE_PREFIX,module,var)
#define QACEGVAR(module,var) QUOTE(ACEGVAR(module,var))
#define ACEFUNC(var1,var2) TRIPLES(DOUBLES(ACE_PREFIX,var1),fnc,var2)
#define DACEFUNC(var1,var2) TRIPLES(DOUBLES(ACE_PREFIX,var1),fnc,var2)
#define QACEFUNC(var1,var2) QUOTE(DACEFUNC(var1,var2))

#define ACECSTRING(var1,var2) QUOTE(TRIPLES($STR,DOUBLES(ACE_PREFIX,var1),var2))
#define ACELSTRING(var1,var2) QUOTE(TRIPLES(STR,DOUBLES(ACE_PREFIX,var1),var2))

#define CFUNC(var1) EFUNC(core,var1)

#define POTATO_PREFIX potato

#define POTATOGVAR(module,var) TRIPLES(POTATO_PREFIX,module,var)
#define QPOTATOGVAR(module,var) QUOTE(POTATOGVAR(module,var))
#define POTATOFUNC(var1,var2) TRIPLES(DOUBLES(POTATO_PREFIX,var1),fnc,var2)
#define DPOTATOFUNC(var1,var2) TRIPLES(DOUBLES(POTATO_PREFIX,var1),fnc,var2)
#define QPOTATOFUNC(var1,var2) QUOTE(DPOTATOFUNC(var1,var2))

#define POTATOCSTRING(var1,var2) QUOTE(TRIPLES($STR,DOUBLES(POTATO_PREFIX,var1),var2))
#define POTATOLSTRING(var1,var2) QUOTE(TRIPLES(STR,DOUBLES(POTATO_PREFIX,var1),var2))

#define CFUNC(var1) EFUNC(core,var1)
