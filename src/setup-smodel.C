/*
   Copyright (C) 2004-2010 Benjamin Redelings

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
/// \file setup-smodel.C
///
/// \brief Create substitution models from strings of the form model+...+model.
///

#include <vector>
#include <boost/program_options.hpp>
#include "setup.H"
#include "smodel/smodel.H"
#include "util.H"
#include "rates.H"
#include "myexception.H"
#include "smodel/operations.H"
#include "distribution-operations.H"

using std::vector;
using std::valarray;
using namespace substitution;
using boost::program_options::variables_map;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;

/// \brief Take a string of the form \a s[\a arg] off the top of \a sstack, if its present
///
/// \param sstack A stack of strings that represent a substitution model.
/// \param s The model name to match.
/// \param args The possible argument.
///
bool match(vector<string>& sstack,const string& s, vector<string>& args) {
  if (not sstack.size())
    return false;
  
  string name = sstack.back();

  args = get_arguments(name,'[',']');

  if (name != s) return false;

  sstack.pop_back();

  return true;
}

/// \brief Take a string of the form \a s[\a arg] off the top of \a sstack, if its present
///
/// \param sstack A stack of strings that represent a substitution model.
/// \param s The model name to match.
/// \param arg A possible argument.
///
bool match(vector<string>& sstack,const string& s,string& arg) 
{
  vector<string> arguments;

  if (not match(sstack,s,arguments)) return false;

  if (arguments.size() == 0)
    arg = "";
  else if (arguments.size() > 0)
    arg = arguments[0];

  return true;
}

/// \brief Return the default substitution model name for alphabet \a a, and "" if there is no default.
string default_markov_model(const alphabet& a) 
{
  if (dynamic_cast<const Nucleotides*>(&a))
    return "TN";
  else if (dynamic_cast<const AminoAcidsWithStop*>(&a))
    return "";
  else if (dynamic_cast<const AminoAcids*>(&a))
    return "LG";
  else if (dynamic_cast<const Codons*>(&a))
    return "M0";
  else if (dynamic_cast<const Triplets*>(&a))
    return "TNx3";
  else
    return "";
}

formula_expression_ref
get_smodel_(const string& smodel,const shared_ptr<const alphabet>& a, const shared_ptr<const valarray<double> >&);

formula_expression_ref
get_smodel_(const string& smodel,const shared_ptr<const alphabet>& a);

formula_expression_ref
get_smodel_(const string& smodel);

/// \brief Construct a model from the top of the string stack
///
/// \param string_stack The list of strings representing the substitution model.
/// \param model_stack The list of models that may be used as components of the current model.
/// \param a The alphabet on which the model lives.
/// \param frequencies The initial frequencies for the model.
///
bool process_stack_Markov(vector<string>& string_stack,
			  vector<formula_expression_ref >& model_stack,
			  const shared_ptr<const alphabet>& a,
			  const shared_ptr<const valarray<double> >& frequencies)
{
  string arg;

  formula_expression_ref R;

  //------ Get the base markov model (Reversible Markov) ------//
  if (match(string_stack,"EQU",arg))
    model_stack.push_back(EQU(*a));

  else if (match(string_stack,"F81",arg))
  {
    if (frequencies)
      model_stack.push_back(F81_Model(*a,*frequencies));
    else
      model_stack.push_back(F81_Model(*a));
  }

  else if (match(string_stack,"HKY",arg)) 
  {
    const Nucleotides* N = dynamic_cast<const Nucleotides*>(&*a);
    if (not N)
      throw myexception()<<"HKY: '"<<a->name<<"' is not a nucleotide alphabet.";

    R = HKY_Model(*a);
  }
  else if (match(string_stack,"TN",arg)) 
  {
    const Nucleotides* N = dynamic_cast<const Nucleotides*>(&*a);
    if (not N)
      throw myexception()<<"TN: '"<<a->name<<"' is not a nucleotide alphabet.";

    R = TN_Model(*a);
  }
  else if (match(string_stack,"GTR",arg)) 
  {
    const Nucleotides* N = dynamic_cast<const Nucleotides*>(&*a);
    if (not N)
      throw myexception()<<"GTR: '"<<a->name<<"' is not a nucleotide alphabet.";

    // FIXME - allow/make a general GTR model!

    R = GTR_Model(*a);
    // model_stack.push_back(GTR(*N));
    //    model_stack.push_back(FormulaModel(model_formula(GTR(*N))));
  }
  /* THINKO - Must tripletmodels be constructed from nucleotide-specific models?
  else if (match(string_stack,"EQUx3",arg)) {
    const Triplets* T = dynamic_cast<const Triplets*>(&*a);
    if (T) 
      model_stack.push_back(SingletToTripletExchangeModel(*T,EQU(T->getNucleotides())));
    else
      throw myexception()<<"EQUx3:: '"<<a->name<<"' is not a triplet alphabet.";
  }
  */      
  else if (match(string_stack,"HKYx3",arg)) 
  {
    const Triplets* T = dynamic_cast<const Triplets*>(&*a);
    if (T) 
      model_stack.push_back(SingletToTripletExchangeModel(*T,HKY(T->getNucleotides())));
    else
      throw myexception()<<"HKYx3: '"<<a->name<<"' is not a triplet alphabet.";
  }
  else if (match(string_stack,"TNx3",arg)) 
  {
    const Triplets* T = dynamic_cast<const Triplets*>(&*a);
    if (T) 
      model_stack.push_back(SingletToTripletExchangeModel(*T,TN(T->getNucleotides())));
    else
      throw myexception()<<"TNx3: '"<<a->name<<"' is not a triplet alphabet.";
  }
  else if (match(string_stack,"GTRx3",arg)) 
  {
    const Triplets* T = dynamic_cast<const Triplets*>(&*a);
    if (T) 
      model_stack.push_back(SingletToTripletExchangeModel(*T,GTR(T->getNucleotides())));
    else
      throw myexception()<<"GTRx3: '"<<a->name<<"' is not a triplet alphabet.";
  }
  else if (match(string_stack,"PAM",arg)) {
    if (*a != AminoAcids())
      throw myexception()<<"PAM: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    model_stack.push_back(PAM());
  }
  else if (match(string_stack,"JTT",arg)) {
    if (*a != AminoAcids())
      throw myexception()<<"JTT: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    model_stack.push_back(JTT());
  }
  else if (match(string_stack,"WAG",arg)) {
    if (*a != AminoAcids())
      throw myexception()<<"WAG: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    model_stack.push_back(WAG());
  }
  else if (match(string_stack,"LG",arg)) {
    if (*a != AminoAcids())
      throw myexception()<<"LG: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    model_stack.push_back(LG());
  }
  else if (match(string_stack,"Empirical",arg)) {
    Empirical M(*a);
    M.load_file(arg);
    model_stack.push_back(M);
  }
  else if (match(string_stack,"C10",arg))
  {
    if (*a != AminoAcids())
      throw myexception()<<"C20: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    model_stack.push_back(C10_CAT_FixedFrequencyModel());
  }
  else if (match(string_stack,"C20",arg))
  {
    if (*a != AminoAcids())
      throw myexception()<<"C20: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    model_stack.push_back(C20_CAT_FixedFrequencyModel());
  }
  else if (match(string_stack,"CAT-Fix",arg)) {
    if (*a != AminoAcids())
      throw myexception()<<"CAT-Fix: '"<<a->name<<"' is not an 'Amino-Acids' alphabet.";
    CAT_FixedFrequencyModel M(*a);
    M.load_file(arg);
    model_stack.push_back(M);
  }

  else if (match(string_stack,"M0",arg)) 
  {
    const Codons* C = dynamic_cast<const Codons*>(&*a);
    if (not C)
      throw myexception()<<"M0: '"<<a->name<<"' is not a 'Codons' alphabet";

    formula_expression_ref N_submodel = HKY(C->getNucleotides());

    if (not arg.empty()) 
    {
      formula_expression_ref submodel = get_smodel_(arg, const_ptr( C->getNucleotides() ) );

      if (not submodel.result_as<AlphabetExchangeModelObject>() or 
	  not dynamic_pointer_cast<const Nucleotides>(submodel.result_as<AlphabetExchangeModelObject>()->get_alphabet()))
	throw myexception()<<"Submodel '"<<arg<<"' for M0 is not a nucleotide replacement model.";

      N_submodel = submodel;
    }

    model_stack.push_back( M0(*C, FormulaModel(N_submodel)) );
  }
  else
    return false;

  if (R.exp())
    model_stack.push_back(FormulaModel(R));
  return true;
}

