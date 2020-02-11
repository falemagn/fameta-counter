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

The main _trick_ that makes `fameta::counter` work is that a ___monotonically increasing___ value has to be passed to it at construction time and any time the _next_ counter value has to be retrieved. This makes the compiler **always instantiates** a new set of methods to access and modify the counter, which avoids the _"caching problem"_ that seems to affect Roséen's solution.

At this point you might wonder: Isn't a ___monotonically increasing___ value a... counter? Do we really need a counter, to make this counter work? Well, yes, we do, but the values we need don't have to be consecutive neither do they need to have a given _start_, we just need them to _increase by an arbitrary amount_ at each call.

#### `__LINE__` and `__COUNTER__`
Do we have such a thing in the language? Yes, we do, the preprocessor provides us with one such thing: the `__LINE__` macro. However, `__LINE__` is good only _most_ of the times, because it could fail if within the same _translation unit_ two files used two different counters that happened to be positioned at about the same lines in their respective files: remember that we said that each call into the counter has to be __unique__? Well, that might not be the case if we used `__LINE__`. 

There's a workaround, a bit tedious, that will be presented in the example below, but most compilers also provide a `__COUNTER__` macro, and that is perfect for our needs because it changes value even if used on the same line and has a __unique value__ across the _whole translation unit_.

So, to recap, instead of doing `counter::next()` as Roséen's code would have allowed us to do, all we need to do is `counter.next<__COUNTER__>()` or, in the rare event that the compiler doesn't provide the `__COUNTER__` macro, `counter.next<__LINE__>()`, which isn't ___that___ bad after all, is it?

| A moment of your attention, please |
| --- |
| It 's been [noted](https://stackoverflow.com/questions/60082260/c-compile-time-counters-revisited#comment106263031_60082260) that [Anthony Williams](https://stackoverflow.com/users/5597/anthony-williams) on [Stack Overflow](https://stackoverflow.com/a/58200261/566849) proposed a solution that avoids having to pass a _monotonically increasing_ number to each call into the counter, just like Filip Roséen's solution. However, his solution doesn't seem portable across compilers and [has a few quirks](https://stackoverflow.com/questions/51601439/constexpr-counter-that-works-on-gcc-8-and-is-not-restricted-to-namespace-scope/58200261#comment106343647_58200261) also with the compilers that make it work. And I can't fully grasp how it works, where it does. Shame on me. |

| One more short moment |
| --- |
| DaemonSnake has produced a version of his `uncostexpr` library that makes use of the new C++20 facilities: [uncostexpr-cpp20](https://github.com/DaemonSnake/unconstexpr-cpp20). His library does lots more than providing a counter, but fameta-counter works also with just C++11. Also read [his announcement on reddit](https://www.reddit.com/r/cpp/comments/e99enu/c20_library_mutable_constexpr_expression_for/).|

### Example of usage

```cpp
#include <fameta/counter.hpp>

// First template parameter is the monotonically increasing value, 
// the second template parameter is the first value we want fameta::counter::next() to return.
constexpr fameta::counter<__COUNTER__, 100> C;

// As if int array[] = { 100, 101, 102 };
constexpr int array[] = {
    C.next<__COUNTER__>(), 
    C.next<__COUNTER__>(),
    C.next<__COUNTER__>()
};

static_assert(array[0] == 100 && array[1] == 101 && array[2] == 102, "oops");

// We can instantiate as many counters as we want, as long as we abide by the contract.
// Third template parameter is the step at which the the counter has to be incremented.
constexpr fameta::counter<__COUNTER__, 100, 10> C2;

// As if int array2[] = { 100, 110, 120 };
constexpr int array2[] = {
    C2.next<__COUNTER__>(),
    C2.next<__COUNTER__>(),
    C2.next<__COUNTER__>()
};

static_assert(array2[0] == 100 && array2[1] == 110 && array2[2] == 120, "oops");

// Counters can also be decremented: just set a negative step
constexpr fameta::counter<__COUNTER__, 1000, -10> C3;
 
// As if int array3[] = { 1000, 990, 980 };
constexpr int array3[] = {
    C3.next<__COUNTER__>(),
    C3.next<__COUNTER__>(),
    C3.next<__COUNTER__>()
};

static_assert(array3[0] == 1000 && array3[1] == 990 && array3[2] == 980, "oops");

// We could also use __LINE__ and it would still work in **MOST** cases.
// By default, the start value is set to 0 and the step to 1.
constexpr fameta::counter<__LINE__> C4;
 
// As if int array3[] = { 0, 1, 2};
constexpr int array4[] = {
    C4.next<__LINE__>(),
    C4.next<__LINE__>(),
    C4.next<__LINE__>()
};

static_assert(array4[0] == 0 && array4[1] == 1 && array4[2] == 2, "oops");

// To simulate the case in which two counters shared the same lines in two different files within the same translation unit,
// we will use hardcoded numbers.
constexpr fameta::counter<1000> C5;
 
// As if int array5[] = { 0, 1, 2};
constexpr int array5[] = {
    C5.next<1010>(),
    C5.next<1020>(),
    C5.next<1030>()
};

static_assert(array5[0] == 0 && array5[1] == 1 && array5[2] == 2, "oops");

constexpr fameta::counter<1000> C6;
 
// What do you expect? I guess: int array6[] = { 0, 1, 2};
// But instead we get: { 0, 2, 3 }. Why? 
// Because C6 and C5 share the same type fameta::counter<1000>, 
// hence C5.next<1030>() and C6.next<1030>() return the same value.

constexpr int array6[] = {
    C6.next<1001>(),
    C6.next<1030>(),
    C6.next<1040>()
};

static_assert(array6[0] == 0 && array6[1] == 2 && array6[2] == 3, "oops");

// The above suggests us a workaround: what needs to be unique is actually the counter type,
// which the uniqueness of the monotonically incremental value helps us achieve. But there's another way.
// The 4th template parameter of fameta::counter accepts an optional Tag type, that, if unique, makes the
// counter type unique too.
// The unconstexpr project does all this automatically, alas that code won't work on anything but g++7.
constexpr struct MyCounter1: fameta::counter<1000, 0, 1, MyCounter1>{} C7;
 
// As if int array7[] = { 0, 1, 2};
constexpr int array7[] = {
    C7.next<1010>(),
    C7.next<1020>(),
    C7.next<1030>()
};

static_assert(array7[0] == 0 && array7[1] == 1 && array7[2] == 2, "oops");

constexpr struct MyCounter2: fameta::counter<1000, 0, 1, MyCounter2>{} C8;

// As expected, as if int array8[] = { 0, 1, 2}.
constexpr int array8[] = {
    C8.next<1001>(),
    C8.next<1030>(),
    C8.next<1040>()
};

static_assert(array8[0] == 0 && array8[1] == 1 && array8[2] == 2, "oops");

```

See a working example on [godbolt](https://godbolt.org/z/Am5MQU)
