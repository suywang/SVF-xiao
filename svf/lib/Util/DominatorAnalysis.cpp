//===- DominatorAnalysis.cpp -- Dominant analysis of CFG---------------------//
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
 * DominatorAnalysis.cpp
 *
 *  Created on: Aug 28, 2024
 *      Author: Xiao Cheng
 */

#include "SVFIR/SVFValue.h"
#include "Util/DominantorAnalysis.h"
#include "Util/SVFUtil.h"
#include "functional"

#define DEBUG 0 /* Turn this on if you're debugging dominator analysis */
#define ERR_MSG(msg)                                                           \
    do                                                                         \
    {                                                                          \
        SVFUtil::errs() << SVFUtil::errMsg("Error ") << __FILE__ << ':'        \
                        << __LINE__ << ": " << (msg) << '\n';                  \
    } while (0)
#define ABORT_MSG(msg)                                                         \
    do                                                                         \
    {                                                                          \
        ERR_MSG(msg);                                                          \
        abort();                                                               \
    } while (0)
#define ABORT_IFNOT(condition, msg)                                            \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
            ABORT_MSG(msg);                                                    \
    } while (0)

using namespace SVF;
using namespace std;

void DominatorAnalysis::analyze()
{
    computeDominators(); // Compute the dominators for the function
    // Uncomment the following lines to compute additional analyses
    computePostDominators(); // Compute the post-dominators
    // computeDominatorFrontier(); // Compute the dominator frontier
}

/**
 * @brief Dumps the dominator map to the standard output for debugging or
 * analysis.
 *
 * This method iterates over the dominator map and prints each basic block along
 * with the set of basic blocks it dominates.
 *
 * @param domMap The dominator map to be dumped.
 */
void DominatorAnalysis::dumpDominatorMap(const DominatorMap& domMap)
{
    for (const auto& entry : domMap)
    {
        const SVFBasicBlock* bb = entry.first; // The current basic block
        const BBSet& bbSet = entry.second; // The set of basic blocks dominated
                                           // by the current basic block

        // Print the name of the current SVFBasicBlock
        cout << bb->getName() << " dominates:" << endl;

        // Print each element in the BBSet
        cout << "{ ";
        for (const SVFBasicBlock* dominatedBB : bbSet)
        {
            if (dominatedBB)
                cout << dominatedBB->getName()
                     << " "; // Print the name of each dominated basic block
        }
        cout << "}" << endl;
    }
}

/**
 * @brief Computes the dominators for the function using a specific algorithm.
 *
 * described in this paper:
 * A Simple, Fast Dominance Algorithm
 * Keith D. Cooper, Timothy J. Harvey and Ken Kennedy
 * Software-Practice and Expreience, 2001;4:1-10.
 *
 * This implementation is simple and runs faster in practice than the classic
 * Lengauer-Tarjan algorithm. For detailed discussions, refer to the paper.
 *
 * It is adopted by llvm-clang
 * (https://github.com/avakar/clang/blob/master/lib/Analysis/Dominators.cpp)
 */
