#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include <valarray>
#include <string>
#include "cow-ptr.H"
#include "object.H"
#include "operation.H"

class Formula;

// However, C++0x/boost functions can also do binding.  How should expressions relate to them?
// How do you bind a 2nd argument in haskell? (currying?)  Can you use match arguments like _?
// Should I replace the match arguments with a (M x)(f x) operator to specify which dummies are for matching?

// Note: I want to be able to construct expressions simply.  For example, I want
// to have an exp(x) expression that does something like exp(Q) = exp_sym(pi(Q),S(Q))

// Note: Expressions can now return multiple values, in an s-expression... I think?

// Note: it is probably necessary for the different parts of a structure to be able to be
// dynamically computed.  A structure is, in some sense, also a function, if structures are
// s-expressions.

// A quote operator would affect how arguments were evaluated.
// - Do I want the question of whether the argument is an expression to affect its evaluation?
// - Perhaps I could evaluate all expression-type arguments.
// - Quoting would stop prevent this, and would take the place of constant(expression(...)).

// Question: Do I really want objects to have names in the Formula?

struct expression_ref;

// Head objects include:
// - parameter(string)
// - dummy(int)
// - match(int)
// - lambda(int)
// - operation
// - function / data
//   + FIXME: should we evaluate these?

struct expression: virtual public Object
{
  // FIXME - replace args with sub, and make sub[0] = head, sub[i] = args[i-1]

  // An expression may or may not have sub-expressions
  std::vector< expression_ref > sub;

  /// The number of terms after the head
  int size() const {return sub.size();}
  
  // Methods from Object
  expression* clone() const {return new expression(*this);}
  tribool compare(const Object& o) const;
  std::string print() const;
  
  expression() {}
  explicit expression(const expression_ref&);
  explicit expression(const expression_ref&, const expression_ref&);
  expression(const std::vector< expression_ref >& V);

  virtual ~expression() {}
};

// a constant expression
struct constant: public Object
{
  boost::shared_ptr<const Object> value;
  constant* clone() const {return new constant(*this);}

  std::string print() const {return value->print();}

  tribool compare(const Object& o) const;
  
  explicit constant(const Object& o): value(o.clone()) {}
  explicit constant(boost::shared_ptr<const Object> v): value(v) {}
};

struct parameter: public Object
{
  std::string parameter_name;

  parameter* clone() const {return new parameter(*this);}
  std::string print() const {return parameter_name;}
  tribool compare(const Object& o) const;
  
  parameter(const std::string& s):parameter_name(s) { }
};

// a dummy variable expression
struct dummy: public Object
{
  int index;

  std::string name;

  dummy* clone() const {return new dummy(*this);}
  std::string print() const;
  tribool compare(const Object& o) const;
  
  bool operator<(const dummy&) const;

  dummy(int i):index(i) {}
  dummy(const std::string& s):index(-1),name(s) {}
};


bool is_dummy(const expression_ref&);

bool is_wildcard(const expression_ref&);

bool is_irrefutable_pattern(const expression_ref& R);

struct match: public Object
{
  int index;

  match* clone() const {return new match(*this);}

  tribool compare(const Object& o) const;
  
  std::string print() const;

  match(int i):index(i) { }
};

struct let_obj: public Object
{
  let_obj* clone() const {return new let_obj(*this);}
  tribool compare(const Object& o) const;

  std::string print() const;
};

struct alt_obj: public Operator
{
  alt_obj* clone() const {return new alt_obj(*this);}
  tribool compare(const Object& o) const;

  std::string name() const;

  int precedence() const {return 0;}

  int n_args() const {return 2;}
};

struct equal_obj: public Operator
{
  equal_obj* clone() const {return new equal_obj(*this);}
  tribool compare(const Object& o) const;

  std::string name() const;

  int precedence() const {return 0;}

  int n_args() const {return 2;}
};

// This no longer needs to be a virtual member function, because we can now walk the expression from outside.
int n_match_results(const expression_ref&);

bool find_match(const expression_ref& pattern,const expression_ref& E, std::vector< expression_ref >& results);

