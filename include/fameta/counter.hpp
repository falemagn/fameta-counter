// Author: Fabio Alemagna <personal@fabioalemagna.net>
// Source: https://github.com/falemagn/fameta-counter
// Inspired to Filip Roséen's work. See https://stackoverflow.com/questions/60082260/c-compile-time-counters-revisited

#ifndef FAMETA_COUNTER_HPP
#define FAMETA_COUNTER_HPP

#if !defined(__cpp_if_constexpr) || __cpp_if_constexpr < 201606L
#   if defined(FAMETA_BINARY_LOOKUP) && FAMETA_BINARY_LOOKUP
#		error "Binary lookup is only available when compiling with c++17 and above"
#	endif
#
#	undef FAMETA_BINARY_LOOKUP
#	define FAMETA_BINARY_LOOKUP 0
#	define FAMETA_UNIQUE_VALUE_TYPE unsigned long long
#else
#   if !defined(FAMETA_BINARY_LOOKUP)
#       define FAMETA_BINARY_LOOKUP 1
#   endif
#	define FAMETA_UNIQUE_VALUE_TYPE auto
#endif

#if defined(__GNUC__) && !defined(__clang__)
// There appears to be a bug on gcc that makes it emit a diagnostic that cannot be turned off in certain conditions. This will silence it.
// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112267
#	define FAMETA_FRIEND_RETURN_TYPE auto
#else
#	define FAMETA_FRIEND_RETURN_TYPE bool
#endif

#if !defined(FAMETA_FRIEND_INJECTION_PRAGMA_BEGIN) && !defined(FAMETA_FRIEND_INJECTION_PRAGMA_END)
#	if defined(__INTEL_COMPILER)
#		define FAMETA_FRIEND_INJECTION_PRAGMA_BEGIN _Pragma("warning push"); _Pragma("warning disable 1624");
#		define FAMETA_FRIEND_INJECTION_PRAGMA_END   _Pragma("warning pop");
#	elif defined(__clang__)
#		define FAMETA_FRIEND_INJECTION_PRAGMA_BEGIN _Pragma("GCC diagnostic push"); _Pragma("GCC diagnostic ignored \"-Wundefined-internal\"");
#		define FAMETA_FRIEND_INJECTION_PRAGMA_END   _Pragma("GCC diagnostic pop");
#	elif defined(__GNUC__)
#		define FAMETA_FRIEND_INJECTION_PRAGMA_BEGIN _Pragma("GCC diagnostic push"); _Pragma("GCC diagnostic ignored \"-Wnon-template-friend\""); \
																					_Pragma("GCC diagnostic ignored \"-Wunused-function\"");
#		define FAMETA_FRIEND_INJECTION_PRAGMA_END   _Pragma("GCC diagnostic pop");
#	else
#		define FAMETA_FRIEND_INJECTION_PRAGMA_BEGIN
#		define FAMETA_FRIEND_INJECTION_PRAGMA_END
#	endif
#endif

namespace fameta
{

// Anonymous namespace to avoid ODR violation
namespace {

template <typename Context, int Start = 0, int Step = 1>
class counter
{
public:
	template <typename Unique>
	static constexpr int next()
	{
		return next<Unique>(0)*Step+Start;
	}

	template <FAMETA_UNIQUE_VALUE_TYPE>
	static constexpr int next()
	{
		struct Unique{};
		return next<Unique>(0)*Step+Start;
	}

	template <typename Unique>
	static constexpr int current()
	{
		return current<Unique>(0)*Step+Start;
	}

	template <FAMETA_UNIQUE_VALUE_TYPE>
	static constexpr int current()
	{
		struct Unique{};
		return current<Unique>(0)*Step+Start;
	}

private:
	template <int I>
	struct slot
	{
		FAMETA_FRIEND_INJECTION_PRAGMA_BEGIN

			friend constexpr FAMETA_FRIEND_RETURN_TYPE slot_allocated(slot<I>);

		FAMETA_FRIEND_INJECTION_PRAGMA_END
	};

	template <int I>
	struct allocate_slot {
		friend constexpr FAMETA_FRIEND_RETURN_TYPE slot_allocated(slot<I>) {
			return true;
		}

		enum { value = I };
	};

#if FAMETA_BINARY_LOOKUP
	// If slot_allocated(slot<I>) has NOT been defined, then SFINAE will keep this function out of the overload set...
	template <typename Unique, int I = 0, int P = 0, bool = slot_allocated(slot<I + (1<<P)-1>())>
	static constexpr int next(int)
	{
		return next<Unique, I, P+1>(0);
	}

	// ...And this function will be used, instead, which will define slot_allocated(slot<I>) via allocate_slot<I>.
	template <typename Unique, int I = 0, int P = 0>
	static constexpr int next(double)
	{
		if constexpr (P == 0)
			return allocate_slot<I>::value;
		else
			return next<Unique, I+(1<<(P-1)), 0>(0);
	}

	// If slot_allocated(slot<I>) has NOT been defined, then SFINAE will keep this function out of the overload set...
	template <typename Unique, int I = 0, int P = 0, bool = slot_allocated(slot<I + (1<<P)-1>())>
	static constexpr int current(int)
	{
		return current<Unique, I, P+1>(0);
	}

	// ...And this function will be used, instead, which will return the current counter, or assert in case next() hasn't been called yet.
	template <typename Unique, int I = 0, int P = 0>
	static constexpr int current(double)
	{
		static_assert(I != 0 || P != 0, "You must invoke next() first");

		if constexpr (P == 0)
			return I-1;
		else
			return current<Unique, I+(1<<(P-1)), 0>(0);
	}
#else // FAMETA_BINARY_LOOKUP
	// If slot_allocated(slot<I>) has NOT been defined, then SFINAE will keep this function out of the overload set...
	template <typename Unique, int I = 0, bool = slot_allocated(slot<I>())>
	static constexpr int next(int)
	{
		return next<Unique, I+1>(0);
	}

	// ...And this function will be used, instead, which will define slot_allocated(slot<I>) via allocate_slot<I>.
	template <typename Unique, int I = 0>
	static constexpr int next(double)
	{
		return allocate_slot<I>::value;
	}

	// If slot_allocated(slot<I>) has NOT been defined, then SFINAE will keep this function out of the overload set...
	template <typename Unique, int I = Start, bool = slot_allocated(slot<I>())>
	static constexpr int current(int)
	{
		return current<Unique, I+1>(0);
	}

	// ...And this function will be used, instead, which will return the current counter, or assert in case next() hasn't been called yet.
	template <typename Unique, int I = Start>
	static constexpr int current(double)
	{
		static_assert(I != 0, "You must invoke next() first");

		return I-1;
	}
#endif // !FAMETA_BINARY_LOOKUP

};

}

}

#endif // FAMETA_COUNTER_HPP
