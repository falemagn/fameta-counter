# fameta-counter
Compile time counter that works with all major modern compilers, compatible with C++11 and above.

### TL;DR
If you're in a hurry, [click here for the How To](#example-of-usage)

### Intro
Back in 2015, [Filip Roséen discovered](http://b.atch.se/posts/constexpr-counter/) yet another C++ twist that was unforeseen by its inventors. Venturing in these dark corners of the language doesn't come without consequence, as compiler vendors haven't felt compelled to stay compatible with whatever was the mechanism that allowed Roséen's work to actually perform as expected. The fact that the C++ Core language Working Group decided that [this mechanism is "_archane_"](http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#2118) hasn't made things better, but since they _only_ stated that _«it **should** be made ill-formed»_ gives us hope that it might not, after all, go.

Meanwhile others have leveraged the technique to make compilers do all sort of seemingly _magic_ things: [unconstexpr](https://github.com/DaemonSnake/unconstexpr) from DaemonSnake is pretty nifty, even though it only works with g++7 and presumably leaves the standard too far behind, eating the dust.

### But...
Compile-time counters are a pretty nice thing to have, and the fact that most compiler vendors implemented a `__COUNTER__` macro into their preprocessor, even though the standard doesn't mandate it, gives credit to this idea. But `__COUNTER__` is not enough: it's just a macro, and as such has no knowledge about the context it's being used in and assumptions about its value can be messed up by the code that gets `#include`'d into our own. 

Other kind of counters, more language-aware, have been implemented, like [CopperSpice has done](https://www.youtube.com/watch?v=lCDA3xaLnDg), but they come with their own limitations, among which stringent limits to the counter maximum value or the fact that incrementing the counter can only be done in a namespace scope (i.e. not within a function body or a class definition).

### The solution
[Filip Roséen's work](http://b.atch.se/posts/constexpr-counter/) doesn't work any longer with recent-ish compilers. I did some investigations and came up with some ideas as of why this is happenning (read about it on [StackOverflow](https://stackoverflow.com/questions/60082260/c-compile-time-counters-revisited)), and on the basis of those ideas I built `fameta::counter`.

### How `fameta::counter` works
`fameta::counter` is as easy to use as Roséen's one, but it does need a bit of help from the user, which makes the usage of the counter more verbose, but the preprocessor makes it perfectly manageable.

The main _trick_ that makes `fameta::counter` work is that a **unique** value or type has to be used as the template parameter of the `next<Unique>()` method each time the _next_ counter value has to be retrieved. This makes the compiler **always instantiate** a new set of methods to access and modify the counter, which avoids the _"caching problem"_ that seems to affect Roséen's solution.

#### `__LINE__` and `__COUNTER__`
Where do we get such a unique value from? The preprocessor comes to our help: the `__LINE__` macro, which increments its value each time it's used, with the only caveats to remember that it expands to the same value each time it's used on a given source line and that it's reset at the beginning of each file being included in the current translation unit.

This means that `__LINE__` might not work in two cases:
1. if it's usage is hidden within other macros that expand on multiple `next<__LINE__>()` calls on the same line;
2. if the counter's scope is the whole translation unit **and** it's incremented in two or more different files **and** at least two instances of the `next<__LINE__>()` calls in two different files happen to expand on the same line number in those files.

In both the above cases, the uniqueness prerequisite would be invalidated.

If those scenarios present themselves, a simple workaround, is to use the `__COUNTER__` macro, which changes value even if used multiple times on the same line and has a __unique value__ across the _whole translation unit_. `__COUNTER__` is not defined by the standard, but all the major compiler vendors support it.

So, to recap, instead of doing `counter::next()` as Roséen's code would have allowed us to do, all we need to do is `counter.next<__COUNTER__>()` or, in the rare event that the compiler doesn't provide the `__COUNTER__` macro, `counter.next<__LINE__>()`, which isn't ___that___ bad after all, is it?

| A moment of your attention, please |
| --- |
| It 's been [noted](https://stackoverflow.com/questions/60082260/c-compile-time-counters-revisited#comment106263031_60082260) that [Anthony Williams](https://stackoverflow.com/users/5597/anthony-williams) on [Stack Overflow](https://stackoverflow.com/a/58200261/566849) proposed a solution that avoids having to pass a unique value to each call into the counter, just like Filip Roséen's solution. However, his solution doesn't seem portable across compilers and [has a few quirks](https://stackoverflow.com/questions/51601439/constexpr-counter-that-works-on-gcc-8-and-is-not-restricted-to-namespace-scope/58200261#comment106343647_58200261) also with the compilers that make it work. And I can't fully grasp how it works, where it does. Shame on me. |

| One more short moment |
| --- |
| DaemonSnake has produced a version of his `uncostexpr` library that makes use of the new C++20 facilities: [uncostexpr-cpp20](https://github.com/DaemonSnake/unconstexpr-cpp20). His library does lots more than providing a counter, but fameta-counter works also with just C++11. Also read [his announcement on reddit](https://www.reddit.com/r/cpp/comments/e99enu/c20_library_mutable_constexpr_expression_for/).|

### Example of usage

```cpp
#include <fameta/counter.hpp>

// Utility template to generate a unique type that makes the whole counter unique
template <int I>
struct U;

// First template parameter is the unique type, 
// the second template parameter is the first value we want fameta::counter::next() to return.
constexpr fameta::counter<U<1>, 100> C;

// As if int array[] = { 100, 101, 102 };
constexpr int array[] = {
    C.next<__COUNTER__>(), 
    C.next<__COUNTER__>(),
    C.next<__COUNTER__>()
};

static_assert(array[0] == 100 && array[1] == 101 && array[2] == 102, "oops");

// We can instantiate as many counters as we want, as long as we abide by the contract.
// Third template parameter is the step at which the the counter has to be incremented.
constexpr fameta::counter<U<2>, 100, 10> C2;

// As if int array2[] = { 100, 110, 120 };
constexpr int array2[] = {
    C2.next<__COUNTER__>(),
    C2.next<__COUNTER__>(),
    C2.next<__COUNTER__>()
};

static_assert(array2[0] == 100 && array2[1] == 110 && array2[2] == 120, "oops");

// Counters can also be decremented: just set a negative step
constexpr fameta::counter<U<3>, 1000, -10> C3;
 
// As if int array3[] = { 1000, 990, 980 };
constexpr int array3[] = {
    C3.next<__COUNTER__>(),
    C3.next<__COUNTER__>(),
    C3.next<__COUNTER__>()
};

static_assert(array3[0] == 1000 && array3[1] == 990 && array3[2] == 980, "oops");

// We could also use __LINE__ and it would still work in **MOST** cases.
// By default, the start value is set to 0 and the step to 1.
constexpr fameta::counter<U<4>> C4;
 
// As if int array3[] = { 0, 1, 2};
constexpr int array4[] = {
    C4.next<__LINE__>(),
    C4.next<__LINE__>(),
    C4.next<__LINE__>()
};

static_assert(array4[0] == 0 && array4[1] == 1 && array4[2] == 2, "oops");

// To simulate the case in which two counters shared the same lines in two different files within the same translation unit,
// we will use hardcoded numbers.
constexpr fameta::counter<U<5>> C5;
 
// As if int array5[] = { 0, 1, 2};
constexpr int array5[] = {
    C5.next<1010>(),
    C5.next<1020>(),
    C5.next<1030>()
};

static_assert(array5[0] == 0 && array5[1] == 1 && array5[2] == 2, "oops");

constexpr fameta::counter<U<5>> C6;
 
// What do you expect? I guess: int array6[] = { 0, 1, 2};
// But instead we get: { 3, 2, 4 }. Why? 
// Because C6 and C5 share the same type fameta::counter<U<5>>, 
// hence C5.next<1030>() and C6.next<1030>() return the same value,
// whilst C6.next<1001>() and C6.next<1040>() increment the value

constexpr int array6[] = {
    C6.next<1001>(),
    C6.next<1030>(),
    C6.next<1040>()
};

static_assert(array6[0] == 3 && array6[1] == 2 && array6[2] == 4, "oops");

// We can also use CRTP to make the counter unique
struct C7: fameta::counter<C7>{};

constexpr int array7[] = {
    C7::next<1001>(),
    C7::next<1030>(),
    C7::next<1040>()
};

static_assert(array7[0] == 0 && array7[1] == 1 && array7[2] == 2, "oops");
```
See it working on godbolt: https://godbolt.org/z/YaYaqbfxc
