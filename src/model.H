/*
   Copyright (C) 2004-2006,2009-2010 Benjamin Redelings

This file is part of BAli-Phy.

BAli-Phy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

BAli-Phy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with BAli-Phy; see the file COPYING.  If not see
<http://www.gnu.org/licenses/>.  */

///
/// \file model.H
///
/// \brief This file defines a generic Model class.
///


#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <iostream>

#include "clone.H"
#include "mytypes.H"
#include "bounds.H"
#include "cow-ptr.H"
#include "object.H"
#include "operator.H"
#include "operation.H"
#include "computation.H"
#include "context.H"
#include "formula.H"

class Model;

std::string parameter_name(const std::string& prefix,int i,int n);

int find_parameter(const Model& M,const std::string& name);

struct Parameter
{
  std::string name;
  polymorphic_cow_ptr<Object> value;
  Bounds<double> bounds;
  bool fixed;

  Parameter(const std::string&);
  Parameter(const std::string&, boost::shared_ptr<const Object>);
  Parameter(const std::string&, boost::shared_ptr<const Object>, const Bounds<double>&, bool=false);

  Parameter(const std::string&, const Object&);
  Parameter(const std::string&, const Object&, bool);
  Parameter(const std::string&, const Object&, const Bounds<double>&, bool=false);
};


// TODO
// 1. Make models take generic Object's as inputs. [DONE]
//    o make substitution models take alphabets as arguments, and thus store them in the argument list [DONE]
// 2. Make models into functions.
//    o however, some objects, like Parameters, evaluate multiple different expressions
// 3. ??
//    o make all stored objects into models?
//    o make supermodels into an Op plus models, as arguments? [WORKING...]
//    o allow models to have computed objects as parts.
//    o allow sharing between models.
//    o pull priors out of models (or expressions).
//    o do some caching of results?
// 4. Make model evaluation lazy.
//    o before: remove read(). [ DONE ] 
//    o before: remove set_rate()
//    o before: remove (set_)frequencies( )
//    o before: convert all recalc() in smodel.H to result()
//      - ReversibleFrequencyModel
//        + SimpleFrequencyModel, UniformFrequencyModel, TripletFM, IndependentNucleotideFM, TripletsFM, CodonFM,
//          AACodonFM, CodonsFM, CodonsFM2, 
//      - MultiModel
//        + 
//    o during: make recalc() a constant function.
//    o during: write parameters to submodels {now, simply}.  Recalc later.
//    o why do set_rate() and frequencies() cause problems? We should just do them on copies.
//    o call update only when calling SubModel(i), remove SuperModel::update()
// 5. Fix multimodel to return base_model(i,m) instead of base_model(m)?
// 6. Allow models to evaluate multiple expressions, ala class Parameters? (e.g. prior, likelihood, etc.)
//    - When those expressions take outside arguments, (such as Exp(Q t), where t comes from outside)
//      it might be helpful to allocate a permanent parameter for the argment of that function.  Then
//      this parameter would be clearly marked as an 'input', and would not need to be updated if the
//      *value* was unchanged.
// 7. Store cached intermediate results in same array as input variable values?
// 8. Handle various annotations, such as priors, bounds, model_names, computed variable names, transition-kernels, loggers, etc. from Formula.
// 9. Consider the possibility of evaluating functions that DO something, and have a void return type?
// 10. DONE: Make the model store most of its properties in a Context object and a Formula Object.
//   - DONE: implement Formula::rename_parameter( ).
//     + The Context will store the values, and the Formula will store the attributes.
//   - Expose the top-level attribute for Formulae (expose top-level indices only?)
//   - TODO: Get rid of references to parameters_
//     + parameters_[i].changed: separate array?
//   - How can attributes change efficiently?
// 11. Make a FormulaModel that uses the Formula to
//   (a) load default values
//   (b) set bounds.
//     and uses the context to 
//   (c) calculate priors
//   (d) calculate results...
//   [The model class still stores the bounds and fixed-ness, so they can easily change]
//   (d) How do we do things like MultiParameterModel, where an operation takes a function as an argument?
// 12. Replace OpModels with FormulaModels:
//   (a) Make each Model also an Operation.  
//   (b) Make each Model also 
//   (b) Make each Model compute priors based on the FOrmulae.
//   (c) Wait... should I 


