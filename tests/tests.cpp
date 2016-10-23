/*
 *  Enhance v0.1 - Test suite
 *
 *  This file is a badly hacked together, unorganized collection of
 *  tests.
 *
 *  ----------------------------------------------------------
 *  Copyright (c) 2016 Johannes Gerer.
 *
 *  Distributed under the MIT License. (See accompanying file
 *  LICENSE.txt)
 * 
 */
#include "../enhance.hpp"
#include "dependencies/catch.hpp"
#include "dependencies/prettyprint.hpp"

#include <iostream>
#include <vector>
#include <tuple>
#include <array>
#include <utility>
#include <string>
#include <iostream>
#include <set>
#include <unordered_set>

#ifndef ENHANCE_NO_SERIALIZE
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif // ENHANCE_NO_SERIALIZE

using namespace enhance;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::tuple;

template<class X>
string toString(X&& x){
  std::ostringstream s;
  s << std::forward<X>(x);
  return s.str();
}

struct C{
    typedef bool result_t;

  template<class A>
  static bool init(A){
    return false;
  }
  
  template<class T>
  static bool apply(bool& result, const T& t)
  {
    result=true;
    return false;
  }
};
  
struct T : LessComparable<T>,
           GreaterEqualPWComparable<T>,
           EqualComparable<T>,
           UnequalComparable<T> {
  public:
  int i;

  T(int i):i(i){}

  int get_i() const
  {
    return i;
  }
  
  template<class C>
  void enhance(C& t) const{
    t([](const T& asd){ return 123;}, &T::i, &T::get_i);
    // t([](const T& asd){ return 123;});
  }

  // void enhance(Less<T>& t) const{
  //   // t(&T::i);
  //   t([](const T& asd){ return 123;});
  // }
};

struct U : LessPWComparable<U>, GreaterPWComparable<U>,
           EqualComparable<U> {
  int i;
  const int j;

  U(int i,int j):i(i),j(j){}

  template<class C>
  void enhance(C& t) const{
    REQUIRE(false); //should never be called
    t(&U::i, &U::j);
  }

  void enhance(Hash<U>& t) const{
    t(&U::i, &U::j);
  }

  template<class C>
  void enhance(Comparison<C,U>& t) const{
    t(&U::i, &U::j);
  }

  void enhance(Copy<U>& t){
    t(&U::i, constCast(&U::j),
      [](const U& q) -> int& { return const_cast<int&>(q.j);});
  }

  U& operator=(const U& y){
    return copy(*this, y);
  }

  U(const U& y) : j(999){
    copy(*this, y).callEnhance();
  }
};



TEST_CASE( "Assignment operators" ) {
  const U u1(3,8);
  U u2(4,9);
  U u3(u1);
  u2 = u1;

  REQUIRE(u2.i == 3);
  REQUIRE(u2 == u1);
  REQUIRE(u3 == u1);

  u2.i=10;

  REQUIRE(u3 == u1);
  REQUIRE(!(u2 == u1));
  REQUIRE(!(u3 == u2));
  
  REQUIRE(false == u1 < u2);
  REQUIRE(false == u2 > u2);
  REQUIRE(less(u1,u2) == true);
  REQUIRE(lessPW(u1,u2) == false);
}


struct P : Serializable<P>, EqualComparable<P> {
  int i, j;

  P(int i,int j):i(i),j(j){}

  template<class C>
  void enhance(C& t) const{
    t(&P::i, &P::j);
  }
};

struct H : EqualComparable<H> {
  int i, j;
  int& get_i(){
    return i;
  }

  const int& get_const_i() const{
    return i;
  }

  int get_j() const{
    return j;
  }

  H(int i,int j):i(i),j(j){}
  H(){}

  template<class C>
  void enhance(C& t) const{
    t(&H::get_const_i, &H::j, &H::get_j);
  }
};