enum function_type_t {data_function_f, body_function_f};

struct Function: public Operator
{
  Function* clone() const {return new Function(*this);}

  tribool compare(const Object& o) const;
  
  std::string f_name;
  int n_args_;

  int n_args() const {return n_args_;}

  function_type_t what_type;

  assoc_type assoc;

  int prec;

  assoc_type associativity() const {return assoc;}
  virtual int precedence() const {return prec;}

  std::string name() const {return f_name;}
  bool is_data_constructor() const {return what_type == data_function_f;}
  bool is_function() const {return what_type == body_function_f;}
  Function(const std::string&, int n, function_type_t);
};

Function data_function(const std::string&, int);

Function left_assoc_data_function(const std::string& s,int prec=0);

Function right_assoc_data_function(const std::string& s,int prec=0);

Function body_function(const std::string&, int);

std::vector<std::string> find_named_parameters(const expression_ref&);

expression_ref substitute(const expression_ref& E1, int dummy, const expression_ref& E2);

expression_ref substitute(const expression_ref& E1, const expression_ref& D, const expression_ref& E2);

/// Substitute for all matches found
expression_ref substitute(const expression_ref& R, const std::vector<expression_ref>& replace);

// Single-arg version  (Is this a function at all?)";
expression_ref apply(const expression_ref& E,const expression_ref& arg);

// Multiple-arg version
expression_ref apply(const expression_ref& E, const std::vector< expression_ref > args);

// Single-arg version  (Is this a function at all?)";
expression_ref apply_expression(const expression_ref& E,const expression_ref& arg);

// Multiple-arg version
expression_ref apply_expression(const expression_ref& E, const std::vector< expression_ref > args);

expression_ref lambda_expression(const Operator& O);

// FIXME - If we're substituting a lambda expression we have to rename its dummys to make sure
//         there is no overlap.
struct lambda: public Object
{
  lambda* clone() const {return new lambda(*this);}
  tribool compare(const Object& o) const;
  std::string print() const;
};

struct term_ref;

struct formula_expression_ref;

struct expression_ref: public polymorphic_cow_ptr<Object>
{
  expression_ref operator()(const expression_ref& arg) const
  {
    return apply(*this,arg);
  }

  formula_expression_ref operator()(const formula_expression_ref& arg1) const;

  formula_expression_ref operator()(const formula_expression_ref& arg1,
				    const formula_expression_ref& arg2) const;

  formula_expression_ref operator()(const formula_expression_ref& arg1,
				    const formula_expression_ref& arg2,
				    const formula_expression_ref& arg3) const;

  formula_expression_ref operator()(const formula_expression_ref& arg1,
				    const formula_expression_ref& arg2,
				    const formula_expression_ref& arg3,
				    const formula_expression_ref& arg4) const;

  expression_ref operator()(const expression_ref& arg1, const expression_ref& arg2) const
  {
    return apply(apply(*this,arg1),arg2);
  }

  expression_ref operator()(const expression_ref& arg1, 
			    const expression_ref& arg2,
			    const expression_ref& arg3) const
  {
    return apply(apply(apply(*this,arg1),arg2),arg3);
  }

  expression_ref operator()(const expression_ref& arg1, 
			    const expression_ref& arg2,
			    const expression_ref& arg3,
			    const expression_ref& arg4) const
  {
    return apply(apply(apply(apply(*this,arg1),arg2),arg3),arg4);
  }

  expression_ref operator()(const std::vector<expression_ref>& args) const
  {
    return apply(*this,args);
  }

  expression_ref() {}

  expression_ref(Object* v)
    :polymorphic_cow_ptr<Object>(v) 
  {}

  expression_ref(const boost::shared_ptr<const Object>& v)
    :polymorphic_cow_ptr<Object>(v) 
  {}

  explicit expression_ref(const boost::shared_ptr<Object>& v)
    :polymorphic_cow_ptr<Object>(v) 
  {}

  template <typename T>
  expression_ref(const boost::shared_ptr<const T>& v)
    :polymorphic_cow_ptr<Object>(boost::static_pointer_cast<const Object>(v)) 
  {}

