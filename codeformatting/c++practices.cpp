---
layout: default
---

# C++ coding practices

Although this chapter is not FreeCAD specific, it is provided here to help both developers and code reviewers to ensure
clean and easily maintainable code. The practices presented should be treated like food recipies - you can play with them, alter them - but every change should be thoughtful and intentional.

This document is __very__ much inspired by the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines).
Most rules presented here are from this document and whenever something is not covered or you are in doubt - 
don't hesitate to consult it. 

> [!NOTE]  
> Remember that code review is a collaborative discussion. Don’t hesitate to ask for clarification or help when needed. Reviewers can also make mistakes, the goal is to work together to refine the code to a point where everyone is satisfied.

While this guideline might not be consistently followed throughout all of the existing codebase, adhering to these practices moving forward will help improve the overall quality of the code and make future contributions more maintainable.

## Avoid anonymous namespaces

Anonymous namespaces are very convenient, but makes code unreachable by tests
and other code.
Some code might only make sense in a given context, but if the functionallity
is generic, it could be put in a library or suchlike.

Use a (named) namespace to house free functions rather than a class or struct
full of static (or what should be static) functions.
In addition, private static functions doesn't even need to be decleared in the header file.

## Algorithms and data structures

> Algorithms + Data Structures = Programs
--- Niklaus Wirth, 1976

> STL algorithms say what they do, as opposed to hand-made for loops that just show how they are implemented. By doing this, STL algorithms are a way to rise the level of abstraction of the code, to match the one of your calling site.
--- Jonathan Boccara, 2016

> Debugging code is twice as hard as writing the code in the first place. Therefore, if you write code as cleverly as possible, you are, by definition not smart enough to debug it.
--- Brian W. Kernighan

Data is information, facts etc. An algorithm is code that operates on data.

Programming languages, or their libraries, include thoroughly tested algorithms to handle common data structures. 

By properly considering algorithms and data structure, and keeping data separate from code, both code and data become simpler, more reliable, more flexible, and easier to maintain for the next person.

Raw loops are those starting with `for`, `while` etc. While there are many options on how to write a loop, readabillity and maintainabillity should be the priority.

```cpp
// Pre-C++11:
for(std::map<KeyType, ValueType>::iterator it = itemsMap.begin(); it != itemsMap.end(); ++it) {
	//...
    doSomething(*it);
	//...
}

// C++11
for(auto item : itemsMap) {
	//...
	doSomething(item);
	//...
}

// C++17
for(auto [name, value] : itemsMap) {
	//...
	doSomething(name, value);
	//...
}
```

Another way, which can be even better, is to use STL `<algorithm>` library, which offers a wealth of proven, declarative solutions, e.g.:
```cpp
// C++17
std::for_each(stuff.begin(), stuff.end(), do_something);
auto result = std::find_if(stuff.begin(), stuff.end(), do_something);

// C++20
std::ranges::for_each(stuff, do_something);
auto result = std::ranges::find_if(stuff, do_something);
```

Note, STL `<algorithm>` library can also use lambdas.


**Example**: For a given input, find the appropriate prefix.
```cpp
constexpr std::array<std::string_view, 5> prefixes {"", "k", "M", "G", "T"};
size_t base = 0;
double inUnits = bytes;
constexpr double siFactor {1000.0};

while (inUnits > siFactor && base < prefixes.size() - 1) {
	++base;
	inUnits /= siFactor;
}

auto prefix = prefixes[base];
```

Let’s make the data more expressive, more self-contained, and use STL
algorithm `find_if`:

```cpp
using PrefixSpec = struct {
	char prefix;
	unsigned long long factor;
};

static constexpr std::array<PrefixSpec, 7> sortedPrefixes {
	{{'E', 1ULL << 60}, // 1 << 60 = 2^60
	{'P', 1ULL << 50},
	{'T', 1ULL << 40},
	{'G', 1ULL << 30},
	{'M', 1ULL << 20},  // 1 << 20 = 2^20 = 1024
	{'k', 1ULL << 10},  // 1 << 10 = 2^10 = 1024
	{'\0', 0}}};
    
const auto res = std::find_if(prefixes, [&](const auto& spec) {
	return spec.factor <= size;
});

// Add one digit after the decimal place for all prefixed sizes
return res->factor
	? fmt::format("{:.1f} {}B", static_cast<double>(size) / res->factor, res->prefix)
	: fmt::format("{} B", size);
```

Simpler, cleaner, more reliable. No raw loops, magic numbers or calculations.
Note the reverse iterator.

## Code comments

> Don’t comment bad code—rewrite it.
--- Brian W. Kernighan and P. J. Plaugher, 1974

