#include "rates.H"
#include "smodel/operations.H"
#include "distribution-operations.H"
#include "../operations.H"

using boost::shared_ptr;
using std::vector;
using std::valarray;
using std::string;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

namespace substitution
{
  using namespace probability;

  shared_ptr<ExchangeModelObject> SimpleExchangeFunction(double rho, int n)
  {
    shared_ptr<ExchangeModelObject> R (new ExchangeModelObject(n));

    for(int i=0;i<n;i++) {
      for(int j=0;j<n;j++)
	R->S(i,j) = rho;

      R->S(i,i) = 0;       // this is NOT a rate away.
    }

    return R;
  }

  shared_ptr<ExchangeModelObject> EQU_Exchange_Function(const alphabet& a)
  {
    shared_ptr<ExchangeModelObject> R ( new ExchangeModelObject(a.size()) );

    // Calculate S matrix
    for(int i=0;i<a.size();i++)
      for(int j=0;j<a.size();j++)
	R->S(i,j) = 1;

    return R;
  }

  shared_ptr<AlphabetExchangeModelObject> HKY_Function(const Nucleotides& a, double kappa)
  {
    assert(a.size()==4);

    shared_ptr<AlphabetExchangeModelObject> R ( new AlphabetExchangeModelObject(a) );

    for(int i=0;i<a.size();i++)
      for(int j=0;j<a.size();j++) {
	if (i==j) continue;
	if (a.transversion(i,j))
	  R->S(i,j) = 1;
	else
	  R->S(i,j) = kappa;
      }

    return R;
  }

  formula_expression_ref HKY_Model(const alphabet& a)
  {
    expression_ref HKY = lambda_expression(HKY_Op());

    formula_expression_ref kappa = def_parameter("HKY::kappa", 2.0, lower_bound(0.0), log_laplace_dist, Tuple(2)(log(2), 0.25));

    return HKY(a)(kappa);
  }
  
  shared_ptr<ExchangeModelObject> TN_Function(const Nucleotides& a, double kappa1, double kappa2)
  {
    assert(a.size()==4);
  
    shared_ptr<AlphabetExchangeModelObject> R ( new AlphabetExchangeModelObject(a) );
  
    for(int i=0;i<a.size();i++)
      for(int j=0;j<a.size();j++) {
	if (i==j) continue;
	if (a.transversion(i,j))
	  R->S(i,j) = 1;
	else if (a.purine(i))
	  R->S(i,j) = kappa1;
	else
	  R->S(i,j) = kappa2;
      }
  
    return R;
  }

  formula_expression_ref TN_Model(const alphabet& a)
  {
    formula_expression_ref kappa1 = def_parameter("TN::kappa(pur)", 2.0, lower_bound(0.0), log_laplace_dist, Tuple(2)(log(2), 0.25));
    formula_expression_ref kappa2 = def_parameter("TN::kappa(pyr)", 2.0, lower_bound(0.0), log_laplace_dist, Tuple(2)(log(2), 0.25));

    expression_ref TN = lambda_expression(TN_Op());

    return TN(a)(kappa1)(kappa2);
  }
  
  /*
   * OK, so an INV model can be see as one of two things.  It can be an additional rate (e.g. 0) to run
   * an underlying model at. Or, it can be seen as an additional rate to run every model in a mixture at.
   * 
   * Finally, we note that the ability to use + expressions to specify models is in fact a a problem
   * for a different sub-language.  We are really interested in how to specify models in the real
   * sub-language - the more expressive one.  For that, we need to make the handling of discrete distributions
   * more accurate.  That is, we need to (for example) easily be able to add categories to discrete distributions.
   * We can then take a "Gamma()" distribution, and add an extra category 0 to that w/ weight p.
   *   For the + expressions in setup-smodel its OK to hack in a gammaINV model, temporarily.
   */

  shared_ptr<AlphabetExchangeModelObject> INV_Exchange_Function(const alphabet& a,int n)
  {
    shared_ptr<AlphabetExchangeModelObject> R ( new AlphabetExchangeModelObject(a,n) );

    // Calculate S matrix
    for(int i=0;i<R->n_states();i++)
      for(int j=0;j<R->n_states();j++)
	R->S(i,j) = 0;

    return R;
  }

