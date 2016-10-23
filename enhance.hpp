/*
 *  Enhance v0.1
 *
 *  ----------------------------------------------------------
 *  Copyright (c) 2016 Johannes Gerer.
 *
 *  Distributed under the MIT License. (See accompanying file
 *  LICENSE.txt)
 * 
 */
#ifndef ENHANCE_INCLUDED
#define ENHANCE_INCLUDED

#include <functional>
#include <type_traits>
#include <ostream>
#include <iostream>
using std::cout;
using std::endl;


#ifndef FORCE_INLINE

#ifdef _MSC_FULL_VER
#define FORCE_INLINE __forceinline
#else // _MSC_FULL_VER
#define FORCE_INLINE inline  __attribute__((always_inline))
#endif // _MSC_FULL_VER

#endif // FORCE_INLINE

#if defined(_MSC_VER) && _MSC_VER < 1800
#error "This version of Visual C++ does not support `template aliases` used by Enhance`. Either use Visual C++ 2013 or later or contact the maintainer of `Enhance`, who will be happy to backport to your version."
#endif

//todo assigment mit allen accessors testen

//todo std::swap specialization

//todo std::less specialization? (oder reicht normale instanz die den
//< operator nutzt?

//todo rvalue in constructor of Combiner

//todo base class access?

namespace enhance {

  struct Nothing{};
  
    //#################### 1 the `access` function ############################
    /*

      The `access` function is used to build generic
      `Combiner::singleStep` functions used derive e.g. generic
      comparison operators. They always takes two arguments:
      `Accessor` and `Target`.

      The accessor is used to access some value (like a data member or
      the return value of a member function) from a target.

     */
    
    //`access` for plain data-members
    template<class Accessor,class Target>
    FORCE_INLINE  Accessor&
    access(Accessor( Target::*a), Target& x)
    {
      return x.*a;
    }
  
    //`access` for const plain data-members
    template<class Accessor,class Target>
    FORCE_INLINE const Accessor&
    access(const Accessor(Target::*a), const Target& x)
    {
      return x.*a;
    }
    
    /* `access` for member functions
    */
    template<class Accessor,class Target>
    FORCE_INLINE Accessor
    access(Accessor(Target::*a)(), Target& x)
    {
      return (x.*a)();
    }

     
    /* `access` for member function of constant objects

       member functions need to be `const`, for example:

       class A{
         int version;
         int getVersion() const{
         return version; }
       };
       
    */
    template<class Accessor,class Target>
    FORCE_INLINE Accessor
    access(Accessor(Target::*a)() const, const Target& x)
    {
      return (x.*a)();
    }
    
    //`access` for operators (such as lambda expressions, functions,
    //or classes with ()-operators) acting on the target
    template<class Accessor,class Target>
    FORCE_INLINE auto
    access(Accessor a, Target& x)
      -> decltype(a(x)){
      return a(x);
    }
    
    //#################### 2 Accessor Wrappers ############################
    /*
      Wrapper functions, that modify accessors. (E.g. to derefence
      pointer-Accessors)
     */
    
    //#################### 2.1 Dereference Wrapper ############################
    /** The `dereference` wrapper modifies the Accessor such, that its
        result will be dereferenced. Usefull for e.g. for (smart)
        pointers
        
        usage:

        class A{... 
          int* p;
        }
        
        dereference(&A::p)
     */
    template<class Accessor>
    struct Dereference{

      Accessor m;

      Dereference(Accessor m):m(m){};

      template<class Target>
      auto operator()(Target& d) const
        ->decltype(*access(m, d)){
        //there are no pointers to references, so reference qualifiers
        //are not important here
        return *access(m, d);
      }
    };
    
    // factory function for template argument deduction
    template<class Accessor>
    Dereference<Accessor> dereference(Accessor a){
      return Dereference<Accessor>(a);
    }

    //#################### 2.2 const_cast Wrapper ############################
    /** The `constCast` wrapper modifies the Accessor such, that its
        result will be not be const. This MIGHT be usefull for
        e.g. for serialization of const members.
        
        usage:

        class A{... 
          const int p;
        }
        
        constCast(&A::p)
     */
    template<class Accessor>
    struct ConstCast{

      Accessor m;
      
      ConstCast(Accessor m):m(m){};

      template<class Target>
      auto operator()(Target& d) const
        -> typename std::remove_const<
          typename std::remove_reference<decltype((access(m,d)))>::type>::type&
      {
        return const_cast<typename std::remove_const<
          typename std::remove_reference<decltype((access(m,d)))>::type>::type&>
          (access(m, d));
      }
    };
    