template<class Archive>
void serialize(Archive& ar, H& a, const unsigned int version){
  enhance::serialize(a,ar)(&H::j, &H::get_i);
}



typedef UnaryCombiner<C, T> asd;

ENHANCE_STD_HASH(T)

TEST_CASE( "Some hashing and comparing" ) {
  T a={9};
  const T a2={19};
  asd b(a);

  auto r = (b(&T::i));

  std::set<U,Greater<P>::Functor> st;
  
  std::unordered_set<U,Hash<U>::Functor> mt={U(234,21)};
  
  std::unordered_set<T> mt2={T{321}};

  REQUIRE ( Hash<T>::Functor()(a) == std::hash<T>()(a) );

  std::set<U,Less<U>::Functor> mt3;
  
  REQUIRE(r == true);
  REQUIRE(1 == 1);
  REQUIRE(less(a2,a)(&T::i) == false);
  REQUIRE(less(a2,a) == false);
  REQUIRE(less(a2,a) == a2 < a);
  REQUIRE(less(a,a2) == a < a2);
  REQUIRE(equal(a,a2) == false);
  REQUIRE(unequal(a,a2) == false);
  REQUIRE(unequal(a,a2) == (a!=a2));
  REQUIRE(unequal(a,a2)(&T::i) == true);
  REQUIRE(less(a,a2) == true);
  REQUIRE(lessPW(a2,a) == false);
  REQUIRE(lessPW(a,a2) == false);
  REQUIRE(greater(a2,a)(&T::i) == true);
  REQUIRE(greater(a2,a) == true);
  REQUIRE(greaterEqualPW(a2,a) == (a2 >= a));

#ifndef ENHANCE_NO_SERIALIZE
  {
    std::stringstream ss;
    P p(123,789), q(2,45);
  
    boost::archive::text_oarchive oa(ss);
    oa << p;
  
    boost::archive::text_iarchive ia(ss);
    ia >> q;
  
    REQUIRE(p == q);
  }

  {
    std::stringstream ss;
    H p(123,789), q;
    boost::archive::text_oarchive oa(ss);
    oa << p;
  
    boost::archive::text_iarchive ia(ss);
    ia >> q;
  
    REQUIRE(p == q);
  }
#endif // ENHANCE_NO_SERIALIZE
}


struct Q : EqualComparable<Q>, Serializable<Q> {
  int i, j;
  const int k;
  int get_i() const{
    return i;
  }

  int& set_i(){
    return i;
  }

  Q(int i,int j, int k):i(i),j(j), k(k){}
  Q():k(149){}

  template<class Archive>
  void enhance(Save<Archive, Q>& t) const{
    t(&Q::get_i, &Q::j, &Q::k);
  }

  template<class Archive>
  void enhance(Load<Archive, Q>& t) const{
    t(&Q::set_i, &Q::j, constCast(&Q::k) );
  }

  template<class C>
  void enhance(C& t) const{
    t(&Q::get_i, &Q::j, &Q::k );
  }
};

#ifndef ENHANCE_NO_SERIALIZE
TEST_CASE( "Serialize" ) {
  std::stringstream ss;
  Q p(123,789,3), q;
  boost::archive::text_oarchive oa(ss);
  oa << p;
 
  boost::archive::text_iarchive ia(ss);
  ia >> q;
 
  REQUIRE(p == q);


}
#endif // ENHANCE_NO_SERIALIZE

struct K : LessPWComparable<K>, GreaterPWComparable<K> {
  int i;
  int* j;

  const int* get_i() const{
    return &i;
  }

  K(int i,int j):i(i),j(new int(j)){}

  template<class C>
  void enhance(C& t) const{
    t(&K::i, dereference(&K::get_i), dereference(&K::j));
  }
};

TEST_CASE( "Dereference" ) {
  REQUIRE(K(10,9) < K(11,12));
  REQUIRE(false == K(10,12) < K(11,12));
  REQUIRE(true  == lessPW(K(10,9),K(11,12)));
  K a(10,9), b(11,12);
  REQUIRE( (&a.i < &b.i) == lessPW(a,b)(&K::get_i));
}