void DominatorAnalysis::computeDominators()
{
    // Step 1: Get reverse post order
    vector<const SVFBasicBlock*>
        revPostOrder;                    // Vector to store reverse post order
    Set<const SVFBasicBlock*> reachable; // Set to track reachable basic blocks

    const SVFBasicBlock* entryBB = func->getEntryBlock();
    // Lambda for DFSUtil
    auto DFSUtil = [&](const SVFBasicBlock* v, auto&& DFSUtilRef) -> void {
        reachable.insert(v); // Mark the current block as reachable

        for (const auto& succ : v->getSuccessors())
        {
            if (!reachable.count(succ))
            {
                DFSUtilRef(succ, DFSUtilRef); // Recursively visit successors
            }
        }
        // Push current vertex to stack which stores the postorder
        revPostOrder.push_back(v);
    };

    // Lambda for getReversePostOrder
    auto getReversePostOrder = [&]() -> void {
        // Call the recursive helper function to store postorder
        // traversal starting from the entry block
        DFSUtil(entryBB, DFSUtil);

        reverse(revPostOrder.begin(),
                revPostOrder.end()); // Reverse to get reverse post order
    };

    getReversePostOrder(); // Get the reverse post order

    Map<const SVFBasicBlock*, u32_t>
        bbToIdx; // Map basic blocks to their indices in reverse post order
    for (u32_t i = 0; i < revPostOrder.size(); ++i)
    {
        bbToIdx[revPostOrder[i]] = i; // Assign index to each basic block
    }

    int root = bbToIdx[entryBB]; // Index of the entry block
    int n = revPostOrder.size(); // Number of basic blocks
    vector<int> IDoms(n, -1);    // Vector to store immediate dominators
    // Initialize the dominator sets
    IDoms[root] = root; // The entry block dominates itself
    bool change = true;
    while (change)
    {
        change = false;
        for (int i = 0; i < n; i++)
        {
            if (i == root)
                continue;
            int new_idom = -1;
            for (const auto& predBB : revPostOrder[i]->getPredecessors())
            {
                if (!reachable.count(predBB))
                    continue;
                int pred = bbToIdx[predBB];
                if (IDoms[pred] == -1)
                    continue;
                if (new_idom == -1)
                    new_idom = pred;
                else
                {
                    // insert function
                    int B1 = new_idom, B2 = pred;
                    while (B1 != B2)
                    {
                        while (B1 > B2)
                            B1 = IDoms[B1];
                        while (B2 > B1)
                            B2 = IDoms[B2];
                    }
                    new_idom = B1;
                }
            }
            if (IDoms[i] != new_idom)
            {
                IDoms[i] = new_idom;
                change = true;
            }
        }
    }

    // Map to store the dominator tree
    Map<const SVFBasicBlock*, BBSet>& dtBBsMap =
        func->svfLoopAndDom->getDomTreeMap();
    for (int i = 0; i < n; i++)
    {
        if (IDoms[i] != -1 && i != IDoms[i])
        {
            dtBBsMap[revPostOrder[IDoms[i]]].insert(
                revPostOrder[i]); // Populate the dominator tree map
        }
    }

#if DEBUG
    // Perform differential testing between SVF and LLVM dominator maps
    DominatorMap& svfMap = func->svfLoopAndDom->getDomTreeMap();
    DominatorMap& llvmMap = func->loopAndDom->getDomTreeMap();
    differentialTesting(llvmMap, svfMap);
#endif
}
/**
 * @brief Performs differential testing between LLVM and SVF dominator maps.
 *
 * This method compares the dominator maps generated by LLVM and SVF to ensure
 * they are consistent. It prints both maps for visual inspection and checks for
 * discrepancies, aborting on any mismatch.
 *
 * @param llvmMap The dominator map computed by LLVM.
 * @param svfMap The dominator map computed by SVF.
 */
void DominatorAnalysis::differentialTesting(const DominatorMap& llvmMap,
                                            const DominatorMap& svfMap)
{
    // Separator for visual clarity in output
    string sep = "===================================";

    // Print the function name
    cout << "\nFunction: " << func->getName() << "\n";

    // Print the SVF dominator map
    cout << "SVFMap:" << sep << "\n";
    dumpDominatorMap(svfMap);

    // Print the LLVM dominator map
    cout << "LLVMMap:" << sep << "\n";
    dumpDominatorMap(llvmMap);

    // Print the separator again
    cout << sep << "\n";

    // Check if the sizes of the two maps are the same
    ABORT_IFNOT(llvmMap.size() == svfMap.size(), "different size");

    // Iterate over the LLVM dominator map
    for (const auto& [dom, children] : llvmMap)
    {
        // Find the corresponding entry in the SVF dominator map
        auto it = svfMap.find(dom);

        // Check if the dominator exists in the SVF map
        ABORT_IFNOT(it != svfMap.end(),
                    dom->getName() + " not found in svfMap");

        // Check if the children sets are the same
        ABORT_IFNOT(it->second == children,
                    dom->getName() + "'s children are different");
    }
}

/**
 * @brief Computes the dominators for the function using the classic iterative
 * algorithm.
 *
 * This method follows the classic algorithm to compute the dominator sets for
 * each basic block in the function.
 */
