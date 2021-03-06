#include "smodel/objects.H"

#include "exponential.H"

using std::vector;
using std::valarray;
using std::string;
using boost::shared_ptr;

namespace substitution
{
  SModelObject::SModelObject(const alphabet& A)
    :a(ptr(A)),
     state_letters_(A.size())
  { 
    for(int i=0;i<n_states();i++)
      state_letters_[i] = i;
  }

  SModelObject::SModelObject(const alphabet& A, int n)
    :a(ptr(A)),
     state_letters_(n)
  { 
    if (n%A.size() != 0)
      throw myexception()<<"Cannot construct a model with "<<A.size()<<" letters and "<<n<<" states!";

    for(int i=0;i<n_states();i++)
      state_letters_[i] = i%n_letters();
  }


  AlphabetExchangeModelObject::AlphabetExchangeModelObject(const alphabet& a)
    :SModelObject(a),ExchangeModelObject(a.size())
  {
  }

  AlphabetExchangeModelObject::AlphabetExchangeModelObject(const alphabet& a, int n)
    :SModelObject(a,n),ExchangeModelObject(n)
  {
  }

  ReversibleAdditiveObject::ReversibleAdditiveObject(const alphabet& a)
    :SModelObject(a)
  { }

  ReversibleAdditiveObject::ReversibleAdditiveObject(const alphabet& a, int n)
    :SModelObject(a,n)
  { }


  std::valarray<double> ReversibleMarkovModelObject::frequencies() const {return get_varray<double>(pi);}

  ReversibleMarkovModelObject::ReversibleMarkovModelObject(const alphabet& A)
    :ReversibleAdditiveObject(A),
     eigensystem(A.size()),
     Q(A.size(), A.size()),
     pi(A.size())
  { }

  ReversibleMarkovModelObject::ReversibleMarkovModelObject(const alphabet& A,int n)
    :ReversibleAdditiveObject(A,n),
     eigensystem(n),
     Q(A.size(), A.size()),
     pi(n)
  { }


  // Q(i,j) = S(i,j)*pi[j]   for i!=j
  // Q(i,i) = -sum_{i!=j} S(i,j)*pi[j]

  // We want to set S(i,i) so that Q(i,j) = S(i,j)*pi[j] for all i,j
  // Then Q = S*D, and we can easily compute the exponential
  // So, S(i,j) = Q(i,i)/pi[i]

  double ReversibleMarkovModelObject::rate() const 
  {
    const unsigned N = n_states();
    
    double scale=0;

    if (N == Alphabet().size()) 
    {
      for(int i=0;i<Q.size1();i++) 
	scale -= frequencies()[i]*Q(i,i);
    }
    else 
    {
      const vector<unsigned>& smap = state_letters();

      for(int s1=0;s1<N;s1++)
      {
	double temp = 0;
	for(int s2=0;s2<N;s2++)
	  if (smap[s1] != smap[s2])
	    temp += Q(s1,s2);

	scale += temp*frequencies()[s1];
      }
    }

    return scale/Alphabet().width();
  }

  void ReversibleMarkovModelObject::invalidate_eigensystem() 
  {
    eigensystem.invalidate();
    // UNCOMMENT - to test savings from lazy eigensystem calculation.
    // recalc_eigensystem();
  }

  const EigenValues& ReversibleMarkovModelObject::get_eigensystem() const
  {
    if (not eigensystem.is_valid())
      recalc_eigensystem();

    assert(eigensystem.is_valid());

    return eigensystem;
  }

  void ReversibleMarkovModelObject::set_rate(double r)  
  {
    if (r == rate()) return;

    if (rate() == 0 and r != 0)
      throw myexception()<<"Model rate is 0, can't set it to "<<r<<".";

    double scale = r/rate();
    Q *= scale;

    if (eigensystem.is_valid())
    {
      EigenValues& E = eigensystem.modify_value();

      for(int i=0;i<E.Diagonal().size();i++)
	E.Diagonal()[i] *= scale ;

      // We changed it, but now its up-to-date.
      eigensystem.validate();
    }
  }

