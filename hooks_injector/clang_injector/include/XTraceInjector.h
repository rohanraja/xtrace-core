#ifndef XTRACE_INJECTOR_H
#define XTRACE_INJECTOR_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "MethodVisitor.h"
#include "CodeRewriter.h"

namespace xtrace {

class XTraceInjector : public clang::ASTConsumer {
private:
    clang::Rewriter &rewriter_;
    std::unique_ptr<MethodVisitor> visitor_;
    std::unique_ptr<CodeRewriter> code_rewriter_;
    
public:
    explicit XTraceInjector(clang::Rewriter &rewriter);
    
    void HandleTranslationUnit(clang::ASTContext &context) override;
    
    std::string getRewrittenSource() const;
};

class XTraceInjectorAction : public clang::ASTFrontendAction {
private:
    clang::Rewriter rewriter_;
    
public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &compiler, 
        llvm::StringRef in_file) override;
    
    void EndSourceFileAction() override;
    
    std::string getRewrittenSource() const;
};

} // namespace xtrace

#endif // XTRACE_INJECTOR_H
