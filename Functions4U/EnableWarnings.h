#ifndef _Functions4U_EnableWarnings_h_
#define _Functions4U_EnableWarnings_h_

#ifdef _MSC_VER
    //#pragma warning(push)
    //#pragma warning(disable : 4101 4996)
#elif __clang__
    #pragma clang diagnostic warning "-Wall"
    #pragma clang diagnostic warning "-Wextra" 
    #pragma clang diagnostic warning "-Wunused-parameter" 
    #pragma clang diagnostic warning "-Wlogical-op-parentheses" 
    #pragma clang diagnostic warning "-Wdeprecated-copy-with-user-provided-copy" 
    #pragma clang diagnostic warning "-Woverloaded-virtual" 
    #pragma clang diagnostic warning "-Wmissing-braces" 
    #pragma clang diagnostic warning "-Wshadow"  
    #pragma clang diagnostic warning "-Wimplicit-fallthrough"
    #pragma clang diagnostic warning "-Wsign-conversion"
    #pragma clang diagnostic warning "-Wformat=2"
    //#pragma clang diagnostic warning "-Werror=implicit"
	//#pragma clang diagnostic warning "-Werror=incompatible-pointer-types"
	//#pragma clang diagnostic warning "-Werror=int-conversion"
#endif

#endif