// Rewrite:
// - A supermodel is actually a function on a collection of sub-expressions.
// - Currently models carry their own parameters, but in the future the expressions should
//   be separate from the state. (see Context.C)
// - When adding a submodel, we are essentially
//   (i)  registering a sub-expression to be calculated
//   (ii) mapping the parameters of the sub-expression onto either
//     (ii.a) constants
//     (ii.b) parameters of the *main* expression
// - Therefore, to add a submodel, and also all of its parameters as separate parameters, I would:
//   (i)  Go adding parameters with group "n", and get the resulting uniquified names.   
//   (ii) Construct the mapping between submodel parameters and top model parameters or constants.
//   (iii) Add the submodel with the specified mapping.
//   (iv) This has been done.
// - We may still need objects to have calculable attributes, like
//   (i) bounds  [only for numeric objects?]
//   (ii) being fixed or not fixed.
//   (iii) type?
//   (iv) bounds which some objects may not have...
//   (v) priors... etc.
//   In short, we need to have other objects which reference variables in two ways
//   * being ABOUT a variable, or variables (as in priors)
//   * refering to a function of the variables.
//   * priors do both, in a sense.


// Variable State: (i) Dirichlet Process or (ii) Augmenting and unaugmenting
//
// As long as we can express the probability as a fixed expression of this
// variable state then I think its OK.
//
// This could be tricky for the dirichlet process, unless we can make the entire 
// vector a parameter.... and ALSO make its elements parameters?
//
// It almost seems like we could use a list...?


/// \brief An object that depends on a number of parameters, and updates itself when they change.
///
/// Model objects
///   - depend on some number of parameters (currently, all of type double)
///   - update themselves when their parameters change.
///
/// Two Model objects cannot (easily) share a parameter, because 
/// each Model manages and 'owns' its own parameters.  This ownership
/// means that a Model specifies:
/// - a prior distribution on its parameter vector
/// - a name (a string) for each parameter
/// - an attribute (a boolean) that determinies whether each parameter is fixed or variable
///
/// Next: make models out-of-date unless we call model.update();
///
class Model: public Operation
{
protected:
  mutable bool valid;

  /// Parameter values and model attributes
  Context C;

  mutable std::vector<bool> changed;
  std::vector< Bounds<double> > bounds;
  std::vector< bool > fixed;

  int prior_index;

  void validate() const;

  void invalidate() const;

  /// Mark this parameter as being possibly different than the last computed value
  void modify_parameter(int) const;
  /// Mark these parameters as being possibly different than the last computed value
  void modify_parameters(const std::vector<int>&);
  /// Mark all parameters as being possibly different than the last computed value
  void modify_all_parameters() const;
  /// Which parameters have been modified?
  std::vector<int> modified_parameters() const;

  virtual int add_parameter(const Parameter&);
  
  /// \brief Recalculate internal data based on current parameters
  ///
  ////  \param indices The indices of the parameters that changed.
  virtual void recalc(const std::vector<int>& /*indices*/) {}

public:
  /// Recalculate all internal data based on current parameters
  void recalc_all();

  /// Make a copy of this object
  virtual Model* clone() const {return new Model(*this);}

  /// the prior for the model, on the same scale as proposal densities
  virtual efloat_t prior() const;

  /// Names of the parameters
  std::string parameter_name(int i) const;
  void rename_parameter(int i, const std::string& s);

  bool is_fixed(int i) const;
  void set_fixed(int i,bool f);

  const Bounds<double>& get_bounds(int i) const;
  void set_bounds(int i,const Bounds<double>& b);

  /// Get A model parameter value
  boost::shared_ptr<const Object> get_parameter_value(int i) const;
  boost::shared_ptr<const Object> get_parameter_value(const std::string& p_name) const;

  template <typename T>
  bool parameter_has_type(int index) const {return boost::dynamic_pointer_cast<const T>(get_parameter_value(index));}

  /// Get A model parameter value
  template <typename T>
  const T& get_parameter_value_as(int i) const {
    const Object& O = *get_parameter_value(i);
    return dynamic_cast<const T&>(O);
  }
  /// Get SOME model parameter values
  template <typename T>
  std::vector<T> get_parameter_values_as(const std::vector<int>& indices) const
  {
    std::vector<T> values(indices.size());
    
    for(int i=0;i<values.size();i++)
      values[i] = get_parameter_value_as<const T>(indices[i]);
    
    return values;  
  }

  /// Get SOME model parameter values
  std::vector<boost::shared_ptr<const Object> > get_parameter_values(const std::vector<int>&) const;

  /// Get ALL model parameter values
  template <typename T>
  std::vector<T> get_parameter_values_as() const
  {
    std::vector<T> values(n_parameters());
    
    for(int i=0;i<values.size();i++)
      values[i] = get_parameter_value_as<T>(i);
    
    return values;  
  }
  /// Get ALL model parameter values
  std::vector<boost::shared_ptr<const Object> > get_parameter_values() const;

