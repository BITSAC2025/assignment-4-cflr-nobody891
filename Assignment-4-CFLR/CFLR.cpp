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

    // --- 0. collect all nodes in the graph (keys and targets) ---
    std::unordered_set<Node> allNodes;
    for (const auto &p : succ) {
        allNodes.insert(p.first);
        for (const auto &inner : p.second) {
            for (Node dst : inner.second) allNodes.insert(dst);
        }
    }
    // Also consider predecessor map keys (if any nodes only in pred)
    for (const auto &p : pred) {
        allNodes.insert(p.first);
        for (const auto &inner : p.second) {
            for (Node dst : inner.second) allNodes.insert(dst);
        }
    }

    // --- Grammar: epsilon / single / binary ---
    // epsilon productions: VF, VA
    std::vector<Label> epsilon;
    epsilon.push_back(VF);
    epsilon.push_back(VA);

    // single productions (A ::= X) - none in given grammar
    std::vector<Pair> single;

    // binary productions (A ::= L R)
    std::vector<Triple> binary;

    // Fill binary according to the expansion we prepared from the normalized grammar.
    // NOTE: some long RHS were decomposed into plausible binary fragments to
    // encode the structure; this follows the conversion we discussed.

    // PT ::= VF Addr
    binary.emplace_back(PT, VF, Addr);
    // PT ::= Addr VF
    binary.emplace_back(PT, Addr, VF);

    // For VF long rules we include fragments that appear in sequences.
    // These are the decomposition pieces to be used by the worklist algorithm.
    // (This mirrors the previous conversion provided.)
    // VF ::= VF VF
    binary.emplace_back(VF, VF, VF);
    // VF ::= VF Copy
    binary.emplace_back(VF, VF, Copy);
    // VF ::= Copy SV
    binary.emplace_back(VF, Copy, SV);
    // VF ::= SV Load
    binary.emplace_back(VF, SV, Load);
    // VF ::= Load PV
    binary.emplace_back(VF, Load, PV);
    // VF ::= PV Load
    binary.emplace_back(VF, PV, Load);
    // VF ::= Load Store
    binary.emplace_back(VF, Load, Store);
    // VF ::= Store VP
    binary.emplace_back(VF, Store, VP);

    // second variant expansions from the other long rule:
    binary.emplace_back(VF, VF, VF); // may be duplicate; safe
    binary.emplace_back(VF, VF, Copy);
    binary.emplace_back(VF, Copy, Load);
    binary.emplace_back(VF, Load, SV);
    binary.emplace_back(VF, SV, Load);
    binary.emplace_back(VF, Load, VP);
    binary.emplace_back(VF, VP, PV);
    binary.emplace_back(VF, PV, Store);

    // VA rules (decomposed)
    // VA ::= LV Load
    binary.emplace_back(VA, LV, Load);
    // VA ::= Load VF
    binary.emplace_back(VA, Load, VF);
    // VA ::= VF VA
    binary.emplace_back(VA, VF, VA);
    // VA ::= VA VA
    binary.emplace_back(VA, VA, VA);
    // VA ::= VA VF
    binary.emplace_back(VA, VA, VF);

    // SV rules
    // SV ::= Store VA
    binary.emplace_back(SV, Store, VA);
    // SV ::= VA Store
    binary.emplace_back(SV, VA, Store);

    // PV ::= PT VA
    binary.emplace_back(PV, PT, VA);

    // VP ::= VA PT
    binary.emplace_back(VP, VA, PT);

    // LV ::= Load VA
    binary.emplace_back(LV, Load, VA);

    // --- 1. initialize worklist W <- all existing edges in graph (succ map) ---
    // For every u, for every label L, for every v in succ[u][L], push u -L-> v
    for (const auto &u_pair : succ) {
        Node u = u_pair.first;
        for (const auto &lbl_pair : u_pair.second) {
            Label lbl = lbl_pair.first;
            for (Node v : lbl_pair.second) {
                W.push(CFLREdge(u, v, lbl));
            }
        }
    }

    // --- 2. epsilon productions: add v --A--> v for all nodes v and each A in epsilon ---
    for (Label A : epsilon) {
        for (Node v : allNodes) {
            if (!graph->hasEdge(v, v, A)) {
                graph->addEdge(v, v, A);
                W.push(CFLREdge(v, v, A));
            }
        }
    }

    // --- 3. main loop: while W != empty do ---
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

        // Case 2: binary prods where left == X: A ::= X Y
        // if vi --X--> vj and vj --Y--> vk then add vi --A--> vk
        for (const auto &t : binary) {
            Label A, L, R;
            std::tie(A, L, R) = t;
            if (L != X) continue;
            // get succ[vj][R] if exists
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

        // Case 3: binary prods where right == X: A ::= Y X
        // if vk --Y--> vi and vi --X--> vj then add vk --A--> vj
        for (const auto &t : binary) {
            Label A, L, R;
            std::tie(A, L, R) = t;
            if (R != X) continue;
            // get pred[vi][L] if exists
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
    } // end while W

    // solve() done. Results stored in graph (new edges added). dumpResult() can use graph.
}
