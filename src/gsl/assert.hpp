#pragma once
#include<iostream>

namespace gsl {

inline void expects(bool condition, const char* file, int line, const char* func)
{
    if(condition) return;
    std::cerr << file << " " << line << " " << func << std::endl;
    std::terminate();
}

inline void ensures(bool condition, const char* file, int line, const char* func)
{
    if(condition) return;
    std::cerr << file << " " << line << " " << func << std::endl;
    std::terminate();
}

} // namespace gsl

#ifndef __OPTIMIZE__

#ifndef Expects
#define Expects(condition) gsl::expects(condition, __FILE__, __LINE__, __func__)
#endif

#ifndef Ensures
#define Ensures(condition) gsl::ensures(condition, __FILE__, __LINE__, __func__)
#endif

#else
#define Expects(condition)
#define Ensures(condition)
#endif