  expression_ref(const term_ref& t);

  expression_ref(const Object& o)
    :polymorphic_cow_ptr<Object>(o.clone())
  {}

  expression_ref(const polymorphic_cow_ptr<Object>& o)
    :polymorphic_cow_ptr<Object>(o)
  {}

  expression_ref(const int& i):polymorphic_cow_ptr<Object>(new Int(i)) {}
  expression_ref(const double& d):polymorphic_cow_ptr<Object>(new Double(d)) {}
  expression_ref(const bool& b):polymorphic_cow_ptr<Object>(new Bool(b)) {}
  expression_ref(const std::string& s):polymorphic_cow_ptr<Object>(new String(s)) {}
  expression_ref(const char* s):polymorphic_cow_ptr<Object>(new String(s)) {}
};

template <typename T>
struct typed_expression_ref: public expression_ref
{
public:
  typed_expression_ref() {}
  typed_expression_ref(const expression_ref& e):expression_ref(e) {}
  typed_expression_ref(expression* v): expression_ref(v) {}
  typed_expression_ref(const boost::shared_ptr<const Object>& v): expression_ref(v) {}
  typed_expression_ref(const term_ref& t): expression_ref(t) {}
  typed_expression_ref(const Object& o):expression_ref(o) {}
};

template <typename T>
struct typed_expression_ref<POD<T> >: public expression_ref
{
public:
  typed_expression_ref() {}
  typed_expression_ref(const expression_ref& e):expression_ref(e) {}
  typed_expression_ref(expression* v): expression_ref(v) {}
  typed_expression_ref(const boost::shared_ptr<const Object>& v): expression_ref(v) {}
  typed_expression_ref(const term_ref& t): expression_ref(t) {}
  typed_expression_ref(const Object& o):expression_ref(o) {}
  typed_expression_ref(const T& t):expression_ref(POD<T>(t)) {}
};

template <typename T>
struct typed_expression_ref<POD_math<T> >: public expression_ref
{
public:
  typed_expression_ref() {}
  typed_expression_ref(const expression_ref& e):expression_ref(e) {}
  typed_expression_ref(expression* v): expression_ref(v) {}
  typed_expression_ref(const boost::shared_ptr<const Object>& v): expression_ref(v) {}
  typed_expression_ref(const term_ref& t): expression_ref(t) {}
  typed_expression_ref(const Object& o):expression_ref(o) {}
  typed_expression_ref(const T& t):expression_ref(POD_math<T>(t)) {}
};

inline typed_expression_ref<Double> Constant(const double& d)
{
  return typed_expression_ref<Double>(Double(d));
}

inline typed_expression_ref<Int> Constant(const int& i)
{
  return typed_expression_ref<Int>(Int(i));
}

inline typed_expression_ref<Bool> Constant(const bool& b)
{
  return typed_expression_ref<Bool>(Bool(b));
}

#ifdef STDCXXOX
// create an Op automatically from a function... using E(?)
// how does boost::bind work? boost::spirit?
#endif

class Context;

expression_ref eval(const Context& C, const expression_ref& R);

bool eval_match(const Context& C, expression_ref& R, const expression_ref& Q, std::vector<expression_ref>& results,bool=false);

extern expression_ref _;
extern expression_ref _1;
extern expression_ref _2;
extern expression_ref _3;
extern expression_ref _4;

expression_ref Tuple(int);
expression_ref Tuple(const expression_ref&, const expression_ref&);
expression_ref Tuple(const expression_ref&, const expression_ref&, const expression_ref&);
expression_ref Tuple(const expression_ref&, const expression_ref&, const expression_ref&, const expression_ref&);
expression_ref Tuple(const expression_ref&, const expression_ref&, const expression_ref&, const expression_ref&, const expression_ref&);

// 2 Fields, a value, and a Cons or a ListEnd
extern expression_ref Cons;

// 0 fields
extern expression_ref ListEnd;

// 2 Fields: parameter, value
extern expression_ref default_value;

// 2 Fields: parameter, Bounds<double>
extern expression_ref bounds;

// 2 Fields: string, density op
extern expression_ref prob_density;