  shared_ptr<AlphabetExchangeModelObject> INV_for_Mixture_Function(const shared_ptr<const MultiModelObject>& M)
  {
    shared_ptr<AlphabetExchangeModelObject> R = INV_Exchange_Function(*M->base_models[0]->get_alphabet(), M->base_models[0]->n_states());

    return R;
  }

  expression_ref INV_for_Mixture = lambda_expression( INV_for_Mixture_Op() );

  formula_expression_ref WithINV_Model(const formula_expression_ref& R)
  {
    typed_expression_ref<Double> p = parameter("INV::p");
    formula_expression_ref P = def_parameter("INV::p", 1.0, between(0.0, 1.0), beta_dist, Tuple(2)(1.0, 2.0) );

    // Where do we get our frequencies from?
    formula_expression_ref INV = INV_for_Mixture(R);
    expression_ref F;
    //    INV = Q_from_S_and_R(INV,F);
    INV = Unit_Mixture(Unit_Collection(INV));

    return Mixture_E(Cons(1.0-p,Cons(P,ListEnd)), Cons(R,Cons(INV,ListEnd)));
  }


  shared_ptr<AlphabetExchangeModelObject> GTR_Function(const Nucleotides& a, 
						       double AG, double AT, double AC,
						       double GT, double GC, 
						       double TC)
  {
    assert(a.size()==4);

    shared_ptr<AlphabetExchangeModelObject> R ( new AlphabetExchangeModelObject(a) );

    double total = AG + AT + AC + GT + GC + TC;

    R->S(0,1) = AG/total;
    R->S(0,2) = AT/total;
    R->S(0,3) = AC/total;

    R->S(1,2) = GT/total;
    R->S(1,3) = GC/total;

    R->S(2,3) = TC/total;

    return R;
  }

  formula_expression_ref GTR_Model(const alphabet& a)
  {
    formula_expression_ref AG = def_parameter("GTR::AG", 2.0/8, between(0.0,1.0));
    formula_expression_ref AT = def_parameter("GTR::AT", 1.0/8, between(0.0,1.0));
    formula_expression_ref AC = def_parameter("GTR::AC", 1.0/8, between(0.0,1.0));
    formula_expression_ref GT = def_parameter("GTR::GT", 1.0/8, between(0.0,1.0));
    formula_expression_ref GC = def_parameter("GTR::GC", 1.0/8, between(0.0,1.0));
    formula_expression_ref TC = def_parameter("GTR::TC", 2.0/8, between(0.0,1.0));

    expression_ref GTR = lambda_expression(GTR_Op());
    
    formula_expression_ref R = GTR(a)(AG)(AT)(AC)(GT)(GC)(TC);

    // I should generalize this...
    // Should I make a tuple of tuples?
    R.add_expression(distributed(Tuple(6)(AG)(AT)(AC)(GT)(GC)(TC),
				 Tuple(2)(dirichlet_dist, 
					  Tuple(6)(8.0)(4.0)(4.0)(4.0)(4.0)(8.0)
					  )
				 )
		     );
    
    return R;
  }
  
  shared_ptr<AlphabetExchangeModelObject> M0_Function(const Codons& C, const ExchangeModelObject& S2,double omega)
  {
    shared_ptr<AlphabetExchangeModelObject> R ( new AlphabetExchangeModelObject(C) );
    ublas::symmetric_matrix<double>& S = R->S;

    for(int i=0;i<C.size();i++) 
    {
      for(int j=0;j<i;j++) {
	int nmuts=0;
	int pos=-1;
	for(int p=0;p<3;p++)
	  if (C.sub_nuc(i,p) != C.sub_nuc(j,p)) {
	    nmuts++;
	    pos=p;
	  }
	assert(nmuts>0);
	assert(pos >= 0 and pos < 3);

	double rate=0.0;

	if (nmuts == 1) {

	  int l1 = C.sub_nuc(i,pos);
	  int l2 = C.sub_nuc(j,pos);
	  assert(l1 != l2);

	  rate = S2(l1,l2);

	  if (C.translate(i) != C.translate(j))
	    rate *= omega;	
	}

	S(i,j) = S(j,i) = rate;
      }
    }

    return R;
  }


  expression_ref M0E = lambda_expression( M0_Op() );