  /// Store ONE parameter value down into submodels
  virtual void write_value(int i, const boost::shared_ptr<const Object>& p);

  /// Set A model parameter value
          void set_parameter_value(int p,Double value);
  /// Set A model parameter value
          void set_parameter_value(int p, const boost::shared_ptr<const Object>& value);
  /// Set SOME of the model parameter values
          void set_parameter_values(const std::vector<int>& indices,const std::vector<Double>& p);
  /// Set SOME of the model parameter values
          void set_parameter_values(const std::vector<int>& indices,const std::vector<boost::shared_ptr<const Object> >& p);
  /// Set ALL model parameter values
          void set_parameter_values(const std::vector<Double>& p);
  /// Set ALL model parameter values
          void set_parameter_values(const std::vector<boost::shared_ptr<const Object> >& p);

  unsigned n_parameters() const;

  int n_args() const {return n_parameters();}

  /// The model's name
  virtual std::string name() const {return "unnamed";}

  std::string print() const {return name();}

  virtual void check() const {}

  // Evaluate the model and return the result
  virtual boost::shared_ptr<const Object> result() const;
  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const;

  // Evaluate the model and return the result
  template <typename T>
  boost::shared_ptr<const T> result_as() const {return boost::dynamic_pointer_cast<const T>(result());}

  bool is_valid() const {return valid;}
  virtual void update();

  virtual operator formula_expression_ref() const;

  virtual std::vector<std::string> show_priors() const;

  /// Construct an empty model
  Model();

  Model(const boost::shared_ptr<const Formula>&);

  virtual ~Model() {};
};

struct model_prior: public Operation
{
  polymorphic_cow_ptr<Model> M;

  model_prior* clone() const {return new model_prior(*this);}

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const;

  std::string name() const {return "model_prior["+M->name()+"]";}

  std::string print() const {return name();}

  model_prior(const Model& M_):Operation(2),M(M_) { }
};


/// \brief Abstract class for Model objects that are built out of other Model objects.
///
/// A Model object that implements the SuperModel interface can
/// contain other Model objects as parts - child Model parameters are
/// mapped to parameters in the parent Model.  However, two child
/// Models cannot (easily) share a parameter; this follows from the
/// fact that each child Model 'owns' its parameters.
///
/// Names of child Model parameters are unchanged in the parent Model
/// as long as they are unique.  However, if two child Models contain
/// parameters with the same name, the parameters are disambiguated
/// using the name of the child Model as a prefix, separated by '::'.
///
/// Changes to child Model parameters are propagated down into child
/// Model objects using the various write( ) functions.  Values from
/// child Models are propagated into parameters in the parent Models
/// using the read() function.
///

struct arg_expression
{
  int parent_index;
  int sub_model_index;
  boost::shared_ptr<const Object> constant_value;

  bool is_constant() const {return parent_index == -1 and sub_model_index == -1;}
  bool is_term_ref() const {return parent_index != -1;}
  bool is_submodel_ref() const {return sub_model_index != -1;}

  arg_expression():parent_index(-1), sub_model_index(-1) {}
  arg_expression(int i):parent_index(i), sub_model_index(-1) {}
};

struct model_slot
{
  int model_index;
  int slot;
  model_slot():model_index(-1),slot(-1) {}
  model_slot(int m,int s):model_index(m),slot(s) {}
};

class SuperModel: virtual public Model 
{
protected:
  std::vector<std::vector<arg_expression> > slot_expressions_for_submodel;
  std::vector<std::vector<model_slot> > model_slots_for_index;

  /// Is parameter 'index' used by model m?
  bool parameter_is_used_by_model(int index, int m) const;
  /// Is parameter 'index' use by the top level model?
  bool is_super_parameter(int index) const;

  // How many parameters are used at the top level?
  int n_super_parameters() const;
  int add_super_parameter(const Parameter&);

  int add_parameter(const Parameter&);

  int register_last_submodel(const std::vector<arg_expression>&);
  // Register a submodel, creating separate parameter for each submodel parameter
  int register_submodel(const std::vector<arg_expression>&);
  // Register a submodel, creating separate parameter for each submodel parameter
  int register_submodel(const std::string&);

  /// The prior for the parameters of the upper model
  virtual efloat_t super_prior() const {return 1.0;}
  
  /// Propagate changes to child model parameters down to child Model objects.
  void write();
  /// Store ONE parameter value down into submodels
  void write_value(int i, const boost::shared_ptr<const Object>& p);

public:
  /// Make a copy of ourselves
  SuperModel* clone() const =0;
  
