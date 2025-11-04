/**
 * SVFIR.cpp
 * @author kisslune
 */

#include "Graphs/SVFG.h"
#include "SVF-LLVM/SVFIRBuilder.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char** argv)
{
    int arg_num = 0;
    int extraArgc = 4;
    char** arg_value = new char*[argc + extraArgc];
    for (; arg_num < argc; ++arg_num) {
        arg_value[arg_num] = argv[arg_num];
    }
    std::vector<std::string> moduleNameVec;

    int orgArgNum = arg_num;
    arg_value[arg_num++] = (char*)"-model-arrays=true";
    arg_value[arg_num++] = (char*)"-pre-field-sensitive=false";
    arg_value[arg_num++] = (char*)"-model-consts=true";
    arg_value[arg_num++] = (char*)"-stat=false";
    assert(arg_num == (orgArgNum + extraArgc) && "more extra arguments? Change the value of extraArgc");

    moduleNameVec = OptionBase::parseOptions(arg_num, arg_value, "SVF IR", "[options] <input-bitcode...>");

    LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(moduleNameVec);

    // Instantiate an SVFIR builder
    SVFIRBuilder builder;
    cout << "Generating SVFIR(PAG), call graph and ICFG ..." << endl;

    // TODO: here, generate SVFIR(PAG), call graph and ICFG, and dump them to files
    //@{
    SVFIR* pag = builder.build();

    CallGraph* cg = const_cast<CallGraph*>(pag->getCallGraph());
    ICFG* icfg = const_cast<ICFG*>(pag->getICFG());


    //name
    std::string inputPath = moduleNameVec.empty() ? "module" : moduleNameVec[0];
    std::string baseName = getBaseName(inputPath);
    //std::string outputDir = "output";

    //cg->dump(outputDir + "/" + baseName + "_CallGraph");
    //icfg->dump(outputDir + "/" + baseName + "_ICFG");
    //cg->dump(baseName + "_CallGraph");
    //icfg->dump(baseName + "_ICFG");
    pag -> dump(); 
    cg -> dump(); 
    icfg -> dump(); 


    return 0;

    //@}

    return 0;
}