/// \brief Construct an AlphabetExchangeModel from model \a M
formula_expression_ref get_EM(const formula_expression_ref& R, const string& name)
{
  if (R.result_as<AlphabetExchangeModelObject>())
    return R;

  throw myexception()<<name<<": '"<<R.exp()<<"' is not an exchange model.";
}


/// \brief Construct an AlphabetExchangeModel from the top of the model stack
formula_expression_ref get_EM(vector<formula_expression_ref>& model_stack, const string& name)
{
  if (model_stack.empty())
    throw myexception()<<name<<": Needed an exchange model, but no model was given.";

  return get_EM(model_stack.back(), name);
}

/// \brief Construct an AlphabetExchangeModel from the top of the model stack
formula_expression_ref get_EM_default(vector<formula_expression_ref>& model_stack, 
				      const string& name,
				      const shared_ptr<const alphabet>& a, 
				      const shared_ptr< const valarray<double> >& frequencies)
{
  if (model_stack.empty())
    model_stack.push_back( get_smodel_(default_markov_model(*a), a, frequencies));

  return get_EM(model_stack.back(), name);
}


/// \brief Construct a model from the top of the string stack
///
/// \param string_stack The list of strings representing the substitution model.
/// \param model_stack The list of models that may be used as components of the current model.
/// \param a The alphabet on which the model lives.
/// \param frequencies The initial frequencies for the model.
///
bool process_stack_Frequencies(vector<string>& string_stack,
			       vector<formula_expression_ref >& model_stack,
			       const shared_ptr<const alphabet>& a,
			       const shared_ptr<const valarray<double> >& frequencies)
{
  string arg;
  
  if (match(string_stack,"F=constant",arg)) 
  {
    formula_expression_ref EM = get_EM_default(model_stack,"+F=constant", a, frequencies);

    formula_expression_ref F = Plus_gwF(*a)(1.0)( get_tuple(*frequencies) );

    model_stack.back() = Reversible_Markov_Model(EM,F);
  }
  else if (match(string_stack,"F",arg)) {
    formula_expression_ref EM = get_EM_default(model_stack,"+F", a, frequencies);

    formula_expression_ref F;
    if (frequencies)
      F = Plus_F_Model(*a,*frequencies);
    else
      F = Plus_F_Model(*a);

    model_stack.back() = Reversible_Markov_Model(EM,F);
  }
  else if (match(string_stack,"F=uniform",arg)) 
  {
    formula_expression_ref EM = get_EM_default(model_stack,"+F=uniform", a, frequencies);

    vector<double> pi(a->size(),1.0/a->size() );

    formula_expression_ref F = Plus_gwF(*a)(1.0)( get_tuple( pi ) );

    model_stack.back() = Reversible_Markov_Model(EM,F);
  }
  else if (match(string_stack,"F=nucleotides",arg)) 
  {
    const Triplets* T = dynamic_cast<const Triplets*>(&*a);
    if (not T)
      throw myexception()<<"+F=nucleotides:: '"<<a->name<<"' is not a triplet alphabet.";

    formula_expression_ref EM = get_EM_default(model_stack,"+F=nucleotides", a, frequencies);

    model_stack.back() = Reversible_Markov_Model(EM, IndependentNucleotideFrequencyModel(*T));
  }
  else if (match(string_stack,"F=amino-acids",arg)) 
  {
    const Codons* C = dynamic_cast<const Codons*>(&*a);
    if (not C)
      throw myexception()<<"+F=amino-acids:: '"<<a->name<<"' is not a codon alphabet.";

    formula_expression_ref EM = get_EM_default(model_stack,"+F=amino-acids", a, frequencies);

    model_stack.back() = Reversible_Markov_Model(EM, AACodonFrequencyModel(*C));
  }
  else if (match(string_stack,"F=triplets",arg)) 
  {
    const Triplets* T = dynamic_cast<const Triplets*>(&*a);
    if (not T)
      throw myexception()<<"+F=triplets:: '"<<a->name<<"' is not a triplet alphabet.";

    formula_expression_ref EM = get_EM_default(model_stack,"+F=triplets", a, frequencies);

    model_stack.back() = Reversible_Markov_Model(EM,TripletsFrequencyModel(*T));
  }
  else if (match(string_stack,"F=codons",arg)) 
  {
    const Codons* C = dynamic_cast<const Codons*>(&*a);
    if (not C)
      throw myexception()<<"+F=codons:: '"<<a->name<<"' is not a codon alphabet.";

    formula_expression_ref EM = get_EM_default(model_stack,"+F=codons", a, frequencies);

    model_stack.back() = Reversible_Markov_Model(EM,CodonsFrequencyModel(*C));
  }
  else if (match(string_stack,"F=codons2",arg)) 
  {
    const Codons* C = dynamic_cast<const Codons*>(&*a);
    if (not C)
      throw myexception()<<"+F=codons2:: '"<<a->name<<"' is not a codon alphabet.";

    formula_expression_ref EM = get_EM_default(model_stack,"+F=codons2", a, frequencies);

    model_stack.back() = Reversible_Markov_Model(EM,CodonsFrequencyModel2(*C));
  }
  else
    return false;
  return true;
}

