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

    std::unordered_map<Node, std::unordered_set<Node>> pt;

    CFLRGraph::DataMap &succ = graph->getSuccessorMap();
    CFLRGraph::DataMap &pred = graph->getPredecessorMap();

    auto addPt = [&](Node u, Node v) -> bool {
        auto &s = pt[u];
        if (s.find(v) == s.end()) {
            s.insert(v);
            workList.push(CFLREdge(u, v, PT));
            return true;
        }
        return false;
    };

    for (const auto &src_pair : succ) {
        Node src = src_pair.first;
        const auto &inner = src_pair.second;
        for (const auto &lbl_pair : inner) {
            Label lbl = lbl_pair.first;
            const auto &dsts = lbl_pair.second;
            if (lbl == Addr || lbl == PT) {
                for (Node dst : dsts) addPt(src, dst);
            }
        }
    }


    while (!workList.empty()) {
        CFLREdge e = workList.pop();
        Node u = e.src;
        Node v = e.dst;
        auto itPredCopy = pred.find(u);
        if (itPredCopy != pred.end()) {
            const auto &predMapForU = itPredCopy->second;
            auto itCopySet = predMapForU.find(Copy);
            if (itCopySet != predMapForU.end()) {
                for (Node t : itCopySet->second) {
                    addPt(t, v);
                }
            }
        }

        auto itPredStore = pred.find(u);
        if (itPredStore != pred.end()) {
            const auto &predMapForU = itPredStore->second;
            auto itStoreSet = predMapForU.find(Store);
            if (itStoreSet != predMapForU.end()) {
                auto itSuccV = succ.find(v);
                if (itSuccV != succ.end()) {
                    const auto &succMapForV = itSuccV->second;
                    auto itLoadSet = succMapForV.find(Load);
                    if (itLoadSet != succMapForV.end()) {
                        for (Node s : itStoreSet->second) {
                            for (Node x : itLoadSet->second) {
                                addPt(s, x);
                            }
                        }
                    }
                }
            }
        }
    }

    for (const auto &p : pt) {
        Node src = p.first;
        for (Node dst : p.second) {
            if (!graph->hasEdge(src, dst, PT)) {
                graph->addEdge(src, dst, PT);
            }
        }
    }
}
