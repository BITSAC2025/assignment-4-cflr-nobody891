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
    unsigned numNodes = graph->getNumNodes(); // 假设此方法可用并返回最大节点ID
    
    for (unsigned v = 1; v <= numNodes; ++v) {
        // 检查并添加 VF ::= epsilon
        if (!graph->hasEdge(v, v, VF)) {
            graph->addEdge(v, v, VF);
            workList.push(CFLREdge(v, v, VF));
        }
        // 检查并添加 VFBar ::= epsilon
        if (!graph->hasEdge(v, v, VFBar)) {
            graph->addEdge(v, v, VFBar);
            workList.push(CFLREdge(v, v, VFBar));
        }
        // 检查并添加 VA ::= epsilon
        if (!graph->hasEdge(v, v, VA)) {
            graph->addEdge(v, v, VA);
            workList.push(CFLREdge(v, v, VA));
        }
    }

    // --- 步骤 2: 动态规划 CFL-可达性算法 ---
    CFLRGraph::DataMap &succMap = graph->getSuccessorMap();
    CFLRGraph::DataMap &predMap = graph->getPredecessorMap();

    while (!workList.empty())
    {
        CFLREdge edge = workList.pop();
        unsigned vi = edge.src;
        unsigned vj = edge.dst;
        EdgeLabel X = edge.label;
        
        // ----------------------------------------------------------------------------------
        // Part A: Binary Production A ::= X Y (Forward closure)
        // (vi --X--> vj) + (vj --Y--> vk) => (vi --A--> vk)
        // ----------------------------------------------------------------------------------
        
        if (succMap.count(vj)) {
            for (auto const& [Y, vk_set] : succMap.at(vj)) {
                for (unsigned vk : vk_set) {
                    
                    EdgeLabel A = 0;
                    
                    // --- 检查所有 A ::= X Y 产生式 (图1文法) ---
                    // PT ::= VFBar Addr
                    if (X == VFBar && Y == Addr) A = PT;
                    
                    // VF ::= VFBar VF | SVBar Load | PVBar Load | StoreBar VP
                    else if (X == VFBar && Y == VF) A = VF;
                    else if (X == SVBar && Y == Load) A = VF;
                    else if (X == PVBar && Y == Load) A = VF;
                    else if (X == StoreBar && Y == VP) A = VF;
                    
                    // VFBar ::= VFBar VF | LoadBar SVBar | LoadBar VPBar | PVBar StoreBar
                    else if (X == VFBar && Y == VF) A = VFBar;
                    else if (X == LoadBar && Y == SVBar) A = VFBar;
                    else if (X == LoadBar && Y == VPBar) A = VFBar;
                    else if (X == PVBar && Y == StoreBar) A = VFBar;
                    
                    // VA ::= LVBar Load | VFBar VA
                    else if (X == LVBar && Y == Load) A = VA;
                    else if (X == VFBar && Y == VA) A = VA;
                    
                    // SV ::= StoreBar VA
                    else if (X == StoreBar && Y == VA) A = SV;
                    
                    // PV ::= PTBar VA
                    else if (X == PTBar && Y == VA) A = PV;
                    
                    // VPBar ::= VABar PTBar
                    else if (X == VABar && Y == PTBar) A = VPBar; 
                    
                    // LV ::= LoadBar VA (Note: LVBar is defined, but LV is LoadBar VA which is A ::= X Y)
                    else if (X == LoadBar && Y == VA) A = LV;

                    // --- 检查一元产生式 A ::= X (例如 A ::= Copy) ---
                    // 在正则化文法中，Copy和CopyBar通常被视为VF和VFBar的别名，
                    // 它们在 buildGraph 中作为 VF/VFBar 边初始化，或在这里作为 A ::= X 规则处理。
                    // 如果它们在 buildGraph 中只被初始化为 Copy/CopyBar，则需要此步骤：
                    // VF ::= Copy
                    if (X == Copy) A = VF;
                    // VFBar ::= CopyBar
                    else if (X == CopyBar) A = VFBar;
                    
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
        // Part B: Binary Production A ::= Y X (Backward closure)
        // (vk --Y--> vi) + (vi --X--> vj) => (vk --A--> vj)
        // ----------------------------------------------------------------------------------
        
        if (predMap.count(vi)) {
            for (auto const& [Y, vk_set] : predMap.at(vi)) {
                for (unsigned vk : vk_set) {
                    
                    EdgeLabel A = 0;
                    
                    // --- 检查所有 A ::= Y X 产生式 (图1文法) ---
                    // PTBar ::= Addr VF
                    if (Y == Addr && X == VF) A = PTBar;
                    
                    // VA ::= VABar VF
                    else if (Y == VABar && X == VF) A = VA;
                    
                    // SVBar ::= VA StoreBar
                    else if (Y == VA && X == StoreBar) A = SVBar;
                    
                    // PVBar ::= VA PTBar
                    else if (Y == VA && X == PTBar) A = PVBar;
                    
                    // VP ::= VA PT
                    else if (Y == VA && X == PT) A = VP;
                    
                    // (其他规则在 A ::= X Y 中已覆盖，或者通过 A ::= Y X 形式查找)
                    
                    if (A != 0) {
                        if (!graph->hasEdge(vk, vj, A)) {
                            graph->addEdge(vk, vj, A);
                            workList.push(CFLREdge(vk, vj, A));
                        }
                    }
                }
            }
        }
        
        // ----------------------------------------------------------------------------------
        // Part C: Binary Production A ::= Y X (for A ::= X Y rules handled as Y X)
        // For rules like VFBar ::= LoadBar SVBar (X=LoadBar, Y=SVBar), we need to check both directions in the worklist loop.
        // When we pop (vi --X--> vj), we check predecessors (vk --Y--> vi) to form (vk --A--> vj).
        // Let's re-examine if any X Y rules must be checked as Y X on the new edge:
        // Ex: (vk --Y--> vi) + (vi --X--> vj) => (vk --A--> vj)
        // If the new edge is (vi --X--> vj) and we look for predecessors (vk --Y--> vi)
        
        // This is necessary if A ::= X Y produces a new X or Y edge that then triggers another rule.
        // For simplicity and correctness, the two main loops (A ::= X Y and A ::= Y X) should be sufficient, 
        // as the worklist mechanism handles subsequent applications.
        
    }
}