    // factory function for template argument deduction
    template<class Accessor>
    ConstCast<Accessor> constCast(Accessor a){
      return ConstCast<Accessor>(a);
    }

    //#################### 2.3 Range Wrapper ############################
    /** The `range` wrapper defined below function takes two accessors
        `begin` and `end`. All elements between `begin` and
        (excluding) `end` will be accessed in order. 
        
        usage:
        
        class Vector{ ...
           double* data;
           size_t size;
        }

        range(&Vector::data,
              [&](const Vector& v){return v.data + v.size;})
     */
    template<class A, class B>
    struct Range {
      A a;
      B b;
      Range(A a,B b):a(a),b(b){};
    };
    
  // factory function for template argument deduction
  template<class A, class B>
  FORCE_INLINE Range<A,B> range(A a, B b){
    return Range<A,B>(a, b);
  }

  
    //#################### 2.4 static (compile time)  Range Wrapper ############################

  template<int begin, int end, class B>
  struct FromTo {
    B m;
    FromTo(B m) : m(m) {}

  };

  //helper for default end==-1
  template<int endP, class T>
  struct endHelper{
    static const int value = endP > -1 ? endP :
      std::tuple_size<typename std::remove_reference<T>::type >::value;
  };
  
  // factory function for template argument deduction
  template<int begin=0, int end=-1, class B>
  FromTo<begin, end, B> range(B b){
    return FromTo<begin, end, B>(b);
  }

  


    //#################### 2.5 begin, end, container Wrapper ############################
  /*
       container, Automatically iterates from std::begin to std::end
   */
  template<class Accessor>
    struct Begin{

      Accessor m;
      
      Begin(Accessor m):m(m){};
     
      template<class Target>
      auto  operator()(Target& d) const
        -> decltype(std::begin(access(m, d)))
      {
        return std::begin(access(m, d));
      }
    };

  template<class Accessor>
    struct End{

      Accessor m;
      
      End(Accessor m):m(m){};
     
      template<class Target>
      auto  operator()(Target& d) const
        -> decltype(std::end(access(m, d)))
      {
        return std::end(access(m, d));
      }
    };
  
    // factory functions for template argument deduction
    template<class Accessor>
    Begin<Accessor> begin(Accessor a){
      return Begin<Accessor>(a);
    }

    template<class Accessor>
    End<Accessor> end(Accessor a){
      return End<Accessor>(a);
    }

    // factory function for template argument deduction
    template<class Accessor>
    FORCE_INLINE auto container(Accessor a)
      -> decltype(range(begin(a),end(a)))
    {
      return range(begin(a),end(a));
    }


    //#################### 3 Combiners ############################
    /*
      A `Combiner` iteratively applies a given operator a list of
      values derived from a `Target` (using `Accessors`, see above)
      and combines the results to a single `Result`.
      
      This functionality needs to be supplied by the derived class in
      `Derived::singleStep`. The iteration stops, either if there are
      no Accessors left or if `singleStep` returns `true`.

      The list of accessors can be given in two ways:

      1) They can be passed directly to the variadic ()-operator:

         SomeClass target;
         Combiner comb(target);
         comb(&SomeClass::someMember,&SomeClass::anotherMember);

      2) The call of the ()-operator can be put directly in the target
         class, more precisely `Target::enhance`, which is called by
         `Combiner::callEnhanced()` or less verbose in the conversion
         operator converting the `Combiner` to `Result`.

         `enhance` has to have the following signature:
     
           void Target::enhance(Derived&) const

         When deriving from different Combinors, several identical
         enhance members functions can be combined using a template. E.g:
         
         class SomeClass{...
           template<class T> void enhance(T& t) const{
             t(&SomeClass::someMember,&SomeClass::anotherMember);	
  	       }
         }

    */
    template<class Derived, class Target, class Result>
    class Combiner{
      
    protected:
      Target& target;
      Result result;

    public:
      //ctor
      Combiner(Target& target, Result&& result)
        :target(target), result(std::forward<Result>(result)){
        static_cast<Derived&>(*this).initialize();
      }

      //call the target's `enhance` member, which should call the
      //()-operator of the derived class passed to it.
      FORCE_INLINE Result callEnhance(){
        target.enhance(static_cast<Derived&>(*this));
        return result;
      }

      // a conversion operator converting to Result. same
      // functionality as `callEnhance` but less verbose,
      FORCE_INLINE operator Result (){
        callEnhance();
        return result;
      }

