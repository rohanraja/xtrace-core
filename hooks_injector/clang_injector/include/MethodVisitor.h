#ifndef METHOD_VISITOR_H
#define METHOD_VISITOR_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include <vector>
#include <string>

namespace xtrace {

struct MethodInfo {
    clang::FunctionDecl* function_decl;
    std::string name;
    std::string filename;
    clang::SourceLocation start_loc;
    clang::SourceLocation body_start_loc;
    std::vector<clang::ParmVarDecl*> parameters;
    bool should_reset_code_run;
    
    MethodInfo(clang::FunctionDecl* decl, const std::string& fname) 
        : function_decl(decl), name(fname), should_reset_code_run(false) {}
};

class MethodVisitor : public clang::RecursiveASTVisitor<MethodVisitor> {
private:
    clang::ASTContext *context_;
    clang::SourceManager *source_manager_;
    std::vector<MethodInfo> discovered_methods_;
    
    std::string extractMethodName(clang::FunctionDecl* func_decl);
    std::string getFilename(clang::SourceLocation loc);
    bool shouldProcessMethod(const std::string& method_name);
    bool shouldResetCodeRun(const std::string& method_name);
    
public:
    explicit MethodVisitor(clang::ASTContext *context);
    
    bool VisitFunctionDecl(clang::FunctionDecl *func_decl);
    
    const std::vector<MethodInfo>& getDiscoveredMethods() const {
        return discovered_methods_;
    }
};

} // namespace xtrace

#endif // METHOD_VISITOR_H
