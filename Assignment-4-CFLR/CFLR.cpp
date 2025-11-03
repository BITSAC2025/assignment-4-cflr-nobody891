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
    //pag->dump();

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
    auto &succMap = graph->getSuccessorMap();
    auto &predMap = graph->getPredecessorMap();

    // Step 1: 初始化工作表
    for (auto &srcPair : succMap)
    {
        unsigned src = srcPair.first;
        for (auto &labelPair : srcPair.second)
        {
            EdgeLabel lbl = labelPair.first;
            for (unsigned dst : labelPair.second)
                workList.push(CFLREdge(src, dst, lbl));
        }
    }

    // Step 2: 主循环
    while (!workList.empty())
    {
        CFLREdge e = workList.pop();
        unsigned vi = e.src;
        unsigned vj = e.dst;
        EdgeLabel A = e.label;

        /// 
        for (auto &srcPair : succMap)
        {
            (void)srcPair; // 避免警告
        }

        /// PT ::= VF Addr
        if (A == VF)
        {
            for (unsigned vk : succMap[vj][Addr])
                if (!graph->hasEdge(vi, vk, PT))
                {
                    graph->addEdge(vi, vk, PT);
                    workList.push(CFLREdge(vi, vk, PT));
                }
        }

        /// PT ::= Addr VF
        if (A == Addr)
        {
            for (unsigned vk : succMap[vj][VF])
                if (!graph->hasEdge(vi, vk, PT))
                {
                    graph->addEdge(vi, vk, PT);
                    workList.push(CFLREdge(vi, vk, PT));
                }
        }

        /// VF ::= VF VF
        if (A == VF)
        {
            for (unsigned vk : succMap[vj][VF])
                if (!graph->hasEdge(vi, vk, VF))
                {
                    graph->addEdge(vi, vk, VF);
                    workList.push(CFLREdge(vi, vk, VF));
                }
        }

        /// VF ::= SV Load
        if (A == SV)
        {
            for (unsigned vk : succMap[vj][Load])
                if (!graph->hasEdge(vi, vk, VF))
                {
                    graph->addEdge(vi, vk, VF);
                    workList.push(CFLREdge(vi, vk, VF));
                }
        }

        /// VF ::= PV Load
        if (A == PV)
        {
            for (unsigned vk : succMap[vj][Load])
                if (!graph->hasEdge(vi, vk, VF))
                {
                    graph->addEdge(vi, vk, VF);
                    workList.push(CFLREdge(vi, vk, VF));
                }
        }

        /// VF ::= Store VP
        if (A == Store)
        {
            for (unsigned vk : succMap[vj][VP])
                if (!graph->hasEdge(vi, vk, VF))
                {
                    graph->addEdge(vi, vk, VF);
                    workList.push(CFLREdge(vi, vk, VF));
                }
        }

        /// VF ::= PV Store
        if (A == PV)
        {
            for (unsigned vk : succMap[vj][Store])
                if (!graph->hasEdge(vi, vk, VF))
                {
                    graph->addEdge(vi, vk, VF);
                    workList.push(CFLREdge(vi, vk, VF));
                }
        }

        /// VA ::= LV Load
        if (A == LV)
        {
            for (unsigned vk : succMap[vj][Load])
                if (!graph->hasEdge(vi, vk, VA))
                {
                    graph->addEdge(vi, vk, VA);
                    workList.push(CFLREdge(vi, vk, VA));
                }
        }

        /// SV ::= Store VA
        if (A == Store)
        {
            for (unsigned vk : succMap[vj][VA])
                if (!graph->hasEdge(vi, vk, SV))
                {
                    graph->addEdge(vi, vk, SV);
                    workList.push(CFLREdge(vi, vk, SV));
                }
        }

        /// SV ::= VA Store
        if (A == VA)
        {
            for (unsigned vk : succMap[vj][Store])
                if (!graph->hasEdge(vi, vk, SV))
                {
                    graph->addEdge(vi, vk, SV);
                    workList.push(CFLREdge(vi, vk, SV));
                }
        }

        /// PV ::= PT VA
        if (A == PT)
        {
            for (unsigned vk : succMap[vj][VA])
                if (!graph->hasEdge(vi, vk, PV))
                {
                    graph->addEdge(vi, vk, PV);
                    workList.push(CFLREdge(vi, vk, PV));
                }
        }

        /// VP ::= VA PT
        if (A == VA)
        {
            for (unsigned vk : succMap[vj][PT])
                if (!graph->hasEdge(vi, vk, VP))
                {
                    graph->addEdge(vi, vk, VP);
                    workList.push(CFLREdge(vi, vk, VP));
                }
        }

        /// LV ::= Load VA
        if (A == Load)
        {
            for (unsigned vk : succMap[vj][VA])
                if (!graph->hasEdge(vi, vk, LV))
                {
                    graph->addEdge(vi, vk, LV);
                    workList.push(CFLREdge(vi, vk, LV));
                }
        }

        /// VA ::= VF VA
        if (A == VF)
        {
            for (unsigned vk : succMap[vj][VA])
                if (!graph->hasEdge(vi, vk, VA))
                {
                    graph->addEdge(vi, vk, VA);
                    workList.push(CFLREdge(vi, vk, VA));
                }
        }

        /// VA ::= VA VF
        if (A == VA)
        {
            for (unsigned vk : succMap[vj][VF])
                if (!graph->hasEdge(vi, vk, VA))
                {
                    graph->addEdge(vi, vk, VA);
                    workList.push(CFLREdge(vi, vk, VA));
                }
        }

        /// VF ::= Copy `
        if (A == Copy)
        {
            if (!graph->hasEdge(vi, vj, VF))
            {
                graph->addEdge(vi, vj, VF);
                workList.push(CFLREdge(vi, vj, VF));
            }
        }

    } // end while
}