      // the trivial version of the ()-operator
			FORCE_INLINE Result operator()(){
        static_cast<Derived&>(*this).finalize();
        return result;
      }

      //Check for variadic tempalte support in MS Visual C++
#if !defined(_MSC_VER) || _MSC_VER >= 1800
			template<class Accessor, class ... Rest>
			FORCE_INLINE Result operator()(Accessor a, Rest  ... rest)
			{
        if(!applySingleStep(a))
          operator()(rest ...);
        return result;
      }

#else
      template<class A1>
      FORCE_INLINE Result operator()(A1 a1)
      {   if(!applySingleStep(a1)) operator()(); return result; }
      template<class A1,class A2>
      FORCE_INLINE Result operator()(A1 a1, A2 a2)
      {   if(!applySingleStep(a1)) operator()(a2); return result; }
      template<class A1,class A2,class A3>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3)
      {   if(!applySingleStep(a1)) operator()(a2,a3); return result; }
      template<class A1,class A2,class A3,class A4>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4); return result; }
      template<class A1,class A2,class A3,class A4,class A5>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4,A5 a5)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4,a5); return result; }
      template<class A1,class A2,class A3,class A4,class A5,class A6>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4,A5 a5,A6 a6)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4,a5,a6); return result; }
      template<class A1,class A2,class A3,class A4,class A5,class A6,class A7>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4,a5,a6,a7); return result; }
      template<class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4,a5,a6,a7,a8); return result; }
      template<class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8,class A9>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4,a5,a6,a7,a8,a9); return result; }
      template<class A1,class A2,class A3,class A4,class A5,class A6,class A7,class A8,class A9,class A10>
      FORCE_INLINE Result operator()(A1 a1, A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9,A10 a10)
      {   if(!applySingleStep(a1)) operator()(a2,a3,a4,a5,a6,a7,a8,a9,a10); return result; }
