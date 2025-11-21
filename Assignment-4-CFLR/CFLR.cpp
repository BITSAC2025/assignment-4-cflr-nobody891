/**
 * CFLR.cpp
 * @author kisslune 
 */

#include "A4Header.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    pag->dump();

    CFLR solver;
    solver.buildGraph(pag);
    // TODO: complete this method
    solver.solve();
    solver.dumpResult();

    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFLR::solve()
{
    // TODO: complete this function. The implementations of graph and worklist are provided.
    //  You need to:
    //  1. implement the grammar production rules into code;
    //  2. implement the dynamic-programming CFL-reachability algorithm.
    //  You may need to add your new methods to 'CFLRGraph' and 'CFLR'.
    CFLRGraph::DataMap &succMap = graph->getSuccessorMap();
    CFLRGraph::DataMap &predMap = graph->getPredecessorMap();

    // Step 3 (Algorithm 1, line 4): Process worklist W until empty.
    while (!workList.empty())
    {
        // Step 3.1 (Algorithm 1, line 5): Select and remove an edge (vi --X--> vj) from W.
        CFLREdge edge = workList.pop();
        unsigned vi = edge.src;
        unsigned vj = edge.dst;
        EdgeLabel X = edge.label;

        // The CFL-reachability algorithm has three main types of production rules to handle:
        // 1. A ::= X (Unary production, line 6-7) - Epsilon productions are handled implicitly or in initialization.
        // 2. A ::= X Y (Binary production, line 8-10) - Forward closure.
        // 3. A ::= Y X (Binary production, line 11-13) - Backward closure.

        // --- Unary Productions (A ::= X) ---
        // The grammar has no non-terminal A that solely produces a single terminal X (like A ::= Addr).
        // However, there are non-terminal to non-terminal unary rules that represent the identity:
        // VF ::= Copy, VF ::= Store VP, etc. (These are actually handled as binary rules where one side is epsilon or implicitly handled by the subsequent binary rules).
        // We only explicitly handle epsilon rules here if needed, but the core logic focuses on binary rules.
        
        // Handling epsilon rules (A ::= epsilon) and implicit rules like VF ::= Copy:
        // Epsilon rules (VF, VFBar, VA) are usually handled by initializing the graph with self-loops
        // for nodes where A ::= epsilon is applicable (e.g., vi --VF--> vi) or through the subsequent
        // binary rules that use them. In this context, the provided pseudocode for CFLR focuses only on
        // edge discovery through composition. We'll rely on the binary rules.

        // --- Binary Productions (A ::= X Y or A ::= Y X) ---
        
        // ----------------------------------------------------------------------------------
        // Case 1: (vi --X--> vj) combined with a forward edge (vj --Y--> vk) to produce (vi --A--> vk)
        // A ::= X Y (Algorithm 1, lines 8-10)
        // ----------------------------------------------------------------------------------
        
        // Iterate over all successors vk of vj
        if (succMap.count(vj)) {
            for (auto const& [Y, vk_set] : succMap.at(vj)) {
                for (unsigned vk : vk_set) {
                    
                    // --- Check all A ::= X Y productions from the grammar ---
                    // Example: PT ::= VFBar Addr (if X=VFBar, Y=Addr, then A=PT)
                    
                    EdgeLabel A = 0; // Default to an invalid label
                    
                    if (X == VFBar && Y == Addr) A = PT;
                    else if (X == VA && Y == PT) A = VP;
                    else if (X == LVBar && Y == Load) A = VA;
                    else if (X == VFBar && Y == VA) A = VA;
                    else if (X == SVBar && Y == Load) A = VF;
                    else if (X == PVBar && Y == Load) A = VF;
                    else if (X == StoreBar && Y == VP) A = VF;
                    else if (X == VFBar && Y == VF) A = VF;
                    
                    // Barred versions: ABar ::= XBar YBar
                    else if (X == Addr && Y == VF) A = PTBar;
                    else if (X == VFBar && Y == VF) A = VFBar;
                    else if (X == LoadBar && Y == SVBar) A = VFBar;
                    else if (X == LoadBar && Y == VPBar) A = VFBar;
                    else if (X == PVBar && Y == StoreBar) A = VFBar;
                    else if (X == StoreBar && Y == VA) A = SV; // SV ::= StoreBar VA
                    else if (X == PTBar && Y == VA) A = PV;
                    
                    // If a valid production A was found, try to add the new edge (vi --A--> vk)
                    if (A != 0) {
                        if (!graph->hasEdge(vi, vk, A)) {
                            graph->addEdge(vi, vk, A);
                            workList.push(CFLREdge(vi, vk, A));
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------------------------------------
        // Case 2: (vk --Y--> vi) combined with (vi --X--> vj) to produce (vk --A--> vj)
        // A ::= Y X (Algorithm 1, lines 11-13)
        // ----------------------------------------------------------------------------------
        
        // Iterate over all predecessors vk of vi
        if (predMap.count(vi)) {
            for (auto const& [Y, vk_set] : predMap.at(vi)) {
                for (unsigned vk : vk_set) {
                    
                    // --- Check all A ::= Y X productions from the grammar ---
                    // Example: SVBar ::= VA StoreBar (if Y=VA, X=StoreBar, then A=SVBar)

                    EdgeLabel A = 0; // Default to an invalid label
                    
                    if (Y == VA && X == PT) A = VP; // VP ::= VA PT
                    else if (Y == VA && X == PTBar) A = PVBar;
                    else if (Y == VA && X == StoreBar) A = SVBar; // SVBar ::= VA StoreBar
                    else if (Y == VA && X == VF) A = VA;
                    else if (Y == PTBar && X == VA) A = PV;
                    else if (Y == LoadBar && X == VA) A = LV;
                    else if (Y == VA && X == PT) A = VP; // Already covered
                    
                    // Barred versions: ABar ::= YBar XBar
                    else if (Y == LoadBar && X == SVBar) A = VFBar;
                    else if (Y == LoadBar && X == VPBar) A = VFBar;
                    else if (Y == PVBar && X == StoreBar) A = VFBar;
                    
                    // Barred versions (from A ::= X Y):
                    else if (Y == VFBar && X == VF) A = VF; // VF ::= VFBar VF
                    else if (Y == VFBar && X == VF) A = VFBar; // VFBar ::= VFBar VF (This is a left-recursive rule, handling in A ::= X Y covers the left-most application)
                    
                    // If a valid production A was found, try to add the new edge (vk --A--> vj)
                    if (A != 0) {
                        if (!graph->hasEdge(vk, vj, A)) {
                            graph->addEdge(vk, vj, A);
                            workList.push(CFLREdge(vk, vj, A));
                        }
                    }
                }
            }
        }
    }
}