struct W : LessPWComparable<W>, GreaterPWComparable<W>,
           Insertable<'{', ',', ' ','}',W> {
  int i;
  int* j;
  int k[3];
  vector<int> l;

  const int* get_i() const{
    return &i;
  }

  W(int i,int j)
    :i(i)
    ,j(new int(j))
    ,k{9,5,1}
    ,l({0,1,0}){
  }

  template<class C>
  void enhance(C& t) const{
    t(&W::i, dereference(&W::get_i), dereference(&W::j),
      range(&W::k,[](const W& w){ return w.k+3;})
      , range(begin(&W::l),end(&W::l))
      ,container(&W::l)
      );
  }
};

TEST_CASE( "pretty print" ) {
  W k(843,901);
  vector<int> v={1,4,5};
  tuple<vector<int>,vector<int>,int, string> t{v,v,4, "asd"};
  // cout << v << endl<< t << endl;

  std::ostringstream os, os2;
  insertion<'<', ';', ' ','>'>(k, os).callEnhance();
  REQUIRE(os.str() == "<843; 843; 901; <9; 5; 1>; <0; 1; 0>; <0; 1; 0>>");
  // cout<<os.str()<<endl;

  os2 << k;
  REQUIRE(os2.str() == "{843, 843, 901, {9, 5, 1}, {0, 1, 0}, {0, 1, 0}}");
  // cout<<os2.str()<<endl;
}

//##########   Readme / Basic Usage   #################

struct Point2D : Subtractable<Point2D>,
                   Addible<Point2D>,
                   ScalarDividable<int, Point2D>,
                   ScalarMultiplicable<int, Point2D>,
                   WithScalarProduct<int, Point2D>,
                   Insertable<'[',',',' ',']',Point2D>,
                   EqualComparable<Point2D>,
                   LessComparable<Point2D>
{
  int x,y;

  Point2D(int x, int y):x(x), y(y) {}
  
  template<class C>
  void enhance(C& t) const{
    t(&Point2D::x, &Point2D::y);
  }

  ENHANCE_COPY_ASSIGMENT(, Point2D)

  ENHANCE_COPY_CONSTRUCTOR(const, Point2D)


};

TEST_CASE( "Basic Usage", "[Readme]" ) {
  Point2D v{2,3}, w{2,3}, q{20,30};
  std::set<Point2D> points = {Point2D{4,5}, Point2D{10,0}, v, w, q};
  // cout<< std::boolalpha << (v < w )<< endl;
  REQUIRE( v < w );
  const Point2D k = v;
  Point2D j(k);
  REQUIRE(k == v);
  REQUIRE(k == j);
  REQUIRE(v == w);
  v *= 10;
  REQUIRE(v == q);
  v /= 10;
  REQUIRE(v == w);
  REQUIRE(v * q == 130);
  REQUIRE((v-= w) == Point2D(0,0));
  REQUIRE((v+= q) == q);
}


struct Vector : Insertable<'<',' ',' ','>',Vector,false>{
  int data[3];

  Vector():data{4,9,2}{}
    
  template<class C> void enhance(C& c) const{
    c(range(&Vector::data,
            [](const Vector& v){ return v.data+3;}));
  }
};


TEST_CASE( "5.3.1 iterators and containers", "[Readme]" ) {
  // cout<< Vector()<<endl;
  REQUIRE(toString(Vector()) == "<4  9  2>");
}

template<class R>
const int& function2(const R& r){
   return r.k;
}

struct functor1{
  int i;
  template<class R>
  int operator()(const R& r){
    return r.k + i;
  }
};

struct R;

const int& function(const R& r);