#endif

      typedef Result result_t;

      void initialize() const{
      }
          
      void beforeStep() const{
      }
          
      void finalize() const{
      }
          

    private:
      template<class Accessor>
      FORCE_INLINE bool applySingleStep(Accessor a)
      {
        if(static_cast<Derived&>(*this).singleStep(a)){
          operator()();
          return true;
        }else
          return false;
      }

    };

    //#################### 3.1 Unary Combiner ############################
    /*
      A Combiner for 'unary' operators, i.e. operators, that act on
      one target. Example: Hash functions

      `Operator` is required to have the following static members:
      
        typedef Result Operator::result_t

        static Result Operator::init()

        static bool Operator::apply(Result&, Target&)
                                             
      The iterative application of `Operator::apply` will stop, if it
      returns `true`.

    */
  template<class Operator, class Target, class Derived=std::false_type>
    struct UnaryCombiner : 
    public Combiner<typename std::conditional<std::is_same<Derived, std::false_type>::value,
                                              UnaryCombiner<Operator, Target>, Derived>::type,
                    Target,
                    typename Operator::result_t> {

    typedef typename Operator::result_t result_t;
    typedef typename std::conditional<std::is_same<Derived, std::false_type>::value,
                                              UnaryCombiner<Operator, Target>, Derived>::type
    derived_t;
    
      FORCE_INLINE UnaryCombiner(Target& target)
        : UnaryCombiner::Combiner(target, Operator::init(target)){};

    FORCE_INLINE UnaryCombiner(Target& target, result_t&& result)
      : UnaryCombiner::Combiner(target, std::forward<result_t>(result)){};

      template<class Accessor>
      FORCE_INLINE bool singleStep(Accessor ac) {
        static_cast<derived_t&>(*this).beforeStep();
        return Operator::apply
          (this->result, access(ac,this->target));
      }

      struct Functor{
        typename UnaryCombiner::result_t operator()
        (Target& target) const{
          return UnaryCombiner(target);
        }
      };

      //specialization for `Range` accessors
      template<class A,class B>
      FORCE_INLINE bool singleStep(Range<A,B> ac) {
        // cout<<"ss1"<<endl;
        //copy the begin iterator
        auto   b = access(ac.a,this->target);
        //reference to the end iterator
        auto&& e = access(ac.b,this->target);

        for(; b<e; ++b){
          static_cast<derived_t&>(*this).beforeStep();
          if(Operator::apply(this->result, *b))
            return true;
        }
        return false;
      }

      //Compile Time Range specialization.
			template<int begin, int end, class Accessor>
      FORCE_INLINE bool singleStep(FromTo<begin, end, Accessor> a)
			{
        auto& ref = access(a.m, this->target);
        return recurseRange<begin,endHelper<end,decltype(ref)>::value >(ref);
      }

      private:
      template<int begin, int end, class B>
			FORCE_INLINE typename std::enable_if<begin < end, bool>::type
      recurseRange(B& o)
      {
        static_cast<derived_t&>(*this).beforeStep();
        if(Operator::apply(this->result, std::get<begin>(o))) 
          return true;
        return recurseRange<begin+1,end>(o);
      }

      // Base case
      template<int begin, int end, class B>
			FORCE_INLINE typename std::enable_if<begin >= end, bool>::type
      recurseRange(B& o)
      {
        return false;
      }
          

    };

    //#################### 3.2 Binary Combiner ############################
    /*
      A Combiner for 'binary' operators, i.e. operators, that act on
      two targets. Example: Comparison operators
      
      `Operator` is required to have the following typed and static
      members:
      
        typedef Result Operator::result_t

        static Result Operator::init()

        static bool Operator::apply(Result&, Target&, Target&)
                                             
      The iterative application of `Operator::apply` will stop, if it
      returns `true`.

    */
  template<class Operator, class Target, class Target2 = Target,
           class Derived = std::false_type>
    struct  BinaryCombiner :
    Combiner<typename std::conditional<std::is_same<Derived, std::false_type>::value,
                                       BinaryCombiner<Operator, Target, Target2>,
                                       Derived>::type,
             Target,
             typename Operator::result_t> {
    
    typedef typename Operator::result_t result_t;
    typedef typename std::conditional<std::is_same<Derived, std::false_type>::value,
                                      BinaryCombiner<Operator, Target, Target2>,
                                      Derived>::type
    derived_t;

      Target2& target2;

      FORCE_INLINE BinaryCombiner(Target& target, Target2& target2)
        : BinaryCombiner::Combiner(target, Operator::init(target, target2))
        , target2(target2){};

      FORCE_INLINE BinaryCombiner(Target& target, Target2& target2,
                                  result_t&& result)
        : BinaryCombiner::Combiner(target, std::forward<result_t>(result))
        , target2(target2){};

    template<class Accessor>
      FORCE_INLINE bool singleStep(Accessor ac) {
        static_cast<derived_t&>(*this).beforeStep();
        return Operator::apply
          (this->result, access(ac,this->target),
                         access(ac,this->target2));
      }

      struct Functor{
        typename BinaryCombiner::result_t operator()
        (Target& target, Target2& target2){
          return BinaryCombiner(target, target2);
        }
      };

      //specialization for `Range` accessors
      template<class A,class B>
      FORCE_INLINE bool singleStep(Range<A,B> ac) {
        //copy the begin iterators
        auto   b_x = access(ac.a,this->target);
        auto   b_y = access(ac.a,this->target2);
        //reference to the end iterators
        auto&& e_x = access(ac.b,this->target);
        auto&& e_y = access(ac.b,this->target2);

        //require range of equal lengths (alternative implementations
        // are possible, esp. for operator==, where differing length
        // simply should result in the value `false`.)
        assert(e_x-b_x==e_y-b_y);

        for(; b_x<e_x; ++b_x, ++b_y){
          static_cast<derived_t&>(*this).beforeStep();
          if(Operator::apply(this->result, *b_x, *b_y))
            return true;
        }
        return false;
      }

      //Compile Time Range specialization.
			template<int begin, int end, class Accessor>
      FORCE_INLINE bool singleStep(FromTo<begin, end, Accessor> a)
			{
        auto& ref = access(a.m, this->target);
        auto& ref2 = access(a.m, this->target2);
        return recurseRange<begin,endHelper<end,decltype(ref)>::value >(ref, ref2);
      }

      private:
    template<int begin, int end, class B, class C>
			FORCE_INLINE typename std::enable_if<begin < end, bool>::type
    recurseRange(B& x, C& y)
      {
        static_cast<derived_t&>(*this).beforeStep();
        if(Operator::apply(this->result, std::get<begin>(x), std::get<begin>(y))) 
          return true;
        return recurseRange<begin+1,end>(x,y);
      }

      // Base case
      template<int begin, int end, class B, class C>
			FORCE_INLINE typename std::enable_if<begin >= end, bool>::type
      recurseRange(B&, C&)
      {
        return false;
      }
    };

    //#################### 4 Modules ############################
    //#################### 4.1 Comparison Operators ############################
    /*
      
      This adopts the terminology of std::tuple.

      From http://en.cppreference.com/w/cpp/utility/tuple/operator_cmp:

      Point-wise comparison: Compares every pair of components.
      
      Lexicographic comparison: Returns the result of the underlying
      comparison operator applied to the first pair of components that
      is not `==`.

      All comparison operators are short-circuited; they do not access
      tuple elements beyond what is necessary to determine the result
      of the comparison.

      Specilize e.g. using Comparison, ComparisonPW, ComparisonLEX

     */

  template<class Base>
  struct ComparisonOp : Base {
    typedef bool result_t;
    template<class A, class B>
    static bool init(A&, B&){ return true; }
  };

  /* Template for point-wise comparison 
   */
  template<template<class> class Operator>
  struct PointwiseComparisonOp {
    template<class Value>
    static bool apply(bool& r, Value&& a, Value&& b){
      Operator<Value> op;
      if(op(std::forward<Value>(a),std::forward<Value>(b)))
        return false;
      r = false;
      return true;
    }
  };

  /* Template for lexicographic comparison 
   */
  template<template<class> class Operator>
  struct LexicographicalComparisonOp {
    template<class Value>
    static bool apply(bool& r, Value&& a, Value&& b){
      Operator<Value> op;
      if(a == b) return false;
      r = op(std::forward<Value>(a),std::forward<Value>(b));
      return true;
    }
  };

  // Combiner aliases for the different operator types and single operators
  template<class Operator, class Target>
  using Comparison = BinaryCombiner<
    ComparisonOp<Operator>, const Target>;

  template<template<class> class Operator, class Target>
  using ComparisonPW = Comparison<
    PointwiseComparisonOp<Operator>, Target>;

  template<template<class> class Operator, class Target>
  using ComparisonLEX = Comparison<
    LexicographicalComparisonOp<Operator>, Target>;

  template<class Target>
  using Equal = ComparisonPW<std::equal_to, Target>;

  template<class Target>
  using Unequal = ComparisonPW<std::not_equal_to, Target>;

  template<class Target>
  using Less = ComparisonLEX<std::less, Target>;

  template<class Target>
  using Greater = ComparisonLEX<std::greater, Target>;
  
  template<class Target>
  using LessEqual = ComparisonLEX<std::less_equal, Target>;

  template<class Target>
  using GreaterEqual = ComparisonLEX<std::greater_equal, Target>;

  template<class Target>
  using LessPW = ComparisonPW<std::less, Target>;

  template<class Target>
  using GreaterPW = ComparisonPW<std::greater, Target>;

  template<class Target>
  using LessEqualPW = ComparisonPW<std::less_equal, Target>;

  template<class Target>
  using GreaterEqualPW = ComparisonPW<std::greater_equal, Target>;

  // factory functions for template argument deduction:
  
  template<class Target>
  Equal<const Target> equal(const Target& x,const Target& y){
    return Equal<const Target>(x,y);
  }

  template<class Target>
  Unequal<const Target> unequal(const Target& x,const Target& y){
    return Unequal<const Target>(x,y);
  }

  template<class Target>
  Less<const Target> less(const Target& x,const Target& y){
    return Less<const Target>(x,y);
  }
  
  template<class Target>
  Greater<const Target> greater(const Target& x,const Target& y){
    return Greater<const Target>(x,y);
  }

  template<class Target>
  LessEqualPW<const Target> lessEqualPW(const Target& x,const Target& y){
    return LessEqualPW<const Target>(x,y);
  }
  
  template<class Target>
  GreaterEqualPW<const Target> greaterEqualPW(const Target& x,const Target& y){
    return GreaterEqualPW<const Target>(x,y);
  }
  
  template<class Target>
  LessPW<const Target> lessPW(const Target& x,const Target& y){
    return LessPW<const Target>(x,y);
  }
  
  template<class Target>
  GreaterPW<const Target> greaterPW(const Target& x,const Target& y){
    return GreaterPW<const Target>(x,y);
  }
  
  // base classes for operator inheritance

  template<class Derived>
  struct GreaterEqualPWComparable {
    bool operator>=(const Derived& y) const{
      return greaterEqualPW(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct GreaterEqualComparable {
    bool operator>=(const Derived& y) const{
      return greaterEqual(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct LessEqualPWComparable {
    bool operator<=(const Derived& y) const{
      return lessEqualPW(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct LessEqualComparable {
    bool operator<=(const Derived& y) const{
      return lessEqual(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct GreaterPWComparable {
    bool operator>(const Derived& y) const{
      return greaterPW(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct GreaterComparable {
    bool operator>(const Derived& y) const{
      return greater(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct LessPWComparable {
    bool operator<(const Derived& y) const{
      return lessPW(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct LessComparable {
    bool operator<(const Derived& y) const{
      return less(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct EqualComparable {
    bool operator==(const Derived& y) const{
      return equal(static_cast<const Derived&>(*this), y);
    }
  };

  template<class Derived>
  struct UnequalComparable {
    bool operator!=(const Derived& y) const{
      return unequal(static_cast<const Derived&>(*this), y);
    }
  };
    //#################### 4.2 Arithmetic Operators ############################

    //#################### 4.2.1 componentwise addition and subtraction ############################

  template<class Base>
  struct ArithmeticComponentwiseOp : Base {

    template<class A, class B>
    static Nothing init(A&, B&){ return Nothing(); }
  };

  struct AdditionOp {
    typedef Nothing result_t;

    template<class Value, class Value2>
    static bool apply(Nothing& r, Value& a, Value2&& b){
      a += std::forward<Value2>(b);
      return false;
    }
  };
  
  struct SubtractionOp {
    typedef Nothing result_t;
    
    template<class Value, class Value2>
    static bool apply(Nothing& r, Value& a, Value2&& b){
      a -= std::forward<Value2>(b);
      return false;
    }
  };
  
  // Combiner aliases
  template<class Op, class Target>
  using ArithmeticComponentwise = BinaryCombiner<
    ArithmeticComponentwiseOp<Op>, Target, const Target>;

  template<class Target>
  using Addition = ArithmeticComponentwise<AdditionOp, Target>;

  template<class Target>
  using Subtraction = ArithmeticComponentwise<SubtractionOp, Target>;


  // factory functions for template argument deduction:
  template<class Target>
  Subtraction<Target> subtraction(Target& target1, const Target& target2)
  {
    return Subtraction<Target>(target1, target2);
  }

  template<class Target>
  Addition<Target> addition(Target& target1, const Target& target2)
  {
    return Addition<Target>(target1, target2);
  }


  // base classes for operator inheritance
  template<class Derived>
  struct Addible {
    Derived& operator+=(Derived& y){
      Derived& thisR(static_cast<Derived&>(*this));
      addition(static_cast<Derived&>(*this), y).callEnhance();
      return thisR;
    }
  };

  template<class Derived>
  struct Subtractable {
    Derived& operator-=(Derived& y){
      Derived& thisR(static_cast<Derived&>(*this));
      subtraction(static_cast<Derived&>(*this), y).callEnhance();
      return thisR;
    }
  };

    //###### 4.2.2 scalar (dot-, or inner) product of two objects returning a scalar  #############
  template<class Scalar>
  struct ScalarProductOp {
    typedef Scalar result_t;

    template<class A, class B>
    static result_t init(A&, B&){return 0;}

    template<class Value>
    static bool apply(Scalar& r, Value&& a, Value&& b){
      r += std::forward<Value>(a) * std::forward<Value>(b);
      return false;
    }
  };

  template<class Scalar, class Target>
  using ScalarProduct = BinaryCombiner<ScalarProductOp<Scalar>, Target>;

  template<class Scalar, class Target>
  ScalarProduct<Scalar, Target> scalarProduct(Target& target1, Target& target2)
  {
    return ScalarProduct<Scalar, Target>(target1, target2);
  }

  template<class Scalar, class Derived>
  struct WithScalarProduct {
    Scalar operator*(Derived& y){
      return scalarProduct<Scalar>(static_cast<Derived&>(*this), y);
    }
  };

  //#################### 4.2.3 multiply/divide all fields by a scalar ############################
  template<class Base>
  struct ArithmeticScalarOp : Base {
  };

  template<class Scalar>
  struct ScalarMultiplyOp {
    typedef Scalar result_t;

    template<class Value>
    static bool apply(Scalar& r, Value& a){
      a *= r;
      return false;
    }
  };

  template<class Scalar>
  struct ScalarDivideOp {
    typedef Scalar result_t;

    template<class Value>
    static bool apply(Scalar& r, Value& a){
      a /= r;
      return false;
    }
  };

  template<class Op, class Target>
  using ArithmeticScalar = UnaryCombiner<
    ArithmeticScalarOp<Op>, Target>;

  template<class Scalar, class Target>
  using ScalarMultiply = ArithmeticScalar<
    ScalarMultiplyOp<Scalar>, Target>;

  template<class Scalar, class Target>
  using ScalarDivide = ArithmeticScalar<
    ScalarDivideOp<Scalar>, Target>;

  template<class Scalar, class Target>
  ScalarMultiply<Scalar, Target> scalarMultiply(Target& target,
                                                Scalar&& scalar)
  {
    return ScalarMultiply<Scalar, Target>
      (target, std::forward<Scalar>(scalar));
  }

  template<class Scalar, class Target>
  ScalarDivide<Scalar, Target> scalarDivide(Target& target,
                                            Scalar&& scalar)
  {
    return ScalarDivide<Scalar, Target>
      (target, std::forward<Scalar>(scalar));
  }

  // base classes for operator inheritance
  
  template<class Scalar, class Derived>
  struct ScalarMultiplicable {
    Derived& operator*=(Scalar scalar){
      Derived& thisR(static_cast<Derived&>(*this));
      scalarMultiply(thisR, scalar).callEnhance();
      return thisR;
    }
  };

  template<class Scalar, class Derived>
  struct ScalarDividable {
    Derived& operator/=(Scalar scalar){
      Derived& thisR(static_cast<Derived&>(*this));
      scalarDivide(thisR, scalar).callEnhance();
      return thisR;
    }
  };

    //#################### 4.3 Constructors and Assignment Operators ############################
  /*
    
    Assignment operators and constructors cannot be inherited in a way
    that overwrites the implicit default implementations, so they have
    to be given explicitely. This can be done using the
    ENHANCE_COPY_ASSIGMENT and ENHANCE_COPY_CONSTRUCTOR Macros, whose
    first argument can either be empty, 'const', or any other valid
    qualifier.
    
    U& operator=(const U& y){
      return copy(*this, y)(&U::i, ...);
    }
    
    or:

    U& operator=(const U& y){
      return copy(*this, y)
    }
    

    U(const U& y) {
      copy(*this, y).callEnhance();
    }

   */

  template<class Target>
  struct CopyOp {

    typedef Target& result_t;

    template<class B>
    static result_t init(Target& target, B&){return target;}
    
    template<class Value>
    static bool apply(result_t&, Value& a, const Value& b){
      a=b;
      return false;
    }
  };

  // Combiner alias
  template<class Target>
  using Copy = BinaryCombiner<CopyOp<Target>, Target, const Target>;

  // factory functions for template argument deduction:
  template<class Target>
  Copy<Target> copy(Target& x,const Target& y){
    return Copy<Target>(x,y);
  }

#define ENHANCE_COPY_CONSTRUCTOR(QUALIFIERS, TARGET)  \
  TARGET (QUALIFIERS TARGET& y){                      \
    copy(*this, y).callEnhance();                     \
  }                                                   \
  
#define ENHANCE_COPY_ASSIGMENT(QUALIFIERS, TARGET)    \
  TARGET& operator=(QUALIFIERS TARGET& y){            \
    return copy(*this, y);                            \
  }                                                   \
  
    //#################### 4.4 Hash functionality ############################
  /*
    1) Pass the Hash<U>::Functor to the unordered_set template:
    
        std::unordered_set<U,Hash<U>::Functor> mt;

    2) Specialize std::hash using the macro ENHANCE_STD_HASH(U)

   */

  // default implementation taken from CityHash's Hash128to64
  struct DefaultHashCombiner {
    FORCE_INLINE void operator()(size_t& result, size_t hash){
      /*
        
        Copyright (c) 2011 Google, Inc.
        
        Permission is hereby granted, free of charge, to any person
        obtaining a copy of this software and associated documentation
        files (the "Software"), to deal in the Software without
        restriction, including without limitation the rights to use,
        copy, modify, merge, publish, distribute, sublicense, and/or
        sell copies of the Software, and to permit persons to whom the
        Software is furnished to do so, subject to the following
        conditions:
        
        The above copyright notice and this permission notice shall be
        included in all copies or substantial portions of the Software.
      */
      const size_t kMul = 0x9ddfea08eb382d69ULL;
      size_t& b = result;
      size_t a = (b ^ hash) * kMul;
      a ^= (a >> 47);
      b =  (hash ^ a) * kMul;
      b ^= (b >> 47);
      b *= kMul;
    }
  };
      

  template<template<class> class Hasher, class HashCombiner>
  struct HashOp{
    typedef size_t result_t;

    template<class A>
    static result_t init(A&){return 0;}

    template<class Value>
    static bool apply(result_t& r, const Value& v){
      Hasher<Value> hasher;
      HashCombiner combiner;
      combiner(r, hasher(v));
      return false;
    }
  };

  // Combiner alias
  template<class Target, template<class> class Hasher=std::hash,
           class HashCombiner=DefaultHashCombiner>
  using Hash = UnaryCombiner<HashOp<Hasher, HashCombiner> ,const Target>;


  // factory functions for template argument deduction:
  template<class Target, template<class> class Hasher=std::hash,
           class HashCombiner=DefaultHashCombiner>
  Hash<Target> hash(Target& x){
    return Hash<Target>(x);
  }

// custom specialization of std::hash can be injected in namespace std
#define ENHANCE_STD_HASH(TARGET)                             \
  namespace std{                                             \
    template<> struct hash<TARGET> {                         \
      std::size_t operator()(const TARGET& s) const {        \
        return enhance::hash(s);                             \
      }                                                      \
    };                                                       \
  }
    
    //#################### 4.5 Serialization functionality ############################
  /*
   */

  template<class Archive, class Operator>
  struct SerializeOp : Operator {
    typedef Archive& result_t;
  };

  struct LoadOp {
    template<class Archive, class Value>
    static bool apply(Archive& a, Value& v)
    {
      a >> v;
      return false;
    }
  };

  struct SaveOp {
    template<class Archive, class Value>
    static bool apply(Archive& a, const Value& v)
    {
      a << v;
      return false;
    }
  };

  // Combiner aliases
  template<class Archive, class Operator, class Target>
  using Serialize = UnaryCombiner<SerializeOp<Archive, Operator>,Target>;

  template<class Archive, class Target>
  using Save = Serialize<Archive, SaveOp, Target>;

  template<class Archive, class Target>
  using Load = Serialize<Archive, LoadOp, Target>;

  template<class Archive, class Target>
  using SerializeSplit = Serialize
    <Archive, typename std::conditional<Archive::is_saving::value,
                                        SaveOp, LoadOp>::type,
     Target>;
  
  // factory functions for template argument deduction:
  template<class Archive, class Target>
  SerializeSplit<Archive, Target> serialize
    (Target& x, Archive& ar){
    return SerializeSplit<Archive, Target>(x, ar);
  }

  //base function for `serialize` member function inheritance
  template<class Derived>
  struct Serializable {
    
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
      enhance::serialize(static_cast<Derived&>(*this),
                         ar).callEnhance();
    }
  };

    //############ 4.6 string conversion / pretty printing  functionality ###############
  struct InsertionOp {
    typedef std::ostream& result_t;
    
    template<class Value>
    static bool apply(result_t& r, Value&& a){
      r << a;
      return false;
    }
  };

  template<char delim1, char sep1, char sep2, char delim2, bool Grouping, class Target>
  struct Insertion : UnaryCombiner<InsertionOp,Target,
                                   Insertion<delim1, sep1, sep2, delim2, Grouping, Target>> {

    bool first;
    
    FORCE_INLINE Insertion(Target& target,
                           std::ostream& result)
      : Insertion::UnaryCombiner(target, result)
      , first(true){
      result << delim1;
    };

    void beforeStep(){
      if(!first)
        this->result << sep1 << sep2;
      else
        first = false;
    }

    using Insertion::UnaryCombiner::singleStep;
    
    //specialization for `Range` accessors
			template<int begin, int end, class Accessor>
      FORCE_INLINE bool singleStep(FromTo<begin, end, Accessor> ac){
        return wrap(ac);
    }

    //specialization for `Range` accessors
    template<class A,class B>
    FORCE_INLINE bool singleStep(Range<A,B> ac) {
      return wrap(ac);
    }
    
    template<class Accessor> bool wrap(Accessor ac)
    {
      if(Grouping){
        beforeStep();
        Insertion<delim1, sep1, sep2, delim2, false, Target>
          (this->target, this->result)(ac);
        return false;
      }
      return Insertion::UnaryCombiner::singleStep(ac);
    }

    void finalize()
    {
      this->result << delim2;
    }
  };


  // factory functions for template argument deduction:
  template<char delim1, char sep1, char sep2, char delim2, bool Grouping = true, class Target>
  Insertion<delim1, sep1, sep2, delim2, Grouping, const Target>
  insertion(const Target& target, std::ostream& os){
    return Insertion<delim1, sep1, sep2, delim2, Grouping, const Target>(target, os);
  }

  template<char delim1, char sep1, char sep2, char delim2, class Target, bool Grouping = true>
	struct Insertable{
		FORCE_INLINE friend std::ostream&
    operator<<(std::ostream& os, const Target& x){
			insertion<delim1,sep1,sep2,delim2, Grouping>(x, os).callEnhance();
			return os;
		}
	};

}

#endif // ENHANCE_INCLUDED
