#include "clang/Tooling/Tooling.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "XTraceInjector.h"
#include <iostream>
#include <memory>
#include <vector>

using namespace clang::tooling;

int main(int argc, const char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.cpp> [-- clang-args...]\n";
        std::cerr << "Example: " << argv[0] << " file.cpp -- -std=c++17\n";
        return 1;
    }
    
    std::string filename = argv[1];
    std::vector<std::string> clang_args;
    
    // Parse command line arguments
    bool after_dashdash = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--") {
            after_dashdash = true;
            continue;
        }
        if (after_dashdash) {
            clang_args.push_back(argv[i]);
        }
    }
    
    // If no clang args provided, use defaults
    if (clang_args.empty()) {
        clang_args.push_back("-std=c++17");
    }
    
    std::cerr << "Processing file: " << filename << std::endl;
    
    // Create a simple compilation database
    FixedCompilationDatabase compilation_db(".", clang_args);
    ClangTool tool(compilation_db, {filename});
    
    // Create our custom action factory
    auto action_factory = newFrontendActionFactory<xtrace::XTraceInjectorAction>();
    
    int result = tool.run(action_factory.get());
    
    if (result == 0) {
        std::cerr << "XTrace injection completed successfully" << std::endl;
    } else {
        std::cerr << "XTrace injection failed with error code: " << result << std::endl;
    }
    
    return result;
}