/// \brief Construct a ReversibleMarkovModel from model \a M
formula_expression_ref get_RA(const formula_expression_ref& M, const string& name,
			      const shared_ptr< const valarray<double> >& frequencies)
{
  if (M.result_as<ReversibleMarkovModelObject>())
    return M;

  try {
    formula_expression_ref top = get_EM(M,name);
    shared_ptr<const alphabet> a = get_alphabet( FormulaModel(top) );
    // If the frequencies.size() != alphabet.size(), this call will throw a meaningful exception.
    if (frequencies)
      return Simple_gwF_Model(top, *a, *frequencies); 
    else
      return Simple_gwF_Model(top, *a); 
  }
  catch (std::exception& e) { 
    throw myexception()<<name<<": Can't construct a SimpleReversibleMarkovModel from '"<<M.exp()<<"':\n "<<e.what();
  }
}

/// \brief Construct a ReversibleMarkovModel from the top of the model stack
formula_expression_ref get_RA(vector<formula_expression_ref >& model_stack, 
			      const string& name,
			      const shared_ptr< const valarray<double> >& frequencies)
{
  if (model_stack.empty())
    throw myexception()<<name<<": couldn't find any model to use.";
  
  return get_RA(model_stack.back(), name, frequencies);
}


/// \brief Construct a ReversibleMarkovModel from the top of the model stack
formula_expression_ref get_RA_default(vector<formula_expression_ref >& model_stack, 
				      const string& name,
				      const shared_ptr<const alphabet>& a,
				      const shared_ptr< const valarray<double> >& frequencies)
{
  if (model_stack.empty())
    model_stack.push_back( get_smodel_(default_markov_model(*a), a, frequencies));
  
  return get_RA(model_stack.back(), name, frequencies);
}


/// \brief Construct a MultiModel from model \a M
formula_expression_ref
get_MM(const formula_expression_ref& M, const string& name, const shared_ptr< const valarray<double> >& frequencies)
{
  if (M.result_as<MultiModelObject>())
    return M;

  try { 
    return Unit_Model( get_RA(M,name,frequencies) ) ; 
  }
  catch (std::exception& e) { 
    throw myexception()<<name<<": Can't construct a UnitModel from '"<<M.exp()<<"':\n"<<e.what();
  }
}

/// \brief Construct a MultiModel from the top of the model stack.
formula_expression_ref
get_MM(vector<formula_expression_ref >& model_stack, const string& name,const shared_ptr< const valarray<double> >& frequencies)
{
  if (model_stack.empty())
    throw myexception()<<name<<": Trying to construct a MultiModel, but no model was given.";

  return get_MM(model_stack.back(), name, frequencies);
}

/// \brief Construct a MultiModel from the top of the model stack.
formula_expression_ref
get_MM_default(vector<formula_expression_ref >& model_stack, const string& name, 
	       const shared_ptr<const alphabet>& a, 
	       const shared_ptr< const valarray<double> >& frequencies)
{
  if (model_stack.empty())
    model_stack.push_back( get_smodel_(default_markov_model(*a), a, frequencies));

  return get_MM(model_stack.back(), name, frequencies);
}

bool process_stack_Multi(vector<string>& string_stack,  
			 vector<formula_expression_ref >& model_stack,
			  const shared_ptr<const alphabet>& a,
			 const shared_ptr< const valarray<double> >& frequencies)
{
  string arg;
  vector<string> args;

  if (match(string_stack,"single",arg)) 
    model_stack.back() = get_MM_default(model_stack,"single",a,frequencies);

  else if (match(string_stack,"gamma_plus_uniform",arg)) {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);

    model_stack.back() = MultiRate(get_MM_default(model_stack,"gamma_plus_uniform",a,frequencies),  Discretize(model_formula(Gamma()+Uniform()), expression_ref(n)) );
  }
  else if (match(string_stack,"gamma",arg)) {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);

    formula_expression_ref base = get_MM_default(model_stack,"gamma",a,frequencies);
    formula_expression_ref dist = Discretize(model_formula(Gamma()), expression_ref(n));
    model_stack.back() = MultiRate(base,  dist);
  }
  else if (match(string_stack,"gamma_inv",arg)) {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);

    formula_expression_ref base = get_MM_default(model_stack,"gamma",a,frequencies);
    formula_expression_ref dist = Discretize(model_formula(Gamma()), expression_ref(n));
    formula_expression_ref p = def_parameter("INV::p", 0.01, between(0,1), beta_dist, Tuple(2)(1.0, 2.0) );
    dist = ExtendDiscreteDistribution(dist)(0.0)(p);
    model_stack.back() = MultiRate(base,  dist);
  }
  else if (match(string_stack,"log-normal",arg)) {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);

    model_stack.back() = MultiRate(get_MM_default(model_stack,"log-normal",a,frequencies),  Discretize(model_formula(LogNormal()), expression_ref(n)) );
  }
  else if (match(string_stack,"multi_freq",arg)) {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);

    model_stack.back() = MultiFrequencyModel(FormulaModel(get_EM(model_stack,"multi_freq")),n);
  }
  else if (match(string_stack,"INV",arg))
    model_stack.back() = WithINV( FormulaModel(get_MM_default(model_stack,"INV",a,frequencies)) );
  //    model_stack.back() = WithINV_Model(get_MM_default(model_stack,"INV",a,frequencies));

  else if (match(string_stack,"DP",arg)) {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);
    model_stack.back() = DirichletParameterModel(FormulaModel(get_MM_default(model_stack,"DP",a,frequencies)),-1,n);
  }
  else if (match(string_stack,"Modulated",arg))
  {
    formula_expression_ref MM = get_MM_default(model_stack,"Modulated",a,frequencies);

    int n = MM.result_as<MultiModelObject>()->n_base_models();
    model_stack.back() = ModulatedMarkovModel(FormulaModel(MM),
					      SimpleExchangeModel(n));
  }
  else if (match(string_stack,"Mixture",args)) 
  {
    vector <formula_expression_ref> models;
    for(int i=0;i<args.size();i++)
      models.push_back( get_MM (get_smodel_(args[i], a, frequencies),"Mixture",frequencies ) );

    model_stack.push_back(Mixture_Model(models));
    std::cout<<"Mixture formula = "<<model_stack.back()<<std::endl;
  }
  else if (match(string_stack,"M2",arg)) 
  {
    FormulaModel FM(model_stack.back());
    model_stack.back() = M2(FM, SimpleFrequencyModel(*get_alphabet(FM)));
  }
  else if (match(string_stack,"M2a",arg)) 
  {
    FormulaModel FM(model_stack.back());
    model_stack.back() = M2a(FM, SimpleFrequencyModel(*get_alphabet(FM)));
  }
  else if (match(string_stack,"M8b",arg))
  {
    int n=3;
    if (not arg.empty())
      n = convertTo<int>(arg);

    FormulaModel FM(model_stack.back());
    model_stack.back() = M8b(FM, SimpleFrequencyModel(*get_alphabet(FM)),n);
  }
  else if (match(string_stack,"M3",arg)) 
  {
    int n=3;
    if (not arg.empty())
      n = convertTo<int>(arg);

    FormulaModel FM(model_stack.back());
    model_stack.back() = M3(FM, SimpleFrequencyModel(*get_alphabet(FM)),n); 
  }
  else if (match(string_stack,"M7",arg)) 
  {
    int n=4;
    if (not arg.empty())
      n = convertTo<int>(arg);

    FormulaModel FM(model_stack.back());
    model_stack.back() = M7(FM, SimpleFrequencyModel(*get_alphabet(FM)),n); 
  }
  else
    return false;
  return true;
}

formula_expression_ref 
get_smodel_(const string& smodel,const shared_ptr<const alphabet>& a,const shared_ptr<const valarray<double> >& frequencies) 
{
  // Initialize the string stack from the model name
  vector<string> string_stack;
  if (smodel != "") 
  {
    string_stack = split(smodel,'+');
    std::reverse(string_stack.begin(),string_stack.end());
  }
  else
  {
    string model_name = default_markov_model(*a);
    if (not model_name.size())
      throw myexception()<<"You must specify a substitution model - there is no default substitution model for alphabet '"<<a->name<<"'";
    string_stack.push_back(model_name);
    //    if (not process_stack_Markov(string_stack,model_stack,a,frequencies))
    //      throw myexception()<<"Can't guess the base CTMC model for alphabet '"<<a->name<<"'";
  }


  // Initialize the model stack 
  vector<formula_expression_ref > model_stack;

  //-------- Run the model specification -----------//
  while(string_stack.size()) {
    int length = string_stack.size();

    process_stack_Markov(string_stack, model_stack, a, frequencies);

    process_stack_Frequencies(string_stack, model_stack, a, frequencies);

    process_stack_Multi(string_stack, model_stack, a, frequencies);

    if (string_stack.size() == length)
      throw myexception()<<"Couldn't process substitution model level \""<<string_stack.back()<<"\"";
  }

  //---------------------- Stack should be empty now ----------------------//
  if (model_stack.size()>1) {
    throw myexception()<<"Substitution model "<<model_stack.back()<<" was specified but not used!\n";
  }

  return model_stack.back();
}

formula_expression_ref
get_smodel_(const string& smodel,const shared_ptr<const alphabet>& a)
{
  return get_smodel_(smodel, a, shared_ptr<const valarray<double> >());
}

formula_expression_ref
get_smodel_(const string& smodel)
{
  return get_smodel_(smodel, shared_ptr<const alphabet>(),shared_ptr<const valarray<double> >());
}


/// \brief Constrict a substitution::MultiModel for a specific alphabet
///
/// \param smodel_name The name of the substitution model.
/// \param a The alphabet.
/// \param frequencies The initial letter frequencies in the model.
///
formula_expression_ref
get_smodel(const string& smodel_name, const shared_ptr<const alphabet>& a, const shared_ptr<const valarray<double> >& frequencies) 
{
  assert(frequencies->size() == a->size());

  //------------------ Get smodel ----------------------//
  formula_expression_ref smodel = get_smodel_(smodel_name,a,frequencies);

  // check if the model actually fits alphabet a...

  // --------- Convert smodel to MultiModel ------------//
  formula_expression_ref full_smodel = get_MM(smodel,"Final",frequencies);

  return full_smodel;
}

/// \brief Construct a substitution::MultiModel model for a collection of alignments
///
/// \param smodel_name The name of the substitution model.
/// \param A The alignments.
///
/// This routine constructs the initial frequencies based on all of the alignments.
///
formula_expression_ref get_smodel(const variables_map& args, const string& smodel_name,const vector<alignment>& A) 
{
  for(int i=1;i<A.size();i++)
    if (A[i].get_alphabet() != A[0].get_alphabet())
      throw myexception()<<"alignments in partition don't all have the same alphabet!";

  shared_ptr< const valarray<double> > frequencies (new valarray<double>(empirical_frequencies(args,A)));

  return get_smodel(smodel_name, const_ptr( A[0].get_alphabet() ), frequencies);
}

formula_expression_ref get_smodel(const variables_map& args, const string& smodel_name,const alignment& A) 
{
  shared_ptr< const valarray<double> > frequencies (new valarray<double>(empirical_frequencies(args,A)));

  return get_smodel(smodel_name, const_ptr( A.get_alphabet() ), frequencies);
}

formula_expression_ref get_smodel(const variables_map& args, const alignment& A) 
{
  string smodel_name = args["smodel"].as<string>();

  shared_ptr< const valarray<double> > frequencies (new valarray<double>(empirical_frequencies(args,A)));

  return get_smodel(smodel_name, const_ptr( A.get_alphabet() ), frequencies);
}
