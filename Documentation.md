See also [Readme](README.md).

Most examples shown in this document are found in
[`tests/tests.cpp`](tests/tests.cpp).

**Everything is in the `enhance` namespace!**

# 1 How to use *Enhance*

First locate the desired [module](#4-modules). Then there several ways
to use `Enhance`.

## 1.1 The simple and intrusive way

1. Inherit from the one or more of the provided classes or — where
   inheritance is not possible — use the compiler MACROS for
   enhancements.
   
2. Add an `enhance` member function to the class, taking one argument
   — the so called *[combiner](#4-modules)*, which gets passed the list of
   [accessors](#2-accessors) to be used. In order to use the same
   [accessors](#2-accessors) for all enhancements, use the templated version:

```c++
template<class C> void enhance(C& c) const{
  c( <<AccessorList>> );
}
```

to use different [accessor](#2-accessors) lists for different enhancements, you can
add specializations. The following example, gives a different list to
all comparison operators:

```c++
template<class Op> 
void enhance(Comparison<Op,ThisClass>& c) const{
  c( <<AccessorList2>> );
}
```

And another list of [accessors](#2-accessors) for the `operator!=`:

```c++
void enhance(Unequal<ThisClass>& c) const{
  c( <<AccessorList3>> );
}
```

## 1.2 The non-intrusive way using a custom accessor list

Instead of inheriting predefined operators, you can implement your own
operators, free functions or any other kind of functionality using the
[combiners](#4-modules) directly.

The following works with every [combiner](#4-modules). For demonstration purposes,
we use the class `Point2D` from above and the `LessPW` [combiner](#4-modules), which
gives the result of a pointwise comparison the selected fields of two
objects:

```c++
// calling of `operator()` with an explicit accessor list:
bool result = LessPW<Point2D>(obj1, obj2)( <<AccessorList>> );
```

For every [combiner](#4-modules), there is a factory function, that automatically
deduces the template parameters:

```c++
// calling of `operator()` with an explicit accessor list:
bool result = lessPW(obj1, obj2)( <<AccessorList>> );
```

*Implementation details:* `operator()` is implemented in `struct
Combiner`.

## 1.3 Make combiners use the `enhance` member function

[Combiners](#4-modules) can also use a class's `enhance` function (as
described [above](#11-the-simple-and-intrusive-way)):

```c++
// explicitly:
bool result = lessPW(obj1, obj2).callEnhance();

// or implicitly using the provided conversion operator:
bool result = lessPW(obj1, obj2);

// or using the functor:
bool result = LessPW<Point2D>::Functor(obj1, obj2);
```

The `Functor` has its use, e.g. for `std::set` with
[lexical comparison combiner `Less`](#lexicographical-operators) or
for `std::unordered_set` with the [`Hash` combiner](#44Hashing).

```c++
// Both types will use the accessor list defined in `Point2D::enhance`

typedef std::set<Point2D, Less<Point2D>::Functor> Point2DSet;

typedef std::unordered_set<Point2D, Hash<Point2D>::Functor> Point2DHashset;
```

*Implementation details:* `callEnhance`, conversion operator and
`Functor` are all implemented in `struct Combiner`.


# 2 Accessors

*Enhance* extracts a collection of values or features from one or more
objects and combines them in some meaningful way (e.g. to calculate
the result of `operator<`). How this collection is to be extracted, is
described by a list of **Accessors**, which can be any of the
following things.

*Implementation details:* The action of accessors is defined in the
`access` function.

## 2.1 Data member (member variable) accessor

Plain data members are accessed using "pointer to members":

```c++
&Point2D::x
```

provides access (as reference if needed) to the `x` field of an object
of type `Point2D`.

## 2.2 Member function accessor

Another possibility are pointer to member functions with *zero arguments*:

```c++
&Point2D::get_quadrant
```

provides access to the result of `int Point2D::get_quadrant()`.

In order to derive enhancements that work on `const` objects, make
sure, that the member function works on `const` objects, i.e. declare
it like this: `int Point2D::get_quadrant() const`.

## 2.3 Function and functor accessors

More flexbility can be obtained through the use of functors and
functions. Any function of one argument and any object with an
`operator()` taking one argument, including lambda expressions, can be
used as an accessor. Examples:

```c++
// free (template) functions
template<class R> int quadrant(const R&){
  ...
  return 2;
}

// functors
struct IsInQuadrant {
  int i;
  
  bool operator()(const Point2D&){
    ..
    return false;
  }
};

// lambda expressions
c(quadrant<Point2D>, IsInQuadrant({3}), [](const Point2D& p){return p.x + p.y;});

```

# 3 Accessor Modifiers

There a couple of modifiers, that take an accessor and change its
action.

*Implementation details:* Modifiers are either implemented as functors
that work transparently or using wrapper classes that require special
handling like `range`.

## 3.1 Dynamic Ranges

In order to access the values of an iterable container, use the
`range(from, to)` modifier, taking two arguments `from` and `to`. All
elements between `from` and (excluding) `to` will be accessed in
order. Example:

```c++
struct Vector : Insertable<'<',',',' ','>',Vector,false>{
  int data[3];

  Vector():data{4,9,2}{}
    
  template<class C> void enhance(C& c) const{
    c(range(&Vector::data,
            [](const Vector& v){ return v.data+3;}));
  }
};

...

cout<< Vector()<<endl;
// prints: <4, 9, 2>
```

## 3.2 Containers

For containers with `std::begin` and `std::end` implementations, there
exist the following modifiers:

* `begin(accessor)` will evaluate to `std::begin` of the accessor

* `end(accessor)` will evaluate to `std::end` of the accessor

* `container(accessor)` is a shortcut for `range(begin(accessor), end(accessor))`

## 3.3 Static tuple-like ranges

For objects implementing the compile-time access function `std::get`
(like `std::tuples`, `std::arrays`, `std::pair`, etc.), there is
`range<from,to>(accessor)`. `from` defaults to `0` and `to` to
`std::tuple_size` of the accessor:


```c++
struct R2 : Insertable<'{', ',', ' ','}',R2> {
  std::tuple<int,double,char> m;

  R2() : m({42, 1.1, 'g'}) {}

  template<class C> void enhance(C& c) const{
    c(range<1,2>(&R2::m),
      range<>(&R2::m));
  }
};

...

cout<<R2()<<endl;
//prints: {{1.1}, {42, 1.1, g}}
```

## 3.4 Dereference

To access the value pointed to by a pointer, use `dereference(ac)`,
which will call `operator*` on `ac`'s result.

```c++
struct R3 : Insertable<'{', ',', ' ','}',R3> {
  std::shared_ptr<int> i;
  int* j;

  R3() : i(std::make_shared<int>(3)), j(new int(4)) {}

  template<class C> void enhance(C& c) const{
    c(&R3::i, dereference(&R3::i), dereference(&R3::j));
  }
};

...

cout<< R3() <<endl;
//prints: {0x<ADDRESS>, 3, 4}
```

## 3.5 Remove `const`

In order get a non-`const` reference to a `const` value, use
`constCast(ac)`, which will call `const_cast<...>` on the `ac`'s
result.

This can be useful, e.g. in serialization, where an object with
`const` member has to be restored from its serialized value, which is
also recommended by the
[Boost.Serialization](http://www.boost.org/doc/libs/1_62_0/libs/serialization/doc/serialization.html#const)
manual.


# 4 Modules

## 4.1 Comparison Operators

This module defines point-wise and lexical comparison combiners and
provides classes for operator inheritance. The terminology is adopted
from
[std::tuple](http://en.cppreference.com/w/cpp/utility/tuple/operator_cmp).

*Point-wise comparison* returns only true, if comparison of all pairs
of accessors return true.

*Lexicographic comparison* returns the result of the underlying
comparison operator applied to the first pair of accessors that is not
`==`.

All comparison operators are *short-circuited*; they do not use
accessors beyond what is necessary to determine the result of the
comparison.

The underlying comparisons are performed using `std::equal_to`,
`std::not_equal_to`, `std::greater`, `std::less`, `std::greater_equal`
and `std::less_equal`.

All constructors, factory functions and functors take two
`const`references to the objects to be compared:

```c++
(const T&, const T&)
```

The most general combiner (useful for `enhance` specialization,
[see above](#11-the-simple-and-intrusive-way)) is:

| Combiner | 
|---|
| `Comparison<Op, T>` |

`T` stands for the type, on which the comparison should be performed.

In addition to deriving comparison operators for classes, it would be
no problem to provide macros to derive non-member function operators
(i.e. `bool operator==(Class, Class)`) or specializations of
`std::less`, etc. Happy to merge pull requests, or please open an
issue if interested.

*Implementation details:* All combiners are type aliases of
`BinaryCombiner<Op, const Target>`, where `Op` is an instantiation of
either `PointwiseComparisonOp` or `LexicographicalComparisonOp`.


### Point-wise Operators

| Combiner | Factory | Inheritable |
|---|---|---|
| `ComparisonPW<Op, T>` | | |
| `Equal<T>` | `equal` | `EqualComparable<T>::operator==` |
| `Unequal<T>` | `unequal` | `UnequalComparable<T>::operator!=` |
| `LessPW<T>` | `lessPW` | `LessComparablePW<T>::operator<` |
| `GreaterPW<T>` | `greaterPW` | `GreaterComparablePW<T>::operator>` |
| `LessEqualPW<T>` | `lessPW` | `LessEqualComparablePW<T>::operator<=` |
| `GreaterEqualPW<T>` | `greaterPW` | `GreaterEqualComparablePW<T>::operator>=` |

### Lexicographical Operators

| Combiner | Factory | Inheritable |
|---|---|---|
| `ComparisonLEX<Op, T>` | | |
| `Unequal<T>` | `unequal` | `UnequalComparable<T>::operator!=` |
| `Less<T>` | `less` | `LessComparable<T>::operator<` |
| `Greater<T>` | `greater` | `GreaterComparable<T>::operator>` |
| `LessEqual<T>` | `less` | `LessEqualComparable<T>::operator<=` |
| `GreaterEqual<T>` | `greater` | `GreaterEqualComparable<T>::operator>=` |

## 4.2 Arithmetic Operators

For examples see [Readme](README.md).

There is currently no general combiner, that can be used in `enhance`
specialization. Open an issue, if this turns into a problem for you.

### Component-wise (vector) addition & subtraction

All constructors, factory functions and functors take one reference of
the object that will contain the result of adding (subtracting) the
second argument passed by `const` reference:

```c++
(T&, const T&)
```

| Combiner | Factory | Inheritable |
|---|---|---|
| `ArithmeticComponentwise<Op, T>` | | |
| `Addition<T>` | `addition` |  `Addible<T>::operator+=` |
| `Subtraction<T>` | `subtraction` |  `Subtractable<T>::operator-=` |


```c++
  typedef std::pair<int,double> T;
  T p(1, 4.4);
  addition(p, {2, 5.5})(&T::first, &T::second);
  cout<< p << endl;
  //prints: (3, 9.9)
```

### Scalar (dot-, or inner) product 

All constructors, factory functions and functors take two `const`
references to the objects to be multiplied:

```c++
(const T&, const T&)
```

| Combiner | Factory | Inheritable |
|---|---|---|
| `ScalarProduct<Scalar, T>` | `scalarProduct` | `WithScalarProduct::operator*` |

```c++
  Point2D v{2,3}, w{20,30};
  cout<< ScalarProduct<double, Point2D>::Functor()(v,w) << endl;
  //prints: 130
```

### Scalar multiplication & division


The combiner's constructor and factory functions take one reference to
the object to be multiplied by the scalar passed by (templated and
thus collapsable) rvalue reference.

```c++
(T&, Scalar&&)
```

| Combiner | Factory | Inheritable |
|---|---|---|
| `ArithmeticScalar<Op, T>` | | |
| `ScalarMultiply<Scalar, T>` | `scalarMultiply` |  `scalarMultiply<Scalar, T>::operator*=` |
| `ScalarDivide<Scalar, T>` | `scalarDivide` |  `scalarDivide<Scalar, T>::operator/=` |

There is no functor provided for these combiners. Open an issue, if
you need it.

## 4.3 Constructors & Assignment Operators

The combiner's constructor, factory functions and functor take one
reference of the object that will contain the result of copying the
second argument passed by `const` reference:

```c++
(T&, const T&)
```

| Combiner | Factory | MACROS |
|---|---|---|
| `Copy<T>` | `copy` | `ENHANCE_COPY_CONSTRUCTOR(QUALIFIER, T)` <br> `ENHANCE_COPY_ASSIGMENT(QUALIFIER, T)`   |

Assignment operators and constructors cannot be inherited in a way hat
overwrites the implicit default implementations, so they have to be
given explicitely. This can be done using the `ENHANCE_COPY_ASSIGMENT`
and `ENHANCE_COPY_CONSTRUCTOR` Macros, whose first argument can either
be empty, 'const', or any other valid qualifier.

```c++
struct CA {
  int i, j; 
  
  CA(int i, int j): i(i), j(j) {}

  template<class C> void enhance(C& c) const{
    c(&CA::i);
  }
  
  ENHANCE_COPY_ASSIGMENT(,CA)
  
  ENHANCE_COPY_CONSTRUCTOR(,CA)
};

...

CA p1(4, 12), p2(p1), p3 = p2;
```

## 4.4 Hashing


The combiner's constructor, factory functions and functor take one `const`
reference of the object that will be  hashed:

```c++
(const T&)
```

| Combiner | Factory | MACROS |
|---|---|---|
| `Hash<T, Hasher = std::hash, HashCombiner = DefaultHashCombiner>` | `hash` | `ENHANCE_STD_HASH(T)`   |

To specialize `std::hash` for a given type `T`, place
`ENHANCE_STD_HASH(T)` outside of any namespace.

`enhance::DefaultHashCombiner` is taken from
[CityHash's](https://github.com/google/cityhash/) `Hash128to64`.

Example:

```c++
struct Person : EqualComparable<Person> {
  string name;
  uint age;

  Person(string name, uint age): name(name), age(age) {}

  template<class C> void enhance(C& c) const{
    c(&Person::name, &Person::age);
  }
};

ENHANCE_STD_HASH(Person)

TEST_CASE( "hash" ) {
  Person p1{"Frank", 21}, p2{"Frank", 21}, p3{"Frank", 22};

  std::unordered_set<Person> mt2{p1, p2, p3};

  REQUIRE( mt2.size() == 2);
  
  REQUIRE( hash(p1) == hash(p2) );
  REQUIRE( hash(p1) == std::hash<Person>()(p2) );
  REQUIRE( hash(p1) != hash(p3) );
}
```

## 4.5 Serialization

This is module adopts the conventions from the
[Boost.Serialization](http://www.boost.org/doc/libs/1_62_0/libs/serialization/doc/index.html)
library. The `Serializable` class defines a `serialize` function that
when passed a *saving* (*loading*) archive `ar` applies `ar <<` (or `ar >>`)
iteratively to all accessors.

The combiner's constructor and factory functions take one reference of
the object that will be serialized (loaded/saved) and a reference of
the corresponding archive:

```c++
(T&, Archive&)
```

| Combiner | Factory | MACROS |
|---|---|---|
| `Serialize<Archive, T>` | `serialize` | `Serializable<T>::serialize(Archive, uint)`   |
| `Load<Archive, T>` |  |    |
| `Save<Archive, T>` |  |    |

Example:

```c++

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

struct P : Serializable<P>, EqualComparable<P> {
  int i, j;

  P(int i,int j):i(i),j(j){}

  template<class C>
  void enhance(C& t) const{
    t(&P::i, &P::j);
  }
};

P p(123, 789), q(2, 45);

boost::archive::text_oarchive oa(ss);
oa << p;

boost::archive::text_iarchive ia(ss);
ia >> q;

REQUIRE(p == q);
```

## 4.6 String conversion / Stream injection / Pretty printing

The combiner's constructor and factory functions take one `const`
reference of the object that will be inserted and a reference of the
corresponding output stream:

```c++
(const T&, std::ostream&)
```

| Combiner | Factory | Inheritable |
|---|---|---|
| `Insertion<d1,s1,s2,d2, Group, T>` | `insertion` | `Insertable<d1,s1,s2,d2, T, Group>::`<br>`operator<<(std::ostream&, const T&)`   |
  
The result will be enclosed in `d1` and `d2` and the accessors'
results separated by `s1s2`, all of which are of type `char`.

`bool Group` specifies, whether [`range` accessors](#3-accessors) are
themselves enclosed in `d1` and `d2`.
  
Sometimes, it is easier to simply get a string with the following
helper function:

```c++
template<class X>
string toString(X&& x){
  std::ostringstream s;
  s << std::forward<X>(x);
  return s.str();
}
```