void DominatorAnalysis::computeDominators_classic()
{
    // Step 1: Get reverse post order
    vector<const SVFBasicBlock*>
        revPostOrder;                    // Vector to store reverse post order
    Set<const SVFBasicBlock*> reachable; // Set to track reachable basic blocks

    // Lambda for DFSUtil
    auto DFSUtil = [&](const SVFBasicBlock* v, auto&& DFSUtilRef) -> void {
        reachable.insert(v); // Mark the current block as reachable

        for (const auto& succ : v->getSuccessors())
        {
            if (!reachable.count(succ))
            {
                DFSUtilRef(succ, DFSUtilRef); // Recursively visit successors
            }
        }
        // Push current vertex to stack which stores the postorder
        revPostOrder.push_back(v);
    };

    // Lambda for getReversePostOrder
    auto getReversePostOrder = [&]() -> void {
        // Call the recursive helper function to store postorder
        // traversal starting from the entry block
        DFSUtil(func->getEntryBlock(), DFSUtil);

        reverse(revPostOrder.begin(),
                revPostOrder.end()); // Reverse to get reverse post order
    };

    getReversePostOrder(); // Get the reverse post order

    Map<const SVFBasicBlock*, u32_t>
        bbToIdx; // Map basic blocks to their indices in reverse post order
    for (u32_t i = 0; i < revPostOrder.size(); ++i)
    {
        bbToIdx[revPostOrder[i]] = i; // Assign index to each basic block
    }

    int root = bbToIdx[func->getEntryBlock()]; // Index of the entry block
    int n = revPostOrder.size();               // Number of basic blocks
    vector<set<int>> dominators(
        n, set<int>()); // Vector of sets to store dominators for each block
    vector<int> IDoms(n, -1); // Vector to store immediate dominators

    for (int i = 0; i < n; ++i)
    {
        IDoms[i] = -1; // Initialize immediate dominators to -1
    }

    // Initialize the dominator sets
    for (int i = 0; i < n; i++)
    {
        if (i == root)
        {
            dominators[i].insert(i); // The entry block dominates itself
        }
        else
        {
            for (int j = 0; j < n; j++)
            {
                dominators[i].insert(j); // Initially, every block is considered
                                         // to dominate every other block
            }
        }
    }

    bool change = true;
    while (change)
    {
        change = false;
        for (int i = 0; i < n; i++)
        {
            if (i == root)
                continue;

            set<int> newDom;
            bool firstPred = true;

            // Compute intersection of dominator sets of predecessors
            for (const auto& predBB : revPostOrder[i]->getPredecessors())
            {
                if (!reachable.count(predBB))
                    continue;
                int pred = bbToIdx[predBB];
                if (firstPred)
                {
                    newDom = dominators[pred];
                    firstPred = false;
                }
                else
                {
                    set<int> temp;
                    set_intersection(
                        newDom.begin(), newDom.end(), dominators[pred].begin(),
                        dominators[pred].end(), inserter(temp, temp.begin()));
                    newDom = temp;
                }
            }

            newDom.insert(i); // Node dominates itself

            if (newDom != dominators[i])
            {
                dominators[i] = newDom;
                change = true;
            }
        }
    }

    // Compute immediate dominators
    for (int u = 0; u < n; ++u)
    {
        if (u == root)
            continue;

        // Immediate dominator is the closest dominator that dominates 'u'
        for (int dom : dominators[u])
        {
            if (dom != u && (IDoms[u] == -1 || dominators[dom].size() >
                                                   dominators[IDoms[u]].size()))
            {
                IDoms[u] = dom;
            }
        }
    }

    // Map to store the dominator tree
    Map<const SVFBasicBlock*, BBSet>& dtBBsMap =
        func->svfLoopAndDom->getDomTreeMap();
    for (int i = 0; i < n; i++)
    {
        if (IDoms[i] != -1)
        {
            dtBBsMap[revPostOrder[IDoms[i]]].insert(
                revPostOrder[i]); // Populate the dominator tree map
        }
    }
}