  efloat_t prior() const;

  /// Access a child Model as type T
  template<typename T>
  const T& SubModelAs(int i) const {
    const T* t = dynamic_cast<const T*>(&SubModels(i));
    assert(t);
    return *t;
  }

  /// Access a child Model as type T
  template<typename T>
  T& SubModelAs(int i) {
    T* t = dynamic_cast<T*>(&SubModels(i));
    assert(t);
    return *t;
  }

  /// Access a child Model
  virtual const Model& SubModels(int) const=0;

  /// Access a child Model
  virtual Model& SubModels(int)=0;

  /// How many child models are there?
  int n_submodels() const;

  void check() const;

  virtual void update();

  virtual std::vector<std::string> show_priors() const;

  SuperModel();
};

/// This is a substitution model built over a submodel of a particular type
template <class SubModel_t>
class SuperModelOver: virtual public SuperModel {
  
protected:
  /// The sub-models this model is build upon
  std::vector< owned_ptr<SubModel_t> > sub_models;

  int insert_submodel(const std::string& name, const SubModel_t& M)
  {
    sub_models.push_back(M);
    return register_submodel(name);
  }
  
  /// Reference the submodels
  const SubModel_t& SubModels(int i) const {return *sub_models[i];}
  SubModel_t& SubModels(int i) {return *sub_models[i];}

public:
  /// Make a copy of ourselves
  SuperModelOver<SubModel_t>* clone() const =0;
};

struct expression_ref;

class OpModel: virtual public Model
{
  friend class OpModelOperationArgs;

  /// by index, what does each input parameter do?
  std::vector<std::vector<model_slot> > model_slots_for_index;

  /// these are the instantiated sub_models
  std::vector< polymorphic_cow_ptr< Model > > sub_models;

  /// what is the top-level operation
  boost::shared_ptr< const Operation > Op;
  /// how do we assemble the inputs for the top-level op?
  std::vector<arg_expression> slot_expressions_for_op;

  void write_value(int i, const boost::shared_ptr<const Object>& p);

  void recalc(const std::vector<int>&) {std::abort();}

  // Is this a good idea?
  void update() { }

  int add_submodel(boost::shared_ptr<const Model>);
public:

  OpModel* clone() const {return new OpModel(*this);}

  efloat_t prior() const;

  /// The prior for the parameters of the upper model
  virtual efloat_t super_prior() const {return 1.0;}

  std::string name() const;

  void check() const;

  boost::shared_ptr<const Object> slot_result(int) const;
  
  boost::shared_ptr<const Object> result() const;

  virtual std::vector<std::string> show_priors() const;

  OpModel(const expression_ref&);
};

class LambdaModel: public Model
{
  int p_change;

  polymorphic_cow_ptr< Model > sub_model;
public:
  LambdaModel* clone() const {return new LambdaModel(*this);}

  void write_value(int i, const boost::shared_ptr<const Object>& p);
  
  void recalc(const std::vector<int>&) {std::abort();}

  efloat_t prior() const;

  std::string name() const;

  boost::shared_ptr<const Object> result() const;

  LambdaModel(const Model& M, int p_change);
  LambdaModel(const Model& M, const std::string&);
};



class ModelFunction: virtual public Object
{
  int p_change;

  mutable polymorphic_cow_ptr < Model > sub_model;
public:
  ModelFunction* clone() const {return new ModelFunction(*this);}

  boost::shared_ptr<const Object> operator()(boost::shared_ptr<const Object> O) const;

  ModelFunction(const Model& M, int p);
};

struct FormulaModel: public Model
{
  int result_index;

  FormulaModel* clone() const {return new FormulaModel(*this);}

  boost::shared_ptr<const Object> result() const;

  std::string name() const;

  operator formula_expression_ref() const;

  FormulaModel(const boost::shared_ptr<const Formula>&,int);
  FormulaModel(const formula_expression_ref&);
};


void show_parameters(std::ostream& o,const Model& M);

bool has_parameter(const Model& M, const std::string& name);

std::vector<std::string> parameter_names(const Model& M);

std::vector<std::string> short_parameter_names(const Model& M);

std::vector<std::string> short_parameter_names(std::vector<std::string> names);

std::vector<int> parameters_with_extension(const Model& M, std::string name);

boost::shared_ptr<Model> prefix_model(const Model&, const std::string& prefix);

std::vector< expression_ref > model_args(const Model& M);

expression_ref model_result_expression(const Model& M);

term_ref add_probability_expression(Context& C);

std::vector< std::string > show_probability_expressions(const Context& C);

formula_expression_ref model_formula(const Model& M);
#endif
