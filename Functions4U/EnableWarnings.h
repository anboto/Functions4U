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
#endif

#endif
