#ifndef FORMULA_EXPRESSION_H
#define FORMULA_EXPRESSION_H

#include <boost/shared_ptr.hpp>
#include "formula.H"
#include <iostream>

class formula_expression_ref
{
  expression_ref R;
public:
  polymorphic_cow_ptr<Formula> notes;

  formula_expression_ref operator()(const formula_expression_ref&) const;

  formula_expression_ref operator()(const expression_ref&) const;

  expression_ref exp() const {return R;}

  int add_expression(const formula_expression_ref&);

  boost::shared_ptr<const Object> result() const;

  template <typename T>
  boost::shared_ptr<const T> result_as() const {return boost::dynamic_pointer_cast<const T>(result());}

  formula_expression_ref();
  formula_expression_ref(const expression_ref&);
  formula_expression_ref(const boost::shared_ptr<const Formula>& F, const expression_ref&);
};

formula_expression_ref prefix_formula(const std::string&,const formula_expression_ref&);

std::ostream& operator<<(std::ostream&, const formula_expression_ref& F);

template <typename T> struct Bounds;

formula_expression_ref def_parameter(const std::string& name, const expression_ref& def_value, const Bounds<double>& b);
formula_expression_ref def_parameter(const std::string& name, const expression_ref& def_value, const Bounds<double>& b, const expression_ref&);
formula_expression_ref def_parameter(const std::string& name, const expression_ref& def_value, const Bounds<double>& b, const expression_ref&, const expression_ref&);
#endif