Comments are a piece of the program that the computer doesn't execute. While the intension is to aid comprehension, they have a tendency to get out-of-sync with the actual code it is commenting.

It is prefered that code is self documenting when possible. This can be achived using good naming and structure.

In some cases, comments can be nessesary, to convay information that cannot be described in code. This can be links to bugs that a workaround describe or why edge cases are needed.

## Conditionals

Every branch in code doubles the number of paths and makes the code difficult to debug and maintain.

Simple `if else` may be better expressed as a ternary `_ : _ ? _`. There are often ways to avoid `else`, when possible the resulting code will be better without. Ternary is ok though.

Even modestly complex code in an `if` or `else` branch should be
extracted to a function or lambda.

A sequence of conditionals stepping through related variables may indicate
code and data conflation.

Can also apply to `switch` statements.

Complicated `if else` code might benefit from converting to a state machine.


## Const correctness

> const all of the things.
--- Jason Turner, 2021

`const` is a statement of intent that something is to be immutable
(not able to change). Immutability aids reliability and is done using `constexpr`

This provides compile-time evaluation. Can increase compile time, but speedup runtime. More errors are caught at compile-time.

`constexpr` is preferable for everything that can be so represented.

## Reducing dependencies

> Any source code dependency, no matter where it is, can be inverted
--- Robert C. (Uncle Bob) Martin

Hard dependencies makes the codebase more entangled, makes changes more difficult, and makes unit testing really difficult.

Examples of dependencies creeping in:
```cpp
Application::getSomething(); // or any other singleton
SomeDistantClass fing;
void method(AnotherDistantClass values){}
```

This does not stand in contrast to code reuse, but it does require care when designing how code is accessing data.

A function which has hard dependencies cannot function, be understood, edited or tested, without the context of its dependencies. Avoiding these types of dependencies without code duplication is worth striving for.

Code and its dependencies are said to be _coupled._ When different pieces of code _have the same_ dependency, they in turn are coupled to each other.

Required information can be injected via constructor or method parameters. 

If it is necessary to introduce external code (e.g. a service object), do so by passing an interface, helper function or similar to avoid coupling.

Even in complex cases where singeltons are used we can avoid hard coupling and make unit test a breeze.
Example:
```c++
// For this example the implementation is included in the class definition
// to simplify the example.
class Example {
public:
    using DependencyHelper std::function<std::unique_ptr<Dependency>()>;
    Example(
        //...
        DependencyHelper provide_helper_ = {}
    ) : provide_helper_(std::move(provide_helper))
    {
        if (!provide_helper_) {
            provide_helper_ = my_namespace::defaultDependencyHelper();
        }
    };
private:
    DependencyHelper provide_helper_;
}
```

Ideally, all dependencies can be avoided, what dependencies to keep depends on the situation.

If your code needs external data that was not available when calling this
code, then there's likely a better overall design that can be used.

**Code that has no hard dependencies is single-purpose, reusable, changeable, and testable. Everything is simpler!**

## Code design

> Any fool can write code that a computer can understand. Good programmers write code that humans can understand
--- Martin Fowler

Something well-designed is _instantly_ understandable, and a pleasure to work
with. Programming principles developed over the last 50 years ensure
well-designed code not only runs well, but is also understandable by humans.

Well-designed code is adaptable, flexible, easily changed to suite new
circumstances.

Well-designed code is completely free from hard-coded data.

Understandable code can be more easily evaluated for correctness.

For a novice programmer many of these concepts are probably quite foreign, but with a little study and help from the community and code reviews, better code will ensue.

---

## Enums

Used correctly, enums are invaluable.

Using enums:
* ...specifically `enum class` allows strongly typed and scoped enumeration to be created.
* ...instead of booleans for function arguments makes it easy to understand what the argument means without consulting the documentation or at least the method signature.
* ...instead of integers when expressing anything other than numbers.

Using enums to codify data values is strongly discouraged, as enums are best suited for representing fixed, intrinsic states rather than variable data. Instead, use `std::unordered_map<>` or other data structures that better represent dynamic or data-driven values, offering flexibility and improving maintainability.

## Minimize getters and setters

In object-oriented design, a class should encapsulate behavior, not just data. **Frequent use of getters and setters can limit a class’s ability to fully encapsulate its responsibilities** and may suggest that the data could be handled differently.

Consider:

* Using a struct for simple data containers with public fields.
* Focusing on methods that represent meaningful actions rather than exposing raw data.

A well-designed class manages its own state and provides behavior, not just access.

## Appropriate typing

Using strings for everything can make code harder to understand and maintain. Use appropriate types to add clarity and structure.

## Main code path and indentation

> If you are past three indents you are basically screwed.
> Time to rewrite.
--- Linus Torvalds, 1995

Indented code can be diﬃcult to reason about, and fragile.

Main execution path should be the least indented one, i.e. conditions should cover specific cases. Early-Exit should be preferred to prune unwanted execution branches fast.

Example:
```cpp
if (something) {
	doSomething();
	if (somethingElse) {
		doSomethingElse()
		if (somethingElseAgain) {
			doThing();
		} else {
			doDifferent();
		}
	} else {
		doTheOther();
	}
} else {
	doNothing();
}
```

Can be changed into:
```cpp
if (!something) {
	doNothing();
	return;
}
doSomething();
if (!somethingElse) {
	doTheOther();
	return;
}
doSomethingElse();
if (!somethingElseAgain) {
	doDifferent();
	return;
}
doThing();
```

## Initialization

**Initialize all objects, and, if possible, make const or better still, constexpr.**

Avoid default constructors. If there is not yet a value for an object, then there's no need to create it. Declare variables close to where they are used (there's no need to start the declear variables at the start of the block like in ansi-c). Joining declaration and initialization allows const:

```cpp
AType thing; // mutable. Does it have a default constructor?
const AType thing3 { calcVal() }; // immutable
```

Using uninitialized POD type variables is undefined behavior.

It _is_ OK to declare variables inside a loop.

Prefer uniform initialization { } (also called brace initialization)(since C++11). Prevents narrowing.

Clarifies not assignment (=) or function call (()).

Initialize class member variables at declaration, not in constructors (since C++11):
- Simplifies constructors
- Avoids repetition
- Establishes default state

Static members can be defined in header file, no need to split to source file:
```cpp
struct Something {
	static const int _value_ = 2; // since C++98 for int
	static inline std::string _something_ {"str"}; // since C++17
}
```
Lambdas can create and initialize variables in the capture, removing the
need to have the parameter in the surrounding scope. But don’t forget
mutable if you want to update its value. The variable stays for the lifetime
of the lambda (think static, but better).

## Lambdas

> One of the most popular features of Modern C++.
--- Jonathan Boccara, 2021

A lambda (since C++11) is like a function that can be named, passed into and
out of functions. They accept parameters, can be generic (since C++ 14), do a
really good job of type deduction (auto), and can be constexpr (since C++17).

Lambdas can capture data from enclosing scopes and enforce encapsulation, simplifying surrounding scopes.

Lambdas are indispensable when breaking up complex code into individual
responsibilities, perhaps as a precursor to moving to functions. Ditto
when removing repetition.

Whilst lambdas are quite happy with simple auto parameters, best not
to omit const or & as appropriate. Types may be required to help the IDE.

Consider the following code: 
```c++
doSomething()
if (somethingWentWrong()) {
    // Clean up this
    // ...
    // Clean up that
    return;
}
doSomethingElse();
if (somethingElseWentWrong()) {
    // Clean up this
    // ...
    // Clean up that
    return;
}
doSomeOtherThing();
// Clean up this
// ...
// Clean up that
```
Using lambdas we can remove code duplication to create the following:
```c++
auto cleanup = [](){
    // Clean up this
    // ...
    // Clean up that
};

doSomething()
if (somethingWentWrong()) {
    cleanup();
    return;
}
doSomethingElse();
if (somethingElseWentWrong()) {
    cleanup();
    return;
}
doSomeOtherThing();
cleanup();
```


## Avoid macros

---

While macros was needed back in the days. Today, with modern c++, that's usually not the case anymore.

Macros are global, can be lead to unpredicted side effects and are difficult to debug. Consider replacing with a function. For conditional compilation in
templates consider constexpr if.

## Avoid magic literals

“Magic” literals placed directly in code offer no clue as to what exactly
is being specified or its origin, and no clue if the same data is used
elsewhere. Comprehension and maintenance burden.

To document what the magic literal is, use a suitable named constant.

Instead of this:
```cpp
displayLines(25);
```

Do the following instead:
```c++
constexpr auto sandardScreenLength {25};
displayLines(sandardScreenLength);
```

## Good naming

**Clear, concise, naming makes code understandable**.

For an object whose purpose is to _do_ something (service object), prefer a verb. E.g. `renderer`.

For an object that _is_ something (value object), prefer a noun. E.g. `drawing`.

Something diﬃcult to name concisely likely does not have a single purpose and needs refactoring.

Use names that are specific. E.g. `SaveLogToDisc`, not `ProcessLog`. "Process" could be anything.

A variable named after its data value defeats the whole point of a variable:
```cpp
struct Dog {
	std::string color {};
};
Dog redDog {"blue"}; // BAD
// 200 lines later, -_obviously_\- redDog is red! He’s blue? WTF?
Dog dog {"pink"}; // OK
constexpr auto redDogColor {"red"}; // OK
```
See also variable sets.

