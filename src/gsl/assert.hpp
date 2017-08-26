#pragma once

namespace gsl {

inline void expects(bool condition)
{
    if(!condition) std::terminate();
}

inline void ensures(bool condition)
{
    if(!condition) std::terminate();
}

} // namespace gsl

#ifndef Expects
#define Expects(cond) gsl::expects(cond)
#endif

#ifndef Ensures
#define Ensures(cond) gsl::ensures(cond)
#endif