struct R2 : GreaterEqualPWComparable<R2>,
            Insertable<'{', ',', ' ','}',R2> {
  std::tuple<int,double,char> m;
  R2() : m(std::make_tuple(42,1.1,'g')) {}

  template<class C>
  void enhance(C& c) const{
    c(range<1,2>(&R2::m),
      range<>(&R2::m));
  }
};

struct R : LessPWComparable<R>, 
           Insertable<'{', ',', ' ','}',R> {
  std::array<int,2> l;
  std::tuple<int,double> m;
  int k;

  R(std::array<int,2> l, std::tuple<int,double> m)
    :l(l), m(m),k(939){}

  template<class C>
  void enhance(C& t) const{
    t([](const R& p){return std::get<0>(p.m);},
      [](const R& p){return std::get<1>(p.m);},
      range<1,2>(&R::m),
      range<>(&R::m),
      container(&R::l),     
      &R::l,
      dereference(begin(&R::l)),
      functor1({3}),
      function,
      function2<R>
      );
  }
};

const int& function(const R& r){
   return r.k;
}


TEST_CASE( "Tuples and Arrays, and function and functor accessors" ) {
  R l(std::array<int,2>{{4,9}}, std::tuple<int,double>(6,1.2));
  std::ostringstream os;
  os<<l;
  // cout<<(size_t(-1))<<endl;
  REQUIRE(os.str() == "{6, 1.2, {1.2}, {6, 1.2}, {4, 9}, [4, 9], 4, 942, 939, 939}");
  // cout<<os.str()<<endl;
  REQUIRE( toString(R2()) == "{{1.1}, {42, 1.1, g}}");
  REQUIRE( R2() >= R2() );
  R2 a, b;
  std::get<1>(a.m) = 10;
  REQUIRE( less(b,a)(range<>(&R2::m)) );
  REQUIRE( ! lessPW(b,a)(range<>(&R2::m)) );
  REQUIRE( lessPW(b,a)(range<1,2>(&R2::m)) );
  // cout << R2() << endl;
}

struct R3 : Insertable<'{', ',', ' ','}',R3> {
  std::shared_ptr<int> i;
  int* j;

  R3() : i(std::make_shared<int>(3)), j(new int(4)) {}

  template<class C> void enhance(C& c) const{
    c(dereference(&R3::i), dereference(&R3::j));
  }
};

TEST_CASE( "Dereference Documentation" ) {
  // insertion<'[',',',' ',']'>(R3(), cout)(&R3::i);
  REQUIRE( toString(R3()) == "{3, 4}");
}

TEST_CASE( "arithmetic" ) {
  typedef std::pair<int,double> T;
  T p(1,4.4);
  addition(p, {2,5.5})(&T::first, &T::second);
  // cout<< p << endl;
  // Vector addition

  
  // ScalarMultiply<int, Point2D>::Functor()(v); not possible
  Point2D v{2,3}, w{2,3}, q{20,30};
  REQUIRE( ( ScalarProduct<double,Point2D>::Functor()(v,q) ) == 130 );
  const int k = 3;
  scalarDivide(v, k).callEnhance();
  REQUIRE(v == Point2D( 0, 1));
  Subtraction<Point2D>::Functor()(v, w);
  REQUIRE(v == Point2D( -2, -2));
}

struct CA {
  int i, j; 
  
  CA(int i, int j): i(i), j(j) {}

  template<class C> void enhance(C& c) const{
    c(&CA::i);
  }
  
  ENHANCE_COPY_ASSIGMENT(,CA)
  
  ENHANCE_COPY_CONSTRUCTOR(const, CA)
};

TEST_CASE( "copy assignment" ) {
  CA p1(4,12);
  CA const p2(p1);
  CA p3 = p2;
  REQUIRE( equal(p1, p3)(&CA::i) ) ;
  REQUIRE( unequal(p1, p3)(&CA::j) ) ;
}

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
  REQUIRE( hash(p1)(&Person::name) == hash(p3)(&Person::name) );
}