// 2 Fields: (random vars) (prob_density (parameter expressions))
extern expression_ref distributed;

// 1 Field: a value
extern expression_ref sys_print;

// 2 Fields: string1 string2
extern expression_ref concat;

// 1 Fields: the density expression
extern expression_ref prob;

// 3 fields: condition body1 body2
extern expression_ref If;

// 3 Fields: definition, guard, body
extern expression_ref defun;

template <typename T,typename U>
std::valarray<T> get_varray(const expression_ref& R)
{
  if (not R)
    return std::valarray<T>();

  boost::shared_ptr<const expression> E = dynamic_pointer_cast<expression>(R);

  if (not E)
  {
    std::valarray<T> v2(1);
    v2[0] = *dynamic_cast<const U*>(&*R);
    return v2;
  }

  std::valarray<T> v2(E->size()-1);
  for(int i=0;i<v2.size();i++)
    v2[i] = *dynamic_cast<const U*>(&*E->sub[i+1]);
  return v2;
}

template <typename T,typename U>
std::vector<T> get_vector(const expression_ref& R) 
{
  if (not R)
    return std::vector<T>();

  boost::shared_ptr<const expression> E = dynamic_pointer_cast<expression>(R);

  if (not E)
  {
    std::vector<T> v2(1);
    v2[0] = *dynamic_cast<const U*>(&*R);
    return v2;
  }
    
  std::vector<T> v2(E->size()-1);
  for(int i=0;i<v2.size();i++)
    v2[i] = *dynamic_cast<const U*>(&*E->sub[i+1]);
  return v2;
}

std::vector<expression_ref> get_ref_vector_from_list(const expression_ref& R);

std::vector<expression_ref> get_ref_vector_from_tuple(const expression_ref& R);

template <typename T>
expression_ref get_tuple(const std::vector<T>& v)
{
  std::vector<expression_ref> v2;
  for(int i=0;i<v.size();i++)
    v2.push_back(v[i]);

  return get_tuple(v2);
}

template<> expression_ref get_tuple<>(const std::vector<expression_ref>& v);

template <typename T>
expression_ref get_tuple(const std::valarray<T>& v)
{
  std::vector<expression_ref> v2;
  for(int i=0;i<v.size();i++)
    v2.push_back(v[i]);

  return get_tuple(v2);
}

expression_ref get_list(const std::vector<expression_ref>& v);

expression_ref let_expression(const std::vector<expression_ref>& vars, const std::vector<expression_ref>& bodies, const expression_ref& T);

expression_ref let_expression(const expression_ref& var, const expression_ref& body, const expression_ref& T);

expression_ref case_expression(bool decompose, const expression_ref& T, const expression_ref& pattern, const expression_ref& body, const expression_ref& otherwise = expression_ref());

expression_ref case_expression(bool decompose, const expression_ref& T, const std::vector<expression_ref>& patterns, const std::vector<expression_ref>& bodies);

expression_ref multi_case_expression(bool decompose, const std::vector<expression_ref>& terms, const std::vector<expression_ref>& patterns, 
				     const expression_ref& body, const expression_ref& otherwise = expression_ref());

expression_ref def_function(bool decompose, const expression_ref& pattern, const expression_ref& body, const expression_ref& otherwise = expression_ref());

expression_ref def_function(bool decompose, const std::vector<expression_ref>& patterns, const std::vector<expression_ref>& bodies, const expression_ref& otherwise = expression_ref());

expression_ref launchbury_normalize(const expression_ref& R);
expression_ref launchbury_unnormalize(const expression_ref& R);

expression_ref evaluate_mark1(const expression_ref& R);

expression_ref add_prefix(const std::string&, const expression_ref& R);

bool parse_let_expression(const expression_ref& R, std::vector<expression_ref>& vars, std::vector<expression_ref>& bodies, expression_ref& T);

void parse_alternatives(const expression_ref& R, std::vector<expression_ref>& cases, std::vector<expression_ref>& results);

bool is_WHNF(const expression_ref& R);

int get_safe_binder_index(const expression_ref& R);
#endif
