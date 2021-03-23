#ifdef COMPILING_EXTENSION
    #define POTATO_AAR_SYMBOL __declspec(dllexport)
#else
    #define POTATO_AAR_SYMBOL __declspec(dllimport)
#endif