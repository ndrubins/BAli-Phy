/*
   Copyright (C) 2007-2008 Benjamin Redelings

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

#include "distribution.H"
#include <cassert>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_sf_gamma.h>
#include "util.H"

using std::string;

namespace probability {

  double  Distribution::mean() const {
    return moment(1);
  }

  double  Distribution::variance() const {
    double M2 = moment(2);
    double M1 = mean();
    return M2-M1*M1;
  }

  // f(x) = cdf(x)-p, monotonically increasing from -p (@0) to 1-p (@ \infty)
  double Distribution::quantile(double p, double tol) const 
  {
    assert(0.0 <= p and p <= 1.0);

    double x = 1.0;

    // We know the zero is in (min,max), if max>=min
    double min=0.0;
    double max=-1;

    const int max_iterations = 2000;
    int iterations=0;

    double dx = 0.001;
    while(std::abs(dx) > tol and iterations < max_iterations) {
      double f = cdf(x)-p;

      // take advantage of the monotonicity
      if (f < 0)
	min = x;
      else if (f > 0)
	max = x;
      else return x;

      // propose do Newton-Raphson step
      double dfdx = pdf(x);
      dx = -f/dfdx;

      // deal with trying to jump out of (min,max)
      double x2 = x;
      if (x2+dx < min)
	x2 = (x2+min)/2.0;
      else if (x+dx > max and max >= 0)
	x2 = (x2+max)/2.0;
      else
	x2 = x2 + dx;
      iterations++;

      double f2 = cdf(x2) - p;
      // move to x2 if its an improvement
      if (std::abs(f2) <= std::abs(f)) 
	x = x2;
      // If x2 is NOT an improvement then...
      else {
	assert(min <= x2);
	assert(max < 0 or x2 <= max);

	// ...update (min,max), ...
	if (f2 < 0)
	  min = x2;
	else if (f2 > 0)
	  max = x2;
	else
	  return x2;

	assert(max >= 0);

	if (max > 0) {
	  // ...propose another location, ...
	  x2 = 0.5*(min + max);

	  f2 = cdf(x2) - p;
	  if (f2 < 0)
	    min = x2;
	  else if (f2 > 0)
	    max = x2;
	  else
	    return x2;
	  
	  // and try to move there.
	  if (std::abs(f2) < std::abs(f))
	    x = x2;
	}

      }

      assert(min <= x);
      if (max >= 0) {
	assert(min <= max);
	assert(x <= max);
	assert(cdf(min) <= p and p <= cdf(max));
      }
      assert(iterations<max_iterations);
    }
    return x;
  }

  Distribution::~Distribution() {}


  //--------------- Uniform Distribution -----------------//

  double Uniform::cdf(double x) const
  {
    if (x < start()) return 0;
    if (x > end()) return 1;
    else return (x-start())/(end()-start());
  }

  log_double_t Uniform::pdf(double x) const 
  {
    if (x < start() or x > end()) 
      return 0; 
    else return 1.0/(end()-start());
  }

  double Uniform::quantile(double p,double) const 
  {
    double s = start();
    double e = end();

    return s+p*(e-s);
  }

  string Uniform::name() const {
    return "Uniform";
  }

  double Uniform::moment(int n) const 
  {
    double s = start();
    double e = end();
    double temp = pow(e,n+1) - pow(s,n+1);
    return temp/(e-s)/(n+1);
  }

  Uniform::Uniform() 
  {
    add_parameter(Parameter("start", Double(0.0)));
    add_parameter(Parameter("end", Double(1.0)));
  }

  Uniform::Uniform(double s, double e) 
  {
    assert(s<e);
    add_parameter(Parameter("start", Double(s)));
    add_parameter(Parameter("end", Double(e)));
  }

  Uniform::~Uniform() {}

  //--------------- Gamma Distribution -----------------//

  string Gamma::name() const {
    return "Gamma";
  }

  double Gamma::cdf(double x) const 
  {
    double a = alpha();
    double b = beta();

    if (a < 1000)
      return gsl_cdf_gamma_P(x,a,b);
    else {
      double M = mean();
      double V = variance();

      double sigma2 =  log1p(V/(M*M));
      double mu = log(M) - sigma2/2.0;
      double sigma = sqrt(sigma2);

      // don't go crazy
      sigma = minmax(sigma, 1.0e-5, 1.0e5);

      return gsl_cdf_lognormal_P(x,mu,sigma);
    }
  }

  log_double_t Gamma::pdf(double x) const 
  {
    double a = alpha();
    double b = beta();

    if (a < 1000)
      return gsl_ran_gamma_pdf(x,a,b);
    else {
      double M = mean();
      double V = variance();

      double sigma2 =  log1p(V/(M*M));
      double mu = log(M) - sigma2/2.0;
      double sigma = sqrt(sigma2);

      // don't go crazy
      sigma = minmax(sigma, 1.0e-5, 1.0e5);

      return gsl_ran_lognormal_pdf(x,mu,sigma);
    }
  }

  double Gamma::quantile(double p,double) const 
  {
    double a = alpha();
    double b = beta();

    return gamma_quantile(p,a,b);
  }

  double Gamma::moment(int n) const
  {
    double a = alpha();
    double b = beta();

    double M =1.0;
    for(int i=0;i<n;i++)
      M *= (a+i);
    return M*pow(b,n);
  }

  double Gamma::mean() const 
  {
    return alpha()*beta();
  }

  double Gamma::variance() const 
  {
    double a = alpha();
    double b = beta();

    return a*b*b;
  }


  Gamma::Gamma() {
    add_parameter(Parameter("alpha", Double(1.0), lower_bound(0.0)));
    add_parameter(Parameter("beta", Double(1.0), lower_bound(0.0)));
  }

  Gamma::Gamma(double a, double b) {
    add_parameter(Parameter("alpha", Double(a), lower_bound(0.0)));
    add_parameter(Parameter("beta", Double(b), lower_bound(0.0)));
  }

  Gamma::~Gamma() {}

  //--------------- Beta Distribution -----------------//

  string Beta::name() const {
    return "Beta";
  }

  double Beta::cdf(double x) const 
  {
    double a = alpha();
    double b = beta();

    if (a<0 or b<0)
      a=b=1;

    double r = 100.0/std::max(a,b);
    if (r < 1) {
      a *= r;
      b *= r;
    }

    return gsl_cdf_beta_P(x,a,b);
  }

  log_double_t Beta::pdf(double x) const 
  {
    double a = alpha();
    double b = beta();

    if (a<0 or b<0)
      a=b=1;

    double r = 100.0/std::max(a,b);
    if (r < 1) {
      a *= r;
      b *= r;
    }

    return gsl_ran_beta_pdf(x,a,b);
  }

  double Beta::quantile(double p,double) const 
  {
    double a = alpha();
    double b = beta();

    if (a<0 or b<0)
      a=b=1;

    double r = 100.0/std::max(a,b);
    if (r < 1) {
      a *= r;
      b *= r;
    }

    return gsl_cdf_beta_Pinv(p,a,b);
  }

  double Beta::moment(int n) const
  {
    double a = alpha();
    double b = beta();

    double M =1.0;
    for(int i=0;i<n;i++)
      M *= (a+i)/(a+b+i);
    return M;
  }

  double Beta::mean() const 
  {
    double a = alpha();
    double b = beta();

    return a/(a+b);
  }


  Beta::Beta() {
    add_parameter(Parameter("alpha", Double(1.0), lower_bound(0.0)));
    add_parameter(Parameter("beta", Double(1.0), lower_bound(0.0)));
  }

  Beta::Beta(double a, double b) {
    add_parameter(Parameter("alpha", Double(a), lower_bound(0.0)));
    add_parameter(Parameter("beta", Double(b), lower_bound(0.0)));
  }

  Beta::~Beta() {}

  //--------------- LogNormal Distribution -----------------//

  string LogNormal::name() const {
    return "LogNormal";
  }

  double LogNormal::cdf(double x) const 
  {
    // don't go crazy
    double s = minmax(lsigma(), 1.0e-5, 1.0e5);

    return gsl_cdf_lognormal_P(x,lmu(),s);
  }

  log_double_t LogNormal::pdf(double x) const 
  {
    // don't go crazy
    double s = minmax(lsigma(), 1.0e-5, 1.0e5);

    return gsl_ran_lognormal_pdf(x,lmu(),s);
  }

  double LogNormal::quantile(double p,double) const 
  {
    // don't go crazy
    double s = minmax(lsigma(), 1.0e-5, 1.0e5);

    return gsl_cdf_lognormal_Pinv(p,lmu(),s);
  }

  double LogNormal::moment(int n) const
  {
    double m = lmu();
    double s = minmax(lsigma(), 1.0e-5, 1.0e5);

    return exp(n*m + 0.5*(n*n*s*s));
  }

  double LogNormal::mean() const 
  {
    double m = lmu();
    double s = minmax(lsigma(), 1.0e-5, 1.0e5);

    return exp(m + 0.5*s*s);
  }

  double LogNormal::variance() const 
  {
    double m = lmu();
    double s = minmax(lsigma(), 1.0e-5, 1.0e5);

    return exp(s*s-1.0) - exp(2*m+s*s);
  }

  LogNormal::LogNormal() {
    add_parameter(Parameter("lmu", Double(0.0), lower_bound(0.0)));
    add_parameter(Parameter("lsigma", Double(1.0), lower_bound(0.0)));
  }

  LogNormal::LogNormal(double lmu, double lsigma) {
    add_parameter(Parameter("lmu", Double(lmu), lower_bound(0.0)));
    add_parameter(Parameter("lsigma", Double(lsigma), lower_bound(0.0)));
  }

  LogNormal::~LogNormal() {}

  //--------------- Normal Distribution -----------------//

  string Normal::name() const {
    return "Normal";
  }

  double Normal::cdf(double x) const 
  {
    return gsl_cdf_gaussian_P(x-mu(),sigma());
  }

  log_double_t Normal::pdf(double x) const 
  {
    return gsl_ran_gaussian_pdf(x-mu(),sigma());
  }

  double Normal::quantile(double p,double) const 
  {
    return mu()+gsl_cdf_gaussian_Pinv(p,sigma());
  }

  double Normal::moment(int n) const
  {
    double m = mu();
    double s = sigma();

    if (n==0)
      return 1.0;
    else if (n==1)
      return m;
    else if (n==2)
      return (m*m+s*s);
    else if (n==3)
      return m*(m*m + 3*s*s);
    else if (n==4)
      return pow(m,4)+6*m*m*s*s + 3*pow(s,4);
    else
      std::abort();
  }

  double Normal::mean() const 
  {
    return mu();
  }

  double Normal::variance() const 
  {
    double s = sigma();
    return s*s;
  }

  Normal::Normal() {
    add_parameter(Parameter("mu", Double(0.0), lower_bound(0.0)));
    add_parameter(Parameter("sigma", Double(1.0), lower_bound(0.0)));
  }

  Normal::Normal(double mu, double sigma) {
    add_parameter(Parameter("mu", Double(mu), lower_bound(0.0)));
    add_parameter(Parameter("sigma", Double(sigma), lower_bound(0.0)));
  }

  Normal::~Normal() {}

  //--------------- Exponential Distribution -----------------//

  string Exponential::name() const {
    return "Exponential";
  }

  double Exponential::cdf(double x) const 
  {
    return gsl_cdf_exponential_P(x,mu());
  }

  log_double_t Exponential::pdf(double x) const 
  {
    return gsl_ran_exponential_pdf(x,mu());
  }

  double Exponential::quantile(double p,double) const 
  {
    return gsl_cdf_exponential_Pinv(p,mu());
  }

  double Exponential::moment(int n) const
  {
    double M =1.0;
    for(int i=0;i<n;i++)
      M *= (1+i);
    return M*pow(mu(),n);
  }

  double Exponential::mean() const 
  {
    return mu();
  }

  double Exponential::variance() const 
  {
    double m = mu();
    return m*m;
  }

  Exponential::Exponential() {
    add_parameter(Parameter("mu", Double(1.0), lower_bound(0.0)));
  }

  Exponential::Exponential(double mu) {
    add_parameter(Parameter("mu", Double(mu), lower_bound(0.0)));
  }

  Exponential::~Exponential() {}

  string Cauchy::name() const {
    return "Cauchy";
  }

  log_double_t Cauchy::pdf(double x) const
  {
    return gsl_ran_exponential_pdf(x-m() ,s());
  }

  double Cauchy::cdf(double x) const
  {
    return gsl_ran_exponential_pdf(x -m(), s());
  }

  double Cauchy::quantile(double p,double) const 
  {
    return m() + gsl_cdf_cauchy_Pinv(p, s());
  }

  double Cauchy::moment(int) const
  {
    std::abort();
  }

  Cauchy::Cauchy() 
  {
    add_parameter(Parameter("m", Double(0), lower_bound(0.0)));
    add_parameter(Parameter("s", Double(1), lower_bound(0.0)));
  }

  Cauchy::Cauchy(double m_, double s_)
  {
    add_parameter(Parameter("m", Double(m_), lower_bound(0.0)));
    add_parameter(Parameter("s", Double(s_), lower_bound(0.0)));
  }

  Cauchy::~Cauchy() {}

  double pointChi2(double prob, double v)
  {
    // Returns z so that Prob{x<z}=prob where x is Chi2 distributed with df
    // = v
    // RATNEST FORTRAN by
    // Best DJ & Roberts DE (1975) The percentage points of the
    // Chi2 distribution. Applied Statistics 24: 385-388. (AS91)

    double e = 0.5e-6, aa = 0.6931471805, p = prob, g;
    double xx, c, ch, a = 0, q = 0, p1 = 0, p2 = 0, t = 0, x = 0, b = 0, s1, s2, s3, s4, s5, s6;
    
    if (v <= 0)
      throw myexception()<<"Arguments out of range: alpha = "<<a;

    assert(p >= 0 and p <= 1);

    if (p < 0.000002) 
    {
      std::cerr<<"Warning: can't handle p = "<<p<<" in gamma quantile: using 0.000002";

      p = 0.000002;
    }

    if (p > 0.999998) 
    {
      std::cerr<<"Warning: can't handle p = "<<p<<" in gamma quantile: using 0.999998";

      p = 0.999998;
    }

    g = gsl_sf_lngamma(v / 2);
    
    xx = v / 2;
    c = xx - 1;
    if (v < -1.24 * log(p)) {
      ch = pow((p * xx * exp(g + xx * aa)), 1 / xx);
      if (ch - e < 0) {
	return ch;
      }
    } else {
      if (v > 0.32) {
	x = gsl_cdf_gaussian_Pinv(p,1);
	p1 = 0.222222 / v;
	ch = v * pow((x * sqrt(p1) + 1 - p1), 3.0);
	if (ch > 2.2 * v + 6) {
	  ch = -2 * (log(1 - p) - c * log(.5 * ch) + g);
	}
      } else {
	ch = 0.4;
	a = log(1 - p);

	do {
	  q = ch;
	  p1 = 1 + ch * (4.67 + ch);
	  p2 = ch * (6.73 + ch * (6.66 + ch));
	  t = -0.5 + (4.67 + 2 * ch) / p1
	    - (6.73 + ch * (13.32 + 3 * ch)) / p2;
	  ch -= (1 - exp(a + g + .5 * ch + c * aa) * p2 / p1)
	    / t;
	} while (std::abs(q / ch - 1) - .01 > 0);
      }
    }
    do {
      q = ch;
      p1 = 0.5 * ch;
      
      if ((t = gsl_sf_gamma_inc_P(xx, p1)) < 0)
	throw myexception()<<"Arguments out of range: t < 0";

      p2 = p - t;
      t = p2 * exp(xx * aa + g + p1 - c * log(ch));
      b = t / ch;
      a = 0.5 * t - b * c;

      s1 = (210 + a * (140 + a * (105 + a * (84 + a * (70 + 60 * a))))) / 420;
      s2 = (420 + a * (735 + a * (966 + a * (1141 + 1278 * a)))) / 2520;
      s3 = (210 + a * (462 + a * (707 + 932 * a))) / 2520;
      s4 = (252 + a * (672 + 1182 * a) + c * (294 + a * (889 + 1740 * a))) / 5040;
      s5 = (84 + 264 * a + c * (175 + 606 * a)) / 2520;
      s6 = (120 + c * (346 + 127 * c)) / 5040;
      ch += t
	* (1 + 0.5 * t * s1 - b
	   * c
	   * (s1 - b
	      * (s2 - b
		 * (s3 - b
		    * (s4 - b * (s5 - b * s6))))));
    } while (std::abs(q / ch - 1) > e);
    
    assert(not std::isnan(ch) and std::isfinite(ch));
    assert(ch >= 0);
    return (ch);
  }

  double gamma_quantile_no_approx(double p, double a, double b)
  {
    assert(a >= 0);
    assert(b >= 0);

    assert(p >= 0);
    assert(p <= 1);

    return 0.5 * b * pointChi2(p, 2.0* a);
  }

  double gamma_quantile(double p, double a, double b)
  {
    assert(a >= 0);
    assert(b >= 0);
    assert(p >= 0);
    assert(p <= 1);

    if (a < 10000)
      return gamma_quantile_no_approx(p,a,b);
    else {
      double M = a*b;
      double V = a*b*b;

      double sigma2 =  log1p(V/(M*M));
      double mu = log(M) - sigma2/2.0;
      double sigma = sqrt(sigma2);

      // don't go crazy
      sigma = minmax(sigma, 1.0e-5, 1.0e5);

      return gsl_cdf_lognormal_Pinv(p,mu,sigma);
    }
  }
}