  shared_ptr<AlphabetExchangeModelObject> SingletToTripletExchangeFunction(const Triplets& T, const ExchangeModelObject& S2)
  {
    shared_ptr<AlphabetExchangeModelObject> R ( new AlphabetExchangeModelObject(T) );
    ublas::symmetric_matrix<double>& S = R->S;

    for(int i=0;i<T.size();i++)
      for(int j=0;j<i;j++) 
      {
	int nmuts=0;
	int pos=-1;
	for(int p=0;p<3;p++)
	  if (T.sub_nuc(i,p) != T.sub_nuc(j,p)) {
	    nmuts++;
	    pos=p;
	  }
	assert(nmuts>0);
	assert(pos >= 0 and pos < 3);
	
	S(i,j) = 0;

	if (nmuts == 1) {

	  int l1 = T.sub_nuc(i,pos);
	  int l2 = T.sub_nuc(j,pos);
	  assert(l1 != l2);

	  S(i,j) = S2(l1,l2);
	}
      }

    return R;
  }

  shared_ptr<ReversibleFrequencyModelObject> Plus_gwF_Function(const alphabet& a, double f, const vector<double>& pi)
  {
    assert(a.size() == pi.size());

    shared_ptr<ReversibleFrequencyModelObject> R( new ReversibleFrequencyModelObject(a) );

    // compute frequencies
    R->pi = pi;
    normalize(R->pi);
    
    // compute transition rates
    valarray<double> pi_f(a.size());
    for(int i=0;i<a.size();i++)
      pi_f[i] = pow(R->pi[i],f);

    for(int i=0;i<a.size();i++)
      for(int j=0;j<a.size();j++)
	R->R(i,j) = pi_f[i]/R->pi[i] * pi_f[j];

    // diagonal entries should have no effect
    for(int i=0;i<a.size();i++)
      R->R(i,i) = 0;

    return R;
  }

  expression_ref Plus_gwF(const alphabet& a)
  {
    return lambda_expression( Plus_gwF_Op(a) );
  }

  formula_expression_ref Frequencies_Model(const alphabet& a, const valarray<double>& pi)
  {
    formula_expression_ref F = Tuple(a.size());
    for(int i=0;i<a.size();i++)
    {
      string pname = string("pi") + a.letter(i);
      formula_expression_ref Var  = def_parameter(pname, pi[i], between(0,1));
      F = F(Var);
    }

    expression_ref N = get_tuple(vector<double>(a.size(), 1.0) );
    F.add_expression( distributed( F, Tuple(2)(dirichlet_dist,N ) ) );

    return F;
  }

  formula_expression_ref Frequencies_Model(const alphabet& a)
  {
    valarray<double> pi (1.0/a.size(), a.size());
    return Frequencies_Model(a, pi);
  }

  // Improvement: make all the variables ALSO be a formula_expression_ref, containing their own bounds, etc.
  formula_expression_ref Plus_F_Model(const alphabet& a, const valarray<double>& pi)
  {
    assert(a.size() == pi.size());
    
    formula_expression_ref Vars = Frequencies_Model(a,pi);

    return Plus_gwF(a)(1.0)(Vars);
  }

  formula_expression_ref Plus_F_Model(const alphabet& a)
  {
    valarray<double> pi (1.0/a.size(), a.size());
    return Plus_gwF_Model(a,pi);
  }

  // Improvement: make all the variables ALSO be a formula_expression_ref, containing their own bounds, etc.
  formula_expression_ref Plus_gwF_Model(const alphabet& a, const valarray<double>& pi)
  {
    assert(a.size() == pi.size());
    
    formula_expression_ref f = def_parameter("f", 1.0, between(0,1), uniform_dist, Tuple(2)(0.0, 1.0));

    formula_expression_ref Vars = Frequencies_Model(a,pi);

    return Plus_gwF(a)(f)(Vars);
  }

  formula_expression_ref Plus_gwF_Model(const alphabet& a)
  {
    valarray<double> pi (1.0/a.size(), a.size());
    return Plus_gwF_Model(a,pi);
  }