  /*
   * 1. pi[i]*Q(i,j) = pi[j]*Q(j,i)         - Because Q is reversible
   * 2. Q(i,j)/pi[j] = Q(j,i)/pi[i] = S1(i,j)
   * 3. pi[i]^1/2 * Q(j,i) / pi[j]^1/2 = S2(i,j)
   * 4. exp(Q) = pi^-1.2 * exp(pi^1/2 * Q * pi^-1/2) * pi^1/2
   *           = pi^-1.2 * exp(S2) * pi^1/2
   */

  void ReversibleMarkovModelObject::recalc_eigensystem() const
  {
    const unsigned n = n_states();

#ifdef DEBUG_RATE_MATRIX
    cerr<<"scale = "<<rate()<<endl;

    assert(std::abs(frequencies().sum()-1.0) < 1.0e-6);
    for(int i=0;i<n;i++) {
      double sum = 0;
      for(int j=0;j<n;j++)
	sum += Q(i,j);
      assert(abs(sum) < 1.0e-6);
    }
#endif

    //--------- Compute pi[i]**0.5 and pi[i]**-0.5 ----------//
    vector<double> sqrt_pi(n);
    vector<double> inverse_sqrt_pi(n);
    for(int i=0;i<n;i++) {
      sqrt_pi[i] = sqrt(frequencies()[i]);
      inverse_sqrt_pi[i] = 1.0/sqrt_pi[i];
    }

    //--------------- Calculate eigensystem -----------------//
    ublas::symmetric_matrix<double> S(n,n);
    for(int i=0;i<n;i++)
      for(int j=0;j<=i;j++) {
	S(i,j) = Q(i,j) * sqrt_pi[i] * inverse_sqrt_pi[j];

#ifdef DEBUG_RATE_MATRIX
	// check reversibility of rate matrix
	if (i != j) {
	  assert (S(i,j) >= 0);
	  double p12 = Q(i,j)*frequencies()[i];
	  double p21 = Q(j,i)*frequencies()[j];
	  assert (abs(p12-p21) < 1.0e-12*(1.0+abs(p12)));
	}
	else
	  assert (Q(i,j) <= 0);
#endif
      }

    //---------------- Compute eigensystem ------------------//
    eigensystem = EigenValues(S);
  }

  Matrix ReversibleMarkovModelObject::transition_p(double t) const 
  {
    vector<double> pi2(n_states());
    const valarray<double> f = frequencies();
    assert(pi2.size() == f.size());
    for(int i=0;i<pi2.size();i++)
      pi2[i] = f[i];
    return exp(get_eigensystem(), pi2,t);
  }

  //------------------------ F81 Model -------------------------//

  Matrix F81_Object::transition_p(double t) const
  {
    const unsigned N = n_states();

    Matrix E(N,N);

    const double exp_a_t = exp(-alpha_ * t);

    for(int i=0;i<N;i++)
      for(int j=0;j<N;j++)
	E(i,j) = pi[j] + (((i==j)?1.0:0.0) - pi[j])*exp_a_t;

    return E;
  }

  double F81_Object::rate() const
  {
    const unsigned N = n_states();

    double sum=0;
    for(int i=0;i<N;i++)
      sum += pi[i]*(1.0-pi[i]);

    return sum*alpha_;
  }

  void F81_Object::set_rate(double r)
  {
    if (r == rate()) return;

    if (rate() == 0 and r != 0)
      throw myexception()<<"Model rate is 0, can't set it to "<<r<<".";

    double scale = r/rate();

    Q *= scale;

    alpha_ *= scale;
  }

  F81_Object::F81_Object(const alphabet& a)
    :ReversibleMarkovModelObject(a),
     alpha_(1)
  {
    const int N = a.size();

    for(int i=0;i<N;i++)
      pi[i] = 1.0/N;

    for(int i=0;i<N;i++)
      for(int j=0;j<N;j++)
	Q(i,j) = (pi[j] - ((i==j)?1:0))*alpha_;
  }

