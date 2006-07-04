/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2006 Mark Joshi

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/reference/license.html>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include <ql/MarketModels/accountingengine.hpp>
#include <algorithm>

namespace QuantLib {

    AccountingEngine::AccountingEngine(
        const boost::shared_ptr<MarketModelEvolver>& evolver,
        const boost::shared_ptr<MarketModelProduct>& product,
        const EvolutionDescription& evolution,
        double initialNumeraireValue)
        :
        evolver_(evolver),
        product_(product),
        evolution_(evolution),
        initialNumeraireValue_(initialNumeraireValue),
        numberProducts_(product->numberOfProducts()),      
        numerairesHeld_(product->numberOfProducts()),
        curveState_(evolution.rateTimes()),
        numberCashFlowsThisStep_(product->numberOfProducts()),
        cashFlowsGenerated_(product->numberOfProducts())
    {
        for (Size i = 0; i <numberProducts_; ++i )
            cashFlowsGenerated_[i].resize(
                       product_->maxNumberOfCashFlowsPerProductPerStep());
        
        const Array& cashFlowTimes = product_->possibleCashFlowTimes();
        const Array& rateTimes = evolution_.rateTimes();
        for (Size j = 0; j < cashFlowTimes.size(); ++j)
            discounters_.push_back(Discounter(cashFlowTimes[j], rateTimes));

    }

    void AccountingEngine::singlePathValues(Array& values)
    {
        std::fill(numerairesHeld_.begin(),numerairesHeld_.end(),0.);
        Real weight = evolver_->startNewPath();
        product_->reset();     
        Real principalInNumerairePortfolio = 1.0;
    
        bool done = false;
        do {
            weight *= evolver_->advanceStep();
            done = product_->nextTimeStep(evolver_->currentState(), 
                                          numberCashFlowsThisStep_, 
                                          cashFlowsGenerated_);
            Size numeraire =
                evolution_.numeraires()[evolver_->currentStep()];

            // for each product...
            for (Size i=0; i<numberProducts_; ++i) {
                // ...and each cash flow...
                const std::vector<MarketModelProduct::CashFlow>& cashflows =
                    cashFlowsGenerated_[i];
                for (Size j=0; j<numberCashFlowsThisStep_[i]; ++j) {
                    // ...convert the cash flow to numeraires.
                    // This is done by calculating the number of
                    // numeraire bonds corresponding to such cash flow...
                    const Discounter& discounter =
                        discounters_[cashflows[j].timeIndex];

                    Real bonds = 
                        cashflows[j].amount *
                        discounter.numeraireBonds(evolver_->currentState(),
                                                  numeraire);

                    // ...and adding the newly bought bonds to the number
                    // of numeraires held.
                    numerairesHeld_[i] += bonds/principalInNumerairePortfolio;
                }
            }

            if (!done) {

                // The numeraire might change between steps. This implies
                // that we might have to convert the numeraire bonds for
                // this step into a corresponding amount of numeraire
                // bonds for the next step. This can be done by changing
                // the principal of the numeraire and updating the number
                // of bonds in the numeraire portfolio accordingly.

                Size nextNumeraire =
                    evolution_.numeraires()[evolver_->currentStep()+1];

                principalInNumerairePortfolio *=
                    curveState_.discountRatio(nextNumeraire, numeraire);
            }
            
        }
        while (!done);

        for (Size i=0; i < numerairesHeld_.size(); ++i)
            values[i] = numerairesHeld_[i] * initialNumeraireValue_;

    }



    AccountingEngine::Discounter::Discounter(Time paymentTime,
                                             const Array& rateTimes) {
        // to be implemented
    }

    Real AccountingEngine::Discounter::numeraireBonds(
                                          const CurveState& curveState,
                                          Size numeraire) const {
        // to be implemented
        return 1.0;
    }

}