  shared_ptr<ReversibleMarkovModelObject> Q_from_S_and_R_Function(const ExchangeModelObject& S, const ReversibleFrequencyModelObject& F)
  {
    shared_ptr<ReversibleMarkovModelObject> R ( new ReversibleMarkovModelObject(F.Alphabet()) );

    R->Alphabet();
    // This doesn't work for Modulated markov models
    assert(F.n_states() == F.Alphabet().size());

    // The exchange model and the frequency model should have the same number of states, if not the same alphabet
    assert(S.size() == F.n_states());

    const unsigned N = F.n_states();

    // recompute rate matrix
    Matrix& Q = R->Q;

    for(int i=0;i<N;i++) {
      double sum=0;
      for(int j=0;j<N;j++) {
	if (i==j) continue;
	Q(i,j) = S(i,j) * F(i,j);
	sum += Q(i,j);
      }
      Q(i,i) = -sum;
    }

    R->invalidate_eigensystem();
    
    R->pi = F.pi;

    return R;
  }

  expression_ref Q_from_S_and_R = lambda_expression( Q_from_S_and_R_Op() );

  formula_expression_ref Reversible_Markov_Model(const formula_expression_ref& FS, const formula_expression_ref& FR)
  {
    formula_expression_ref S = prefix_formula("S",FS);
    formula_expression_ref R = prefix_formula("R",FR);
    
    return Q_from_S_and_R(S)(R);
  }

  formula_expression_ref Simple_gwF_Model(const formula_expression_ref& S, const alphabet& a)
  {
    return Reversible_Markov_Model(S,Plus_gwF_Model(a));
  }

  formula_expression_ref Simple_gwF_Model(const formula_expression_ref& S, const alphabet& a, const valarray<double>& pi)
  {
    return Reversible_Markov_Model(S,Plus_gwF_Model(a,pi));
  }

  shared_ptr<ReversibleAdditiveCollectionObject>
  Unit_Collection_Function(const shared_ptr<const ReversibleAdditiveObject>& O)
  {
    return shared_ptr<ReversibleAdditiveCollectionObject>(new ReversibleAdditiveCollectionObject(* O) );
  }

  expression_ref Unit_Collection = lambda_expression( Unit_Collection_Op() );

  shared_ptr<MultiModelObject>
  Unit_Mixture_Function(const shared_ptr<const ReversibleAdditiveCollectionObject>& O)
  {
    shared_ptr<MultiModelObject> R (new MultiModelObject);

    // set the distribution to 1.0
    R->fraction.resize(1);
    R->fraction[0] = 1;

    // make a copy of the submodel
    R->base_models.resize(1);
    R->base_models[0] = O;

    return R;
  }

  expression_ref Unit_Mixture = lambda_expression( Unit_Mixture_Op() );

  formula_expression_ref Unit_Model(const formula_expression_ref& R)
  {
    formula_expression_ref R2 = R;

    R2 = Unit_Collection(R2);

    R2 = Unit_Mixture(R2);

    return R2;
  }

  boost::shared_ptr<DiscreteDistribution> DiscretizationFunction(const Distribution& D, Int n)
  {
    // Make a discretization - not uniform.
    Discretization d(n,D);

    double ratio = d.scale()/D.mean();
    
    // this used to affect the prior
    //    bool good_enough = (ratio > 1.0/1.5 and ratio < 1.5);

    // problem - this isn't completely general
    d.scale(1.0/ratio);
    
    boost::shared_ptr<DiscreteDistribution> R( new DiscreteDistribution(n) );
    R->fraction = d.f;

    for(int i=0;i<n;i++)
    {
      Double V = d.r[i];
      R->values[i] = boost::shared_ptr<Double>( V.clone() );
    }

    return R;
  }

  shared_ptr<DiscreteDistribution> ExtendDiscreteDistributionFunction(const DiscreteDistribution& D,const expression_ref& V, const Double& p)
  {
    shared_ptr<DiscreteDistribution> D2(new DiscreteDistribution(D.size()+1));
    for(int i=0;i<D.size();i++)
    {
      D2->fraction[i] = D.fraction[i]*(1.0-p);
      D2->values[i] = D.values[i];
    }

    D2->fraction[D.size()] = p;
    D2->values[D.size()] = V;

    return D2;
  }

  expression_ref ExtendDiscreteDistribution = lambda_expression( ExtendDiscreteDistributionOp() );

  expression_ref Discretize = lambda_expression( DiscretizationOp() );

