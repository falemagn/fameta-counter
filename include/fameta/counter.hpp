// Author: Fabio Alemagna
// Source: https://github.com/falemagn/fameta-counter
// Inspired to Filip Roséen's work. See https://stackoverflow.com/questions/60082260/c-compile-time-counters-revisited

#ifndef FAMETA_COUNTER_HPP_
#define FAMETA_COUNTER_HPP_

namespace fameta {
    template <int StartN, int StartValue = 0>
    class counter;
}

template <int StartN, int StartValue>
class fameta::counter {    
#if defined(__INTEL_COMPILER) || defined (_MSC_VER)
    template <int N, typename = void>
    struct slot {
        #if defined(__INTEL_COMPILER)        
        #   pragma warning disable 1624
        #endif
        friend constexpr int slot_value(slot<N>);
    };

    template <typename _>
    struct slot<StartN, _> {
        constexpr slot(){}
        friend constexpr int slot_value(slot<StartN>) {
            return StartValue-1;
        }
    };

    template <int N, int I>
    struct writer {
        friend constexpr int slot_value(slot<N>) {
            return I;
        }

        static constexpr int value = I;
    };

    template <int N, int R = slot_value(slot<N>())>
    static constexpr int reader(int, slot<N>) {
        return R;
    };
#else
    template <int N, typename = void>
    struct slot {
        #if defined(__GNUC__) && !defined(__clang__)
        #   pragma GCC diagnostic ignored "-Wnon-template-friend"
        #endif
        friend constexpr auto slot_value(slot<N>);
    };

    template <typename _>
    struct slot<StartN, _> {
        constexpr slot(){}
        friend constexpr auto slot_value(slot<StartN>) {
            return StartValue-1;
        }
    };

    template <int N, int I>
    struct writer {
        friend constexpr auto slot_value(slot<N>) {
            return I;
        }

        static constexpr int value = I;
    };

    template <int N, typename = decltype(slot_value(slot<N>()))>
    static constexpr int reader(int, slot<N>, int R = slot_value(slot<N>())) {
        return R;
    };
#endif

    template <int N>
    static constexpr int reader(float, slot<N>, int R = reader(0, slot<N-1>())) {
        return R;
    };

public:    
#if !defined(__clang_major__) || __clang_major__ > 7
    template <int N, int Step = 1>
    static constexpr int next(int R = writer<N, reader(0, slot<N-1>())+Step>::value) {
        return R;
    }
#else
    template <int N, int Step = 1>
    static constexpr int next(int R = writer<N, reader(0, slot<N>())+Step>::value) {
        return R;
    }
#endif
};

#endif
