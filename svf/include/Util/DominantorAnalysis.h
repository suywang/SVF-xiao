//===- DominatorAnalysis.h -- Dominant analysis of CFG---------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2017>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

/*
 * DominatorAnalysis.h
 *
 *  Created on: Aug 28, 2024
 *      Author: Xiao Cheng
 */

#ifndef SVF_DOMINANTORANALYSIS_H
#define SVF_DOMINANTORANALYSIS_H

#include "Graphs/GraphPrinter.h"
#include "SVFIR/SVFType.h"
#include "Util/Casting.h"

namespace SVF
{

class SVFFunction;
class SVFBasicBlock;

/**
 * @class DominatorAnalysis
 * @brief This class provides functionality to analyze dominators in a control flow graph of a function.
 *
 * The DominatorAnalysis class is designed to compute dominators, post-dominators, and dominator frontiers
 * for a given function. It serves as a fundamental tool for understanding control flow relationships within
 * a program, which is essential for various program analysis tasks such as optimization and verification.
 */
class DominatorAnalysis
{
public:
    // Type aliases for convenience and readability
    typedef Set<const SVFBasicBlock*> BBSet; // Set of basic blocks
    typedef std::vector<const SVFBasicBlock*> BBList; // List of basic blocks
    typedef Map<const SVFBasicBlock*, BBSet> DominatorMap; // Map from a basic block to a set of basic blocks it dominates

private:
    const SVFFunction* func; // Pointer to the function being analyzed

    /**
     * @brief Computes the dominators using a classic algorithm.
     */
    void computeDominators_classic();

    /**
     * @brief Computes the dominators using an optimized or alternative algorithm.
     */
    void computeDominators();

    /**
     * @brief Computes the post-dominators for the function.
     */
    void computePostDominators();

    /**
     * @brief Computes the dominator frontier for the function.
     */
    void computeDominatorFrontier();

    /**
     * @brief Dumps the dominator map to some output for debugging or analysis.
     *
     * @param mp The dominator map to be dumped.
     */
    void dumpDominatorMap(const DominatorMap& mp);

    /**
     * @brief Compares two dominator maps for differential testing.
     *
     * @param llvmMap The dominator map computed by LLVM.
     * @param svfMap The dominator map computed by SVF.
     */
    void differentialTesting(const DominatorMap& llvmMap, const DominatorMap& svfMap);

public:
    /**
     * @brief Constructor initializing the analysis with the given function.
     *
     * @param function Pointer to the function to be analyzed.
     */
    DominatorAnalysis(const SVFFunction* function) : func(function) {}

    /**
     * @brief Public method to perform the analysis.
     *
     * This method orchestrates the analysis process by calling the dominator computation methods.
     */
    void analyze();
};

} // namespace SVF
#endif // SVF_DOMINANTORANALYSIS_H