  shared_ptr<MultiModelObject> MultiParameterFunction(const ModelFunction& F, const DiscreteDistribution& D)
  {
    shared_ptr<MultiModelObject> R;

    for(int i=0;i<D.fraction.size();i++)
    {
      shared_ptr<const MultiModelObject> M = dynamic_pointer_cast<const MultiModelObject>(F(D.values[i]));

      if (not R) R = shared_ptr<MultiModelObject>(new MultiModelObject);
      
      for(int j=0;j<M->n_base_models();j++)
      {
	R->fraction.push_back( D.fraction[i] * M->distribution()[j] );
	R->base_models.push_back( const_ptr( M->base_model(j) ) );
      }
    }

    return R;
  }

  MultiModelObject MultiRateFunction(const MultiModelObject& M_, const DiscreteDistribution& D)
  {
    shared_ptr<MultiModelObject> M = ptr(M_);

    int N = M->n_base_models() * D.size();

    MultiModelObject R;

    // recalc fractions and base models
    R.resize(N);

    for(int m=0;m<R.n_base_models();m++) 
    {
      int i = m / M->n_base_models();
      int j = m % M->n_base_models();

      R.fraction[m] = D.fraction[i]*M->distribution()[j];

      Double value = dynamic_cast<const Double&>(*D.values[i]);
      M->set_rate( value );

      R.base_models[m] = ptr( M->base_model(j) );
    }

    return R;
  }

  expression_ref MultiParameter = lambda_expression( MultiParameterOp() );

  expression_ref MultiRate = lambda_expression( MultiRateOp() );

  // We want Q(mi -> mj) = Q[m](i -> j)   for letter exchange
  //         Q(mi -> ni) = R(m->n)        for model exchange
  // and     Q(mi -> nj) = 0              for all other pairs

  // We assume that R(m->n) = S(m,n) * M->distribution()[n]

  // This should result in a Markov chain where the frequencies are
  //  frequencies()[mi] = pi[i] * f[m] 
  // with pi = M->frequencies() 
  // and   f = M->distribution()

  // PROBLEM: I don't have a good way of defining the switching rate.
  // Right now, I have S(m,n) = rho, S(m,m) = 0
  // But, the S(m,n) do not correspond to switching rates exactly.
  // Instead, the switching rate is now rho*f[n], which is going to
  // be something like rho*(n-1)/n if there are n categories.
  
  // ADDITIONALLY, depending on how fine-grained the categories are,
  // a switching rate has a different interpretation.

  // HOWEVER, I think the current approach works for now, because it
  // approximates the model that at rate 'rho' the rate is randomly
  // re-drawn from the underlying distribution.  A lot of the time it
  // will fall in the same bin, giving a lower observed switching rate
  // when the discrete approximation to the continuous distribution has
  // low resolution.