  F81_Object::F81_Object(const alphabet& a, const valarray<double>& v)
    :ReversibleMarkovModelObject(a),
     alpha_(1)
  { 
    const int N = a.size();

    pi = get_vector<double>(v);

    for(int i=0;i<N;i++)
      for(int j=0;j<N;j++)
	Q(i,j) = (pi[j] - ((i==j)?1:0))*alpha_;
  }

  shared_ptr<const alphabet> ReversibleAdditiveCollectionObject::get_alphabet() const 
  {
    return part(0).get_alphabet();
  }

  int ReversibleAdditiveCollectionObject::n_parts() const
  {
    return parts.size();
  }

  const ReversibleAdditiveObject& ReversibleAdditiveCollectionObject::part(int i) const
  {
    return *parts[i];
  }

  ReversibleAdditiveObject& ReversibleAdditiveCollectionObject::part(int i)
  {
    return *parts[i];
  }

  const vector<unsigned>& ReversibleAdditiveCollectionObject::state_letters() const
  {
    return part(0).state_letters();
  }

  double ReversibleAdditiveCollectionObject::rate() const
  {
    // FIXME... (see below)
    if (n_parts() > 1) std::abort();

    // Does this really work?
    return part(0).rate();
  }

  void ReversibleAdditiveCollectionObject::set_rate(double r)
  {
    for(int i=0;i<n_parts();i++)
      return part(i).set_rate(r);
  }

  Matrix ReversibleAdditiveCollectionObject::transition_p(double t, int i) const
  {
    return part(i).transition_p(t);
  }

  valarray<double> ReversibleAdditiveCollectionObject::frequencies() const
  {
    return part(0).frequencies();
  }

  int DiscreteDistribution::size() const
  {
    assert(fraction.size() == values.size());
    return fraction.size();
  }

  DiscreteDistribution::DiscreteDistribution(int s)
    :fraction(s), values(s)
  { }

  shared_ptr<const alphabet> MultiModelObject::get_alphabet() const 
  {
    return base_model(0).get_alphabet();
  }

  valarray<double> MultiModelObject::frequencies() const
  {
    valarray<double> pi(0.0, Alphabet().size());

    //recalculate pi
    for(int i=0; i < n_base_models(); i++)
      pi += fraction[i] * base_models[i]->frequencies();

    return pi;
  }

  void MultiModelObject::resize(int s)
  {
    fraction.resize(s);
    base_models.resize(s);
  }

  int MultiModelObject::n_parts() const
  {
    // This should be the same for all base models.
    return base_model(0).n_parts();
  }

  double MultiModelObject::rate() const {
    double r=0;
    for(int m=0;m<n_base_models();m++)
      r += distribution()[m]*base_model(m).rate();
    return r;
  }

  void MultiModelObject::set_rate(double r)  {
    double scale = r/rate();
    for(int m=0;m<n_base_models();m++) {
      base_model(m).set_rate(base_model(m).rate()*scale);
    }
  }

  // This is per-branch, per-column - doesn't pool info about each branches across columns
  Matrix MultiModelObject::transition_p(double t) const {
    Matrix P = distribution()[0] * transition_p(t,0,0);
    for(int m=1;m<n_base_models();m++)
      P += distribution()[m] * transition_p(t,0,m);
    return P;
  }

  MultiModelObject::MultiModelObject()
  { }

  MultiModelObject::MultiModelObject(int n)
    :base_models(n),
     fraction(n)
  { }

  Matrix frequency_matrix(const MultiModelObject& M) {
    Matrix f(M.n_base_models(),M.n_states());
    for(int m=0;m<f.size1();m++)
      for(int l=0;l<f.size2();l++)
	f(m,l) = M.base_model(m).frequencies()[l];
    return f;
  }


}