## out parameter 👎

Out parameters are _non-const, by-reference_, function parameters.
Known to cause hard to find bugs.

Whether values are updated by the function is not obvious.

**Where possible make function parameters `const` or `const&` and return
a value, or return a container to return multiple values**.

RVO simplifies return and usually elides copies.
```cpp
auto func = [](const auto& str, const auto num) {
	return { str, num };
};
```
Structured binding (since C++17) simplifies reading back the result at the calling side:
```cpp
auto [name, value] = func("qty", 2);
```

## repetition 👎

> One of the things I've been trying to do is look for simpler or rules
> underpinning good or bad design. I think one of the most valuable rules
> is to avoid duplication
--- Martin Fowler

> Code duplication is by far one of the worst anti-patterns in software
> engineering, eventually leading to buggy and unmaintainable systems.
--- Magnus Stuhr, 2020

> Don’t Repeat Yourself.
--- Andy Hunt and Dave Thomas, 1999

> Duplicate code is the root of all evil in software design
--- Robert C. (Uncle Bob) Martin

Alright already! Repetition should be _ruthlessly_ eliminated! And not just
identical code, but similar code too!

**DRY** = “_Don’t Repeat Yourself_”

**WET** = “_Waste Everyone’s Time_”, “_Write Everything Twice_”

Change requires finding every usage (difficult) and replicating the change
(error-prone). Failure to catch just one instance creates a nasty bug that
might remain undiscovered for a long time. Comprehension requires studying
every item. Small differences are notoriously difficult to spot.

**Repetition is entirely avoidable!**

The variant part (the bit that is different between usages) of repeating
code is often just one or two simple items in a long gobbledygook statement.
The variant parts can be extracted and passed as parameters to a function
or lambda executing the common body. A sequence of repeated code likely
indicates the underlying data is actually a set and hence should be defined
in a container and dealt with accordingly. clang-format off is often a sign
of repetition or data represented by code.

See also: ternary operator, variable sets, naming.

## static 👎

Often best avoided. For `const` variables consider `constexpr`, or
initialisation in lambda capture.

`static` functions _may_ be better moved out of class into namespace or some
utility library/file.

See also initialization.

## ternary operator 👍

Reduce six lines:
```cpp
int r; // can’t be const

if (x == 2) {
	r = 2;
} else {
	r = 3;
}
```
to one, with a single assignment, no curly braces, no repetition, and
const option:
```cpp
const int r = x == 2 ? 2 : 3;
```
Also great for simplifying return statements. What’s not to like?

## unit test 👍

New code should be delivered with unit tests. Whilst e.g. GUI code is
not so testable, it should contain only the GUI part, no “business logic”.
Ideally, unit tests are created _before_, or _during_, code creation, and
run after every small change, ensuring all behaviours are tested and that
code conforms to the tests (rather than the other way round).

**A unit test targets a single behavior of a single unit of code, with a single assertion**.

If code is difficult to unit test consider if it can be extracted from
its surrounds — maybe into some library-like class — where it can be
tested in isolation. Is it possible to refactor to remove hard-wired
dependencies? Is it trying to do too much? Is it too tightly coupled?
See also dependencies.

Prematurely falling back to integration testing might be a sign of
failure to properly structure new code. Integration tests are to ensure
multiple units play well together.

## variable sets 👎

Related variables having names closely coupled to their initial value. E.g.:
```cpp
Item fred { "Fred", 20 };
Item martha { "Martha", 30 };
Item george { "George", 40 };
```
Issues:
- Data represented by code, variables no longer really variable. See also naming
- Every declaration, definition and usage has to be repeated = exploding code size
- Comprehension and maintenance nightmare

Solution:
- Move data into a container e.g. constexpr

std::array

(not vector or map).

Container elements are typically value, array, pair or tuple:
{% raw %}
```cpp
using Pair = std::pair<std::string_view, size_t>;

constexpr auto numItems {3};

constexpr std::array<Pair, numItems> items {{
	{ "Fred", 20 },
	{ "Martha", 30 },
	{ "George", 40 }
}};
```
{% endraw %}
Or struct, which has the advantage of named elements, but is slightly more overhead:
{% raw %}
```cpp
struct Button {
	std::string_view name;
	size_t height;
	size_t width;
};

constexpr auto numButtons {3};

constexpr std::array<Button, numButtons> buttonDefs {{
	{ "Go", 25, 25 },
	{ "Get set", 20, 20 },
	{ "On your marks", 15, 15 }
}};
```
{% endraw %}