  shared_ptr<ReversibleMarkovModelObject> Modulated_Markov_Function(const ExchangeModelObject& S,MultiModelObject M)
  {
    // Make a copy and use this.
    M.set_rate(1);

    unsigned T = 0;
    for(int m=0; m < M.n_base_models(); m++) 
    {
      const ReversibleMarkovModelObject* RM = dynamic_cast<const ReversibleMarkovModelObject*>(&M.base_model(m).part(0));
      if (not RM)
	throw myexception()<<"Can't construct a modulated Markov model from non-Markov model"; // what is the name?
      T += RM->n_states();
    }

    shared_ptr<ReversibleMarkovModelObject> R ( new ReversibleMarkovModelObject(*M.get_alphabet(), T) );

    // calculate the state_letters() map here!

    T = 0;
    for(int m=0; m < M.n_base_models(); m++) 
    {
      unsigned N = M.base_model(m).n_states();
      for(int i=0; i<N; i++)
	R->state_letters_[T+i] = M.base_model(m).state_letters()[i];

      T += N;
    }

    const int n_models = M.n_base_models();


    const valarray<double>& M_pi = M.frequencies();
    const vector<double>&   M_f  = M.distribution();

    // calculate pi[ ] for each state
    T = 0;
    for(int m=0; m < n_models; m++) {
      unsigned N = M.base_model(m).n_states();
      for(int s=0; s < N; s++) 
	R->pi[T+s] = M_pi[s] * M_f[m];
      T += N;
    }
    

    // initially zero out the matrix
    for(int i=0;i<R->Q.size1();i++)
      for(int j=0;j<R->Q.size2();j++)
	R->Q(i,j) = 0;

    // rates for within-model transitions
    T=0;
    for(int m=0; m < n_models; m++) 
    {
      const ReversibleMarkovModelObject* RM = dynamic_cast<const ReversibleMarkovModelObject*>(&M.base_model(m).part(0));
      if (not RM)
	throw myexception()<<"Can't construct a modulated Markov model from non-Markov model"; // what is the name?

      unsigned N = RM->n_states();
      
      for(int s1=0; s1 < N; s1++) 
	for(int s2=0; s2 < N; s2++)
	  R->Q(T+s1,T+s2) = RM->Q(s1,s2);

      T += N;
    }

    // rates for between-model transitions
    unsigned T1=0;
    for(int m1=0; m1 < n_models; m1++) 
    {
      const ReversibleMarkovModelObject* RM1 = dynamic_cast<const ReversibleMarkovModelObject*>(&M.base_model(m1).part(0));
      unsigned N1 = RM1->n_states();

      unsigned T2=0;
      for(int m2=0; m2 < n_models; m2++) 
      {
	const ReversibleMarkovModelObject* RM2 = dynamic_cast<const ReversibleMarkovModelObject*>(&M.base_model(m2).part(0));
	unsigned N2 = RM2->n_states();
	assert(N1 == N2);

	if (m1 != m2) {
	  double S12 = S(m1,m2);
	  for(int s1=0;s1<N1;s1++)
	    R->Q(T1+s1,T2+s1) = S12*M_f[m2];
	}

	T2 += N2;
      }
      T1 += N1;
    }

    // recompute diagonals 
    for(int i=0;i<R->Q.size1();i++) 
    {
      double sum=0;
      for(int j=0;j<R->Q.size2();j++)
	if (i!=j)
	  sum += R->Q(i,j);
      R->Q(i,i) = -sum;
    }

    R->invalidate_eigensystem();

    return R;
  }

  expression_ref Modulated_Markov_E = lambda_expression( Modulated_Markov_Op() );

  shared_ptr< DiscreteDistribution > M2_Function(Double f1, Double f2, Double f3, Double omega)
  {
    shared_ptr< DiscreteDistribution > R ( new DiscreteDistribution(3) );
    R->fraction[0] = f1;
    R->fraction[1] = f2;
    R->fraction[2] = f3;

    R->values[0] = ptr( Double(0) );
    R->values[1] = ptr( Double(1) );
    R->values[2] = ptr( omega );

    return R;
  }

  shared_ptr<const MultiModelObject> Mixture_Function(const expression_ref& DL, const expression_ref& ML)
  {
    vector<expression_ref> D = get_ref_vector_from_list(DL);
    vector<expression_ref> M = get_ref_vector_from_list(ML);
    assert(D.size() == M.size());

    shared_ptr<MultiModelObject> R (new MultiModelObject);

    for(int m=0;m<M.size();m++)
    {
      shared_ptr<const MultiModelObject> MM = dynamic_pointer_cast<const MultiModelObject>(M[m]);

      double w = dynamic_cast<const Double&>(*D[m]);

      for(int i=0;i<MM->n_base_models();i++)
      {
	R->fraction.push_back(w * MM->distribution()[i]);
	R->base_models.push_back(ptr( MM->base_model(i)) );
      }
    }

    return R;
  }

  expression_ref Mixture_E = lambda_expression(Mixture_Op());

  formula_expression_ref Mixture_Model(const vector<formula_expression_ref>& models)
  {
    const int N = models.size();

    formula_expression_ref models_list = ListEnd;
    formula_expression_ref vars_list = ListEnd;
    expression_ref vars_tuple = Tuple(models.size());
    expression_ref n_tuple = Tuple(models.size());
    for(int i=0;i<N;i++)
    {
      string var_name = "Mixture::p"+convertToString(i+1);
      parameter var(var_name);
      formula_expression_ref Var = def_parameter(var_name, 1.0/N, between(0,1)); 

      models_list = Cons(models[i], models_list);
      vars_list = Cons(Var, vars_list);
      vars_tuple = vars_tuple(var);
      n_tuple = n_tuple(1.0);
    }
    formula_expression_ref R= Mixture_E(vars_list, models_list);

    R.add_expression(distributed(vars_tuple, Tuple(2)(dirichlet_dist, n_tuple ) ) );

    return R;
  }
}
