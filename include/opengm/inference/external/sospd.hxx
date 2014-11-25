#ifndef OPENGM_EXTERNAL_SOSPD_HXX_
#define OPENGM_EXTERNAL_SOSPD_HXX_

#include <limits>

#include <opengm/graphicalmodel/graphicalmodel.hxx>
#include <opengm/inference/inference.hxx>
#include <opengm/operations/minimizer.hxx>
#include <opengm/inference/visitors/visitors.hxx>
#include <opengm/utilities/metaprogramming.hxx>

#include <sospd.hpp>

namespace opengm {
namespace external {

template<class GM>
class SOSPD : public Inference<GM, opengm::Minimizer> {
public:
   typedef GM                              GraphicalModelType;
   typedef opengm::Minimizer               AccumulationType;
   OPENGM_GM_TYPE_TYPEDEFS;
   typedef visitors::VerboseVisitor<SOSPD<GM> > VerboseVisitorType;
   typedef visitors::EmptyVisitor<SOSPD<GM> >   EmptyVisitorType;
   typedef visitors::TimingVisitor<SOSPD<GM> >  TimingVisitorType;

   struct Parameter {
      Parameter() : maxIter_(std::numeric_limits<int>::max()),
            setExpansionSubmodular_(false), setLowerBound_(true),
            setProposalAlphaExpansion_(false),
            setProposalHeightAlphaExpansion_(false) {

      }

      int  maxIter_;                         // Maximum number of iterations.
      bool setExpansionSubmodular_;          // Give hint that energy is expansion submodular.
      bool setLowerBound_;                   // Choose whether to use lower/upper bound in approximating function.
      bool setProposalAlphaExpansion_;       // Set the proposal method to alpha-expansion.
      bool setProposalHeightAlphaExpansion_; // Set the proposal method to best-height alpha-expansion.
   };

   // construction
   SOSPD(const GraphicalModelType& gm, const Parameter para = Parameter());
   // destruction
   ~SOSPD();
   // query
   std::string name() const;
   const GraphicalModelType& graphicalModel() const;
   // inference
   InferenceTermination infer();
   template<class VISITOR>
   InferenceTermination infer(VISITOR & visitor);
   InferenceTermination arg(std::vector<LabelType>& arg, const size_t& n = 1) const;
   typename GM::ValueType bound() const;
   typename GM::ValueType value() const;

protected:
   // typedef
   typedef ::SOSPD::SOSPD_REAL   SOSPDValueType;
   typedef ::SOSPD::MultilabelEnergy::VarId SOSPDIndexType;
   typedef ::SOSPD::MultilabelEnergy::Label SOSPDLabelType;

   // wrapper for openGM factors
   class SOSPDFactorWrapper : public ::SOSPD::Clique {
   public:
      // constructor
      SOSPDFactorWrapper(const GraphicalModelType& gm, const IndexType openGMFactorID);

      // destructor
      virtual ~SOSPDFactorWrapper();

      // SOSPD clique interface
      virtual SOSPDValueType        energy(const SOSPDLabelType* labels) const;
      virtual const SOSPDIndexType* nodes () const;
      virtual size_t                size () const;
   protected:
      // protected member
      const GraphicalModelType& gm_;
      const IndexType           openGMFactorID_;
      SOSPDIndexType*           variables_;
   };

   // protected member
   const GraphicalModelType& gm_;
   Parameter                 parameter_;
   bool                      inferenceStarted_;

   const SOSPDLabelType      numLabels_;
   ::SOSPD::MultilabelEnergy sospdModel_;
   ::SOSPD::SoSPD*           sospdSolver_;

