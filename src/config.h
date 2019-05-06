#ifndef WHATWG_CONFIG_H
#define WHATWG_CONFIG_H

// https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
#if defined(_MSVC_LANG) && (_MSVC_LANG >= 201703)
#define WHATWG__CPP_17
#elif  (__cplusplus >= 201703)
#define WHATWG__CPP_17
#endif

#endif // WHATWG_CONFIG_H
