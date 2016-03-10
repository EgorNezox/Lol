/**
  ******************************************************************************
  * @file    qmflags.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.02.2016
  *
  ******************************************************************************
 */

#ifndef QMFLAGS_H_
#define QMFLAGS_H_

#include <type_traits>
#include <limits>
#include <initializer_list>

class QmFlag
{
    int i;
public:
    constexpr inline QmFlag(int ai) : i(ai) {}
    constexpr inline operator int() const { return i; }

#  if !defined(__LP64__)
    constexpr inline QmFlag(long ai) : i(int(ai)) {}
    constexpr inline QmFlag(unsigned long ai) : i(int(long(ai))) {}
#  endif
    constexpr inline QmFlag(unsigned int ai) : i(int(ai)) {}
    constexpr inline QmFlag(short ai) : i(int(ai)) {}
    constexpr inline QmFlag(unsigned short ai) : i(int((unsigned int)(ai))) {}
    constexpr inline operator unsigned int() const { return (unsigned int)(i); }
};

class QmIncompatibleFlag
{
    int i;
public:
    constexpr inline explicit QmIncompatibleFlag(int i);
    constexpr inline operator int() const { return i; }
};

constexpr inline QmIncompatibleFlag::QmIncompatibleFlag(int ai) : i(ai) {}


template<typename Enum>
class QmFlags
{
    static_assert((sizeof(Enum) <= sizeof(int)),
                      "QmFlags uses an int as storage, so an enum with underlying "
                      "long long will overflow.");
    struct Private;
    typedef int (Private::*Zero);
public:
    typedef typename std::conditional<
    		!std::numeric_limits<Enum>::is_signed,
            unsigned int,
            signed int
        >::type Int;
    typedef Enum enum_type;
    constexpr inline QmFlags(Enum f) : i(Int(f)) {}
    constexpr inline QmFlags(Zero = 0) : i(0) {}
    constexpr inline QmFlags(QmFlag f) : i(f) {}
    constexpr inline QmFlags(std::initializer_list<Enum> flags)
        : i(initializer_list_helper(flags.begin(), flags.end())) {}

    constexpr inline QmFlags &operator&=(int mask) { i &= mask; return *this; }
    constexpr inline QmFlags &operator&=(unsigned int mask) { i &= mask; return *this; }
    constexpr inline QmFlags &operator&=(Enum mask) { i &= Int(mask); return *this; }
    constexpr inline QmFlags &operator|=(QmFlags f) { i |= f.i; return *this; }
    constexpr inline QmFlags &operator|=(Enum f) { i |= Int(f); return *this; }
    constexpr inline QmFlags &operator^=(QmFlags f) { i ^= f.i; return *this; }
    constexpr inline QmFlags &operator^=(Enum f) { i ^= Int(f); return *this; }

    constexpr inline operator Int() const { return i; }

    constexpr inline QmFlags operator|(QmFlags f) const { return QmFlags(QmFlag(i | f.i)); }
    constexpr inline QmFlags operator|(Enum f) const { return QmFlags(QmFlag(i | Int(f))); }
    constexpr inline QmFlags operator^(QmFlags f) const { return QmFlags(QmFlag(i ^ f.i)); }
    constexpr inline QmFlags operator^(Enum f) const { return QmFlags(QmFlag(i ^ Int(f))); }
    constexpr inline QmFlags operator&(int mask) const { return QmFlags(QmFlag(i & mask)); }
    constexpr inline QmFlags operator&(unsigned int mask) const { return QmFlags(QmFlag(i & mask)); }
    constexpr inline QmFlags operator&(Enum f) const { return QmFlags(QmFlag(i & Int(f))); }
    constexpr inline QmFlags operator~() const { return QmFlags(QmFlag(~i)); }

    constexpr inline bool operator!() const { return !i; }

    constexpr inline bool testFlag(Enum f) const { return (i & Int(f)) == Int(f) && (Int(f) != 0 || i == Int(f) ); }
private:
    constexpr static inline Int initializer_list_helper(typename std::initializer_list<Enum>::const_iterator it,
                                                               typename std::initializer_list<Enum>::const_iterator end)
    {
        return (it == end ? Int(0) : (Int(*it) | initializer_list_helper(it + 1, end)));
    }

    Int i;
};

#define QM_DECLARE_FLAGS(Flags, Enum) \
		typedef QmFlags<Enum> Flags;

#define QM_DECLARE_INCOMPATIBLE_FLAGS(Flags) \
		constexpr inline QmIncompatibleFlag operator|(Flags::enum_type f1, int f2) \
		{ return QmIncompatibleFlag(int(f1) | f2); }

#define QM_DECLARE_OPERATORS_FOR_FLAGS(Flags) \
		constexpr inline QmFlags<Flags::enum_type> operator|(Flags::enum_type f1, Flags::enum_type f2) \
		{ return QmFlags<Flags::enum_type>(f1) | f2; } \
		constexpr inline QmFlags<Flags::enum_type> operator|(Flags::enum_type f1, QmFlags<Flags::enum_type> f2) \
		{ return f2 | f1; } \
		QM_DECLARE_INCOMPATIBLE_FLAGS(Flags)

#endif /* QMFLAGS_H_ */
