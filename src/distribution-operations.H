#ifndef DISTRIBUTION_OPERATIONS_H
#define DISTRIBUTION_OPERATIONS_H

#include <vector>
#include <valarray>
#include <string>
#include "expression.H"
#include "operation.H"
#include "computation.H"
#include "probability.H"

struct exponential_density: public Operation
{
  exponential_density* clone() const {return new exponential_density;}
  
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    boost::shared_ptr<const Double> x = Args.evaluate_as<Double>(0);
    boost::shared_ptr<const Double> mu = Args.evaluate_as<Double>(1);

    Log_Double result = exponential_pdf(*x,*mu);
    return boost::shared_ptr<const Object>(result.clone());
  }

  std::string name() const {return "exponential_density";}

  exponential_density():Operation(2) { }
};

extern expression_ref exponential_dist;

struct gamma_density: public Operation
{
  gamma_density* clone() const {return new gamma_density;}
  
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    boost::shared_ptr<const Double> x = Args.evaluate_as<Double>(0);
    expression_ref a = Args.evaluate(1);

    std::valarray<double> A = get_varray<double,Double>(a);
    Log_Double result = gamma_pdf(*x, A[0], A[1]);
    return boost::shared_ptr<const Object>(result.clone());
  }

  std::string name() const {return "gamma_density";}

  gamma_density():Operation(2) { }
};

extern expression_ref gamma_dist;

struct beta_density: public Operation
{
  beta_density* clone() const {return new beta_density;}
  
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    boost::shared_ptr<const Double> x = Args.evaluate_as<Double>(0);
    expression_ref a = Args.evaluate(1);

    std::valarray<double> A = get_varray<double,Double>(a);
    Log_Double result = beta_pdf(*x, A[0], A[1]);
    return boost::shared_ptr<const Object>(result.clone());
  }

  std::string name() const {return "beta_density";}

  beta_density():Operation(2) { }
};

extern expression_ref beta_dist;

struct normal_density: public Operation
{
  normal_density* clone() const {return new normal_density;}
  
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    boost::shared_ptr<const Double> x = Args.evaluate_as<Double>(0);
    expression_ref a = Args.evaluate(1);

    std::valarray<double> A = get_varray<double,Double>(a);
    Log_Double result = normal_pdf(*x, A[0], A[1]);
    return boost::shared_ptr<const Object>(result.clone());
  }

  std::string name() const {return "normal_density";}

  normal_density():Operation(2) { }
};

extern expression_ref normal_dist;

struct log_normal_density: public Operation
{
  log_normal_density* clone() const {return new log_normal_density;}
  
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    boost::shared_ptr<const Double> x = Args.evaluate_as<Double>(0);
    expression_ref a = Args.evaluate(1);

    std::valarray<double> A = get_varray<double,Double>(a);
    Log_Double result = log_normal_pdf(*x, A[0], A[1]);
    return boost::shared_ptr<const Object>(result.clone());
  }

  std::string name() const {return "log_normal_density";}

  log_normal_density():Operation(2) { }
};

extern expression_ref log_normal_dist;

struct cauchy_density: public Operation
{
  cauchy_density* clone() const {return new cauchy_density;}
  
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    boost::shared_ptr<const Double> x = Args.evaluate_as<Double>(0);
    expression_ref a = Args.evaluate(1);

    std::valarray<double> A = get_varray<double,Double>(a);
    Log_Double result = cauchy_pdf(*x, A[0], A[1]);
    return boost::shared_ptr<const Object>(result.clone());
  }

  std::string name() const {return "cauchy_density";}

  cauchy_density():Operation(2) { }
};

extern expression_ref cauchy_dist;

struct dirichlet_density: public Operation
{
  dirichlet_density* clone() const {return new dirichlet_density;}
    
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    expression_ref X = Args.evaluate(0);
    expression_ref N = Args.evaluate(1);
      
    std::valarray<double> x = get_varray<double,Double>(X);
    std::valarray<double> n = get_varray<double,Double>(N);

    boost::shared_ptr<Log_Double> R (new Log_Double( ::dirichlet_pdf(x,n) ) );

    return R;
  }

  std::string name() const {return "dirichlet_density";}
    
  dirichlet_density():Operation(2) { }
};

extern expression_ref dirichlet_dist;

struct laplace_density: public Operation
{
  laplace_density* clone() const {return new laplace_density;}
    
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    double x = *Args.evaluate_as<Double>(0);
    expression_ref A =  Args.evaluate(1);

    std::vector<double> a = get_vector<double,Double>(A);

    return boost::shared_ptr<Log_Double> (new Log_Double( ::laplace_pdf(x,a[0],a[1]) ) );
  }

  std::string name() const {return "laplace_density";}
    
  laplace_density():Operation(2) { }
};

extern expression_ref laplace_dist;

struct log_laplace_density: public Operation
{
  log_laplace_density* clone() const {return new log_laplace_density;}
    
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    double x = *Args.evaluate_as<Double>(0);
    expression_ref A =  Args.evaluate(1);

    std::vector<double> a = get_vector<double,Double>(A);

    return boost::shared_ptr<Log_Double> (new Log_Double( ::laplace_pdf(log(x),a[0],a[1])/x ) );
  }

  std::string name() const {return "log_laplace_density";}
    
  log_laplace_density():Operation(2) { }
};

extern expression_ref log_laplace_dist;

struct uniform_density: public Operation
{
  uniform_density* clone() const {return new uniform_density;}
    
  tribool compare(const Object& O) const
  {
    if (this == &O) 
      return true;

    if (typeid(*this) != typeid(O)) return false;

    return true;
  }

  boost::shared_ptr<const Object> operator()(OperationArgs& Args) const
  {
    double x = *Args.evaluate_as<Double>(0);
    expression_ref A =  Args.evaluate(1);

    std::vector<double> a = get_vector<double,Double>(A);

    return boost::shared_ptr<Log_Double> (new Log_Double( ::uniform_pdf(x,a[0],a[1]) ) );
  }

  std::string name() const {return "uniform_density";}
    
  uniform_density():Operation(2) { }
};

extern expression_ref uniform_dist;

#endif