void DominatorAnalysis::computePostDominators()
{
    // Step 1: Get reverse post order in the reversed CFG
    vector<const SVFBasicBlock*>
        revPostOrder;                    // Vector to store reverse post order
    Set<const SVFBasicBlock*> reachable; // Set to track reachable basic blocks

    Set<const SVFBasicBlock*> exitBBs;
    for (const auto* bb : func->getBasicBlockList())
    {
        if (bb->getSuccessors().empty())
            exitBBs.insert(bb);
    }

    // Lambda for DFSUtil in the reversed CFG
    auto DFSUtil = [&](const SVFBasicBlock* v, auto&& DFSUtilRef) -> void {
        reachable.insert(v); // Mark the current block as reachable

        for (const auto& pred :
             v->getPredecessors()) // Use predecessors instead of successors
        {
            if (!reachable.count(pred))
            {
                DFSUtilRef(pred, DFSUtilRef); // Recursively visit predecessors
            }
        }
        // Push current vertex to stack which stores the postorder
        revPostOrder.push_back(v);
    };

    // Lambda for getReversePostOrder in the reversed CFG
    auto getReversePostOrder = [&]() -> void {
        // Call the recursive helper function to store postorder
        // traversal starting from the exit block
        for (const auto& bb : exitBBs)
            DFSUtil(bb, DFSUtil);

        reverse(revPostOrder.begin(),
                revPostOrder.end()); // Reverse to get reverse post order
    };

    getReversePostOrder(); // Get the reverse post order

    // Put a virtual exit block at the front of the order
    revPostOrder.insert(revPostOrder.begin(), nullptr);
    reachable.insert(nullptr);

    Map<const SVFBasicBlock*, u32_t>
        bbToIdx; // Map basic blocks to their indices in reverse post order
    for (u32_t i = 0; i < revPostOrder.size(); ++i)
    {
        bbToIdx[revPostOrder[i]] = i; // Assign index to each basic block
    }

    int root = 0;                // Index of the virtual exit block
    int n = revPostOrder.size(); // Number of basic blocks
    vector<int> IPdoms(n, -1);   // Vector to store immediate postdominators
    // Initialize the postdominator sets
    IPdoms[root] = root; // The exit block postdominates itself
    bool change = true;
    while (change)
    {
        change = false;
        for (int i = 0; i < n; i++)
        {
            if (i == root)
                continue;
            int new_ipdom = -1;
            vector<const SVFBasicBlock*> successors =
                revPostOrder[i]->getSuccessors();
            if (successors.empty()) // The successor of exit BBs is the virtual
                                    // exit bb
                successors.push_back(nullptr);
            for (const auto& succBB : successors) // Use successors instead of
                                                  // predecessors
            {
                if (!reachable.count(succBB))
                    continue;
                int succ = bbToIdx[succBB];
                if (IPdoms[succ] == -1)
                    continue;
                if (new_ipdom == -1)
                    new_ipdom = succ;
                else
                {
                    // insert function
                    int B1 = new_ipdom, B2 = succ;
                    while (B1 != B2)
                    {
                        while (B1 > B2)
                            B1 = IPdoms[B1];
                        while (B2 > B1)
                            B2 = IPdoms[B2];
                    }
                    new_ipdom = B1;
                }
            }
            if (IPdoms[i] != new_ipdom)
            {
                IPdoms[i] = new_ipdom;
                change = true;
            }
        }
    }

    // Map to store the postdominator tree
    Map<const SVFBasicBlock*, BBSet>& pdtBBsMap =
        func->svfLoopAndDom->getPostDomTreeMap();
    for (int i = 1; i < n; i++)
    {
        if (IPdoms[i] != 0 && IPdoms[i] != -1 && i != IPdoms[i])
        {
            pdtBBsMap[revPostOrder[IPdoms[i]]].insert(
                revPostOrder[i]); // Populate the postdominator tree map
        }
    }

#if DEBUG
    // Perform differential testing between SVF and LLVM postdominator maps
    DominatorMap& svfMap = func->svfLoopAndDom->getPostDomTreeMap();
    DominatorMap& llvmMap = func->loopAndDom->getPostDomTreeMap();
    differentialTesting(llvmMap, svfMap);
#endif
}

void DominatorAnalysis::computeDominatorFrontier() {}