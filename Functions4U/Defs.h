#ifndef _Functions4U_Defs_h_
#define _Functions4U_Defs_h_

namespace Upp {

#ifdef PLATFORM_WIN32
inline bool IsNum(const double &n) 	{return !std::isnan<double>(n) && !std::isinf<double>(n) && !IsNull(n);}
inline bool IsNum(const float &n)	{return !std::isnan<float>(n) && !std::isinf<float>(n);}
#else
inline bool IsNum(const double &n) 	{return !__builtin_isnan(n) && !__builtin_isinf(n) && !IsNull(n);}
inline bool IsNum(const float &n) 	{return !__builtin_isnan(n) && !__builtin_isinf(n);}
#endif
inline bool IsNum(const int &n) 	{return !IsNull(n);}
template <typename T>									   // No idea about the reason of this: (n.real() == 0 && n.imag() == 0)
inline bool IsNum(const std::complex<T> &n) {return !(!IsNum(n.real()) || !IsNum(n.imag())/* || (n.real() == 0 && n.imag() == 0)*/);}

template <typename T>
bool IsNull(const std::complex<T> &d)	{return !IsNum(d);};

#define NaNComplex		std::complex<double>(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN())
#define NaNDouble		std::numeric_limits<double>::quiet_NaN()

template <typename T>
inline std::complex<T> i()	{return std::complex<T>(0, 1);};

template <typename T>
inline bool IsNum(const Point_<T> &n) {return IsNum(n.x) && IsNum(n.y);}


}

#endif
