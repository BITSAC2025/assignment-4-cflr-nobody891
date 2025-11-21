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
        if (!graph) return;

    using Node = unsigned;
    using Label = EdgeLabel;
    using Triple = std::tuple<Label, Label, Label>;
    using Pair = std::pair<Label, Label>;

    auto &succ = graph->getSuccessorMap();
    auto &pred = graph->getPredecessorMap();
    auto &W = workList;

    // --- collect all nodes (from succ and pred maps) ---
    std::unordered_set<Node> allNodes;
    for (const auto &p : succ) {
        allNodes.insert(p.first);
        for (const auto &inner : p.second) {
            for (Node dst : inner.second) allNodes.insert(dst);
        }
    }
    for (const auto &p : pred) {
        allNodes.insert(p.first);
        for (const auto &inner : p.second) {
            for (Node dst : inner.second) allNodes.insert(dst);
        }
    }

    // --- Grammar encoding ---
    // epsilon productions: VF, VA  (they can derive epsilon)
    std::vector<Label> epsilon;
    epsilon.push_back(VF);
    epsilon.push_back(VA);

    // single productions: none in given grammar
    std::vector<Pair> single;

    // binary productions A ::= L R  (encode decomposed fragments)
    std::vector<Triple> binary;

    // PT ::= VF Addr
    binary.emplace_back(PT, VF, Addr);
    // PT ::= Addr VF
    binary.emplace_back(PT, Addr, VF);

    // VF fragments (decomposed chain fragments)
    binary.emplace_back(VF, VF, VF);
    binary.emplace_back(VF, VF, Copy);
    binary.emplace_back(VF, Copy, SV);
    binary.emplace_back(VF, SV, Load);
    binary.emplace_back(VF, Load, PV);
    binary.emplace_back(VF, PV, Load);
    binary.emplace_back(VF, Load, Store);
    binary.emplace_back(VF, Store, VP);

    // another variant fragments
    binary.emplace_back(VF, VF, VF); // duplicate safe
    binary.emplace_back(VF, VF, Copy);
    binary.emplace_back(VF, Copy, Load);
    binary.emplace_back(VF, Load, SV);
    binary.emplace_back(VF, SV, Load);
    binary.emplace_back(VF, Load, VP);
    binary.emplace_back(VF, VP, PV);
    binary.emplace_back(VF, PV, Store);

    // VA decomposed
    binary.emplace_back(VA, LV, Load);
    binary.emplace_back(VA, Load, VF);
    binary.emplace_back(VA, VF, VA);
    binary.emplace_back(VA, VA, VA);
    binary.emplace_back(VA, VA, VF);

    // SV
    binary.emplace_back(SV, Store, VA);
    binary.emplace_back(SV, VA, Store);

    // PV
    binary.emplace_back(PV, PT, VA);

    // VP
    binary.emplace_back(VP, VA, PT);

    // LV
    binary.emplace_back(LV, Load, VA);

    // --- Step 1: initialize W with all existing edges in the graph ---
    for (const auto &u_pair : succ) {
        Node u = u_pair.first;
        for (const auto &lbl_pair : u_pair.second) {
            Label lbl = lbl_pair.first;
            for (Node v : lbl_pair.second) {
                W.push(CFLREdge(u, v, lbl));
            }
        }
    }

    // --- Step 2: epsilon productions -> add v --A--> v for all nodes v and A in epsilon ---
    for (Label A : epsilon) {
        for (Node v : allNodes) {
            if (!graph->hasEdge(v, v, A)) {
                graph->addEdge(v, v, A);
                W.push(CFLREdge(v, v, A));
            }
        }
    }

    // --- Step 3: main loop ---
    while (!W.empty()) {
        CFLREdge cur = W.pop();
        Node vi = cur.src;
        Node vj = cur.dst;
        Label X = cur.label;

        // Case 1: single productions A ::= X
        for (const auto &pr : single) {
            Label A = pr.first;
            Label B = pr.second;
            if (B == X) {
                if (!graph->hasEdge(vi, vj, A)) {
                    graph->addEdge(vi, vj, A);
                    W.push(CFLREdge(vi, vj, A));
                }
            }
        }

        // Case 2: A ::= X Y  (left matches X)
        for (const auto &t : binary) {
            Label A, L, R;
            std::tie(A, L, R) = t;
            if (L != X) continue;
            auto it_vj = succ.find(vj);
            if (it_vj == succ.end()) continue;
            auto itR = it_vj->second.find(R);
            if (itR == it_vj->second.end()) continue;
            for (Node vk : itR->second) {
                if (!graph->hasEdge(vi, vk, A)) {
                    graph->addEdge(vi, vk, A);
                    W.push(CFLREdge(vi, vk, A));
                }
            }
        }

        // Case 3: A ::= Y X  (right matches X)
        for (const auto &t : binary) {
            Label A, L, R;
            std::tie(A, L, R) = t;
            if (R != X) continue;
            auto it_vi = pred.find(vi);
            if (it_vi == pred.end()) continue;
            auto itL = it_vi->second.find(L);
            if (itL == it_vi->second.end()) continue;
            for (Node vk : itL->second) {
                if (!graph->hasEdge(vk, vj, A)) {
                    graph->addEdge(vk, vj, A);
                    W.push(CFLREdge(vk, vj, A));
                }
            }
        }
    } // end while
    // done - all discovered nonterminal-labeled edges have been added to graph
}