   bool sameNumberOfLabels() const;
};

/***********************
 * class documentation *
 ***********************/
//TODO add documentation

/******************
 * implementation *
 ******************/
template<class GM>
inline SOSPD<GM>::SOSPD(const GraphicalModelType& gm, const Parameter para)
: gm_(gm), parameter_(para), inferenceStarted_(false),
  numLabels_(static_cast<const SOSPDLabelType>(gm_.numberOfLabels(0))),
  sospdModel_(numLabels_), sospdSolver_(NULL) {
   OPENGM_ASSERT(std::numeric_limits<SOSPDLabelType>::max() >= std::numeric_limits<LabelType>::max());
   OPENGM_ASSERT(sameNumberOfLabels()); // SOSPD::MultilabelEnergy supports only models where every variable has the same number of labels.

   // add variables to the model
   OPENGM_ASSERT(std::numeric_limits<SOSPDIndexType>::max() >= gm_.numberOfVariables());
   sospdModel_.addVar(gm_.numberOfVariables());

   // add factors
   for(IndexType i = 0; i < gm_.numberOfFactors(); ++i) {
      if(gm_[i].numberOfVariables() == 0) {
         // constant factor
         const LabelType l = 0;
         sospdModel_.addConstantTerm(static_cast<SOSPDValueType>(gm_[i](&l)));
      } else if(gm_[i].numberOfVariables() == 1) {
         // unary factor
         std::vector<SOSPDValueType> factorValues(numLabels_);
         LabelType label = 0;
         for(LabelType j = 0; j < numLabels_; ++j) {
            factorValues[j] = static_cast<SOSPDValueType>(gm_[i](&label));
            ++label;
         }
         sospdModel_.addUnaryTerm(gm_[i].variableIndex(0), factorValues);
      } else if(gm_[i].isPotts() && gm_[i].numberOfVariables() == 2) {
         // add potts factor

         /* TODO SOSPD Potts factor uses a template parameter to set the number
         of variables. Hence the number of variables of the potts factor has to
         be known at compile time. To support SOSPD potts factors with more then
         two variables this has to be added by hand in the source code here.*/

         // get values for same labels and different labels
         SOSPDValueType valueEqual;
         SOSPDValueType valueNotEqual;

         std::vector<LabelType> labeling(gm_[i].numberOfVariables(), 0);
         valueEqual = static_cast<SOSPDValueType>(gm_[i](labeling.begin()));
         for(IndexType j = 0; j < gm_[i].numberOfVariables(); ++j) {
            if(gm_[i].numberOfLabels(j) > 1) {
               labeling[j] = 1;
               break;
            }
         }
         valueNotEqual = static_cast<SOSPDValueType>(gm_[i](labeling.begin()));

         // get variables
         std::vector<SOSPDIndexType> variables(gm_[i].numberOfVariables());
         for(IndexType j = 0; j < gm_[i].numberOfVariables(); ++j) {
            variables[j] = static_cast<SOSPDIndexType>(gm_[i].variableIndex(j));
         }

         // add factor to model
         if(gm_[i].numberOfVariables() == 2) {
            sospdModel_.addClique(::SOSPD::MultilabelEnergy::CliquePtr(new ::SOSPD::PottsClique<2>(variables, valueEqual, valueNotEqual)));
         } else {
            throw RuntimeError("Unsupported number of variables for a SOSPD potts factor.");
         }
      } else {
         // add factor via factor wrapper
         sospdModel_.addClique(::SOSPD::MultilabelEnergy::CliquePtr(new SOSPDFactorWrapper(gm_, i)));
      }
   }

   // create solver
   sospdSolver_ = new ::SOSPD::SoSPD(&sospdModel_);

   // set SOSPD options
   if(parameter_.setExpansionSubmodular_) {
      sospdSolver_->SetExpansionSubmodular(true);
   }
   sospdSolver_->SetLowerBound(parameter_.setLowerBound_);
   if(parameter_.setProposalAlphaExpansion_) {
      sospdSolver_->SetAlphaExpansion();
   }
   if(parameter_.setProposalHeightAlphaExpansion_) {
      sospdSolver_->SetHeightAlphaExpansion();
   }
}

template<class GM>
inline SOSPD<GM>::~SOSPD() {
   if(sospdSolver_) {
      delete sospdSolver_;
   }
}

template<class GM>
inline std::string SOSPD<GM>::name() const {
   return "SOSPD";
}

template<class GM>
inline const typename SOSPD<GM>::GraphicalModelType& SOSPD<GM>::graphicalModel() const {
   return gm_;
}

template<class GM>
inline InferenceTermination SOSPD<GM>::infer() {
   EmptyVisitorType visitor;
   return this->infer(visitor);
}

template<class GM>
template<class VISITOR>
inline InferenceTermination SOSPD<GM>::infer(VISITOR & visitor) {
   visitor.begin(*this);

   inferenceStarted_ = true;
   // call solver
   if(opengm::meta::Compare<VISITOR, EmptyVisitorType>::value) {
      sospdSolver_->Solve(parameter_.maxIter_);
   } else {
      for(int i = 0; i < parameter_.maxIter_; ++i) {
         sospdSolver_->Solve(1);
         if(visitor(*this) != visitors::VisitorReturnFlag::ContinueInf){
            // Cancel further inference
            break;
         }
      }
   }

   visitor.end(*this);
   return NORMAL;
}

template<class GM>
inline InferenceTermination SOSPD<GM>::arg(std::vector<LabelType>& arg, const size_t& n) const {
   if(n > 1) {
      return UNKNOWN;
   }
   else {
      arg.resize(gm_.numberOfVariables());
      if(inferenceStarted_) {
         for(IndexType i = 0; i < gm_.numberOfVariables(); ++i) {
            arg[i] = static_cast<LabelType>(sospdSolver_->GetLabel(i));
         }
      } else {
         for(IndexType i = 0; i < gm_.numberOfVariables(); ++i) {
            arg[i] = 0;
         }
      }
      return NORMAL;
   }
}

template<class GM>
inline typename GM::ValueType SOSPD<GM>::bound() const {
   return opengm::Minimizer::template ineutral<ValueType>();
   /*
   if(inferenceStarted_) {
      return static_cast<ValueType>(sospdSolver_->LowerBound());
   } else {
      return opengm::Minimizer::template ineutral<ValueType>();
   }
   */
}

template<class GM>
inline typename GM::ValueType SOSPD<GM>::value() const {
   if(inferenceStarted_) {
      std::vector<SOSPDLabelType> arg(gm_.numberOfVariables());
      for(SOSPDIndexType i = 0; i < gm_.numberOfVariables(); ++i) {
         arg[i] = sospdSolver_->GetLabel(i);
      }
      return static_cast<ValueType>(sospdModel_.computeEnergy(arg));
   } else {
      return opengm::Minimizer::template neutral<ValueType>();
   }
}

template<class GM>
inline bool SOSPD<GM>::sameNumberOfLabels() const {
   const LabelType numLabels = gm_.numberOfLabels(0);
   for(IndexType i = 1; i < gm_.numberOfVariables(); ++i) {
      if(gm_.numberOfLabels(i) != numLabels) {
         return false;
      }
   }
   return true;
}

template<class GM>
inline SOSPD<GM>::SOSPDFactorWrapper::SOSPDFactorWrapper(const GraphicalModelType& gm, const IndexType openGMFactorID)
   : gm_(gm), openGMFactorID_(openGMFactorID), variables_(new SOSPDIndexType[gm_[openGMFactorID_].numberOfVariables()]) {
   // fill variables
   for(IndexType i = 0; i < gm_[openGMFactorID_].numberOfVariables(); ++i) {
      variables_[i] = static_cast<SOSPDIndexType>(gm_[openGMFactorID_].variableIndex(i));
   }
}

template<class GM>
inline SOSPD<GM>::SOSPDFactorWrapper::~SOSPDFactorWrapper(){
   delete[] variables_;
}

template<class GM>
inline typename SOSPD<GM>::SOSPDValueType SOSPD<GM>::SOSPDFactorWrapper::energy(const SOSPDLabelType* labels) const {
   return static_cast<SOSPDValueType>(gm_[openGMFactorID_](labels));
}

template<class GM>
inline const typename SOSPD<GM>::SOSPDIndexType* SOSPD<GM>::SOSPDFactorWrapper::nodes () const {
   return variables_;
}

template<class GM>
inline size_t SOSPD<GM>::SOSPDFactorWrapper::size () const {
   return static_cast<size_t>(gm_[openGMFactorID_].numberOfVariables());
}

} // namespace external
} // namespace opengm

#endif /* OPENGM_EXTERNAL_SOSPD_HXX_ */
