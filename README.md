# C++ Enhance [![Build Status](https://travis-ci.org/johannesgerer/Enhance.svg?branch=master)](https://travis-ci.org/johannesgerer/Enhance)

... is a single-header C++ library, that provides tools for
automatic derivation of zero-overhead class enhancements, such as
comparison/assignment operators, hash/swap/serialization/pretty
printing functions and much more.

Writing these by hand is cumbersome and repetitive, as they usually
all involve the same fields of the class. Handwritten implementations
are error prone and a hassle to keep up-to-date.

*Enhance* endows classes with the enhancements listed above by
inheritance. The only thing that needs to be specified is the list of
member variables or other [accessors](Documentation.md#2-accessors). If the class
definition cannot be altered, non-intrusive versions are also
available.

This is achieved in pure templated C++ without the use of
macros. There are a few macros built on top of `Enhance`'s core
functionality, provided for such cases where inheritance is not
possible (like specializing `std::hash`).

*Zero-overhead:* Aggressive compiler optimizations (like `g++ -O2`)
should eliminate any overhead caused by *Enhance* compared to a manual
implementation, as it basically only shuffles pointers and references
around in ways that are known at compile time.


# 1 Installation

Just copy the `enhance.hpp` header file into your project's source
tree. Alternatively, you can put it in your compiler's search
path or tell the compiler where to find it (e.g. `g++
-I/path/to/directory`).

# 2 Basic Usage

In the most basic use case, the class to be enhanced needs a template
member function `enhance` taking one argument, which gets passed all
fields that should be used to build up the enhancements. As an example
take a class `Point2D` describing points in the two dimensional plane:


```c++
#include "enhance.hpp"
#include <iostream>
#include <set>

using namespace std;
using namespace enhance;


struct Point2D : LessComparable<Point2D>,
                 ScalarMultiplicable<int, Point2D>,
                 EqualComparable<Point2D>,
                 Insertable< '[', ',', ' ', ']' ,Point2D>,
                 WithScalarProduct<int, Point2D>
{
  int x,y;

  Point2D(int x, int y):x(x), y(y) {}
  
  template<class C> void enhance(C& c) const{
    c(&Point2D::x, &Point2D::y);
  }
};

int main(){
  Point2D v{2,3}, w{5,1}, q{20,30};
  
  // `LessComparable` provides `operator<` needed e.g. for `std::set`:
  std::set<Point2D> points = {v, w, q};

  // `ScalarMultiplicable` provides:
  v *= 10;
  
  // `EqualComparable` gives `operator==`:
  cout<< std::boolalpha << (v == q) << endl;
  // prints: true
  
  // `Insertable` defines `operator<<` for pretty printing:
  cout<< v << endl;
  // prints: [20, 30]
  
  // `WithScalarProduct` defines the scalar product:
  cout << v * w <<endl;
  // prints: 130 
}

```

More examples of working code (including examples from this README)
are found in the unorganized collection of [tests](tests/tests.cpp).

# 3 Tutorial

See http://johannesgerer.com/enhance for a **tutorial** demonstrating
many features of Enhance.

# 5 Documentation can be found [here](Documentation.md)

# 4 Extension

The core concept implemented in *Enhance* is very flexible and the
current set of modules by no means captures everything that is
possible. It is written with extensibility in mind, so please open an
issue, or send a pull request, if you think *Enhance* could provide
more features.

Not currently implemented: Stream extraction (`operator>>`),
in/decrement (`operator++`), specializations of `std::less` or
`std::swap`, ...

# 6 Dependencies

The library depends only on the *C++ Standard Library*.

# 7 Compiler support

This software should be compatible with any `c++11` compiler. More specifically:

| Compiler  | Minimum version  | Successfully tested  | 
|---|---|---|
| GCC  |  4.7  |  6.2.0 (in [Travis CI](https://travis-ci.org/johannesgerer/Enhance))  | 
| Clang   | 3.0  |  3.5.0 (in [Travis CI](https://travis-ci.org/johannesgerer/Enhance)) | 
| MS Visual C++  | 12.0 (2013)  | 14.0 (2015)  | 
| Intel C++  | 12.1  |   |
| IBM XLC++  | 13.1.3  |   |
| Sun/Oracle C++  | 5.13  |   |
| Cray  | 8.4  |   |
| EDG eccp  | 4.2  |   |

If you use an untested compiler, please send a pull request or open an
issue with your (un)successful experiences.

*Enhance* uses *template aliases*, which can be backported easily, and
*type traits*, but only for `ConstCast`. Further, it makes use of
*variadic templates* if available, but provides manual fallback
unpacking of up to 10 accessors. This fallback mechanism currently is
only implemented for Visual C++, but is trivial to add for other
compilers.

# 8 Testing

Tests are currently stored in
[`tests/tests.cpp`](tests/tests.cpp). Pull requests for new features
are encouraged to also add tests to this file.

The current version of `tests.cpp` would benefit from a complete
makover.

## 8.1 Dependencies


The tests use the [Catch](https://github.com/philsquared/Catch) test
library and the
[cxx-prettyprinting](https://github.com/louisdx/cxx-prettyprint)
library, whose single-header versions are included in this
repository. It also uses the
[Boost.Serialization](http://www.boost.org/doc/libs/1_62_0/libs/serialization/doc/index.html)
library, which needs to be installed separately, e.g. through your
package manager (`libboost-serialization-dev` on ubuntu) or manually,
e.g.  on
[Windows](https://sourceforge.net/projects/boost/files/boost-binaries/).
To disable tests using this library add `#define ENHANCE_NO_SERIALIZE`
to `tests.cpp`.

## 8.2 Compiling and running the tests

To run the tests using `gcc`, checkout this repository and run
`make`.

To run the tests using *Visual C++* under *Visual Studio* start the
*Developer Command Prompt* or if you use the free *Visual C++ Build
Tools*, the *x64 (or x86) Native Tools Command Prompt* and run:

```
cl /EHsc test_main.cpp  tests.cpp /I path\to\boost_1_62_0 /link /LIBPATH:"path\to\boost_1_62_0\lib64-msvc-14.0" /SUBSYSTEM:CONSOLE
```

(tested with VC++ 2015, msvc14, compiler version 19.0 and Boost 1.62.)
