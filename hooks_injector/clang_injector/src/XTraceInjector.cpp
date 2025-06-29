#include "XTraceInjector.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

namespace xtrace {

XTraceInjector::XTraceInjector(clang::Rewriter &rewriter) 
    : rewriter_(rewriter) {
}

void XTraceInjector::HandleTranslationUnit(clang::ASTContext &context) {
    // Initialize visitor and code rewriter
    visitor_ = std::make_unique<MethodVisitor>(&context);
    code_rewriter_ = std::make_unique<CodeRewriter>(rewriter_, &context);
    
    // Traverse the AST to find all methods
    visitor_->TraverseDecl(context.getTranslationUnitDecl());
    
    // Get discovered methods
    const auto& methods = visitor_->getDiscoveredMethods();
    
    // Inject code for all discovered methods
    code_rewriter_->injectAllMethods(methods);
}

std::string XTraceInjector::getRewrittenSource() const {
    const clang::RewriteBuffer *rewrite_buffer = 
        rewriter_.getRewriteBufferFor(rewriter_.getSourceMgr().getMainFileID());
    
    if (!rewrite_buffer) {
        return "";
    }
    
    return std::string(rewrite_buffer->begin(), rewrite_buffer->end());
}

std::unique_ptr<clang::ASTConsumer> XTraceInjectorAction::CreateASTConsumer(
    clang::CompilerInstance &compiler, 
    llvm::StringRef in_file) {
    
    rewriter_.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());
    return std::make_unique<XTraceInjector>(rewriter_);
}

void XTraceInjectorAction::EndSourceFileAction() {
    // Get the rewritten source
    const clang::RewriteBuffer *rewrite_buffer = 
        rewriter_.getRewriteBufferFor(rewriter_.getSourceMgr().getMainFileID());
    
    if (rewrite_buffer) {
        // Output the rewritten source
        llvm::outs() << std::string(rewrite_buffer->begin(), rewrite_buffer->end());
    } else {
        // If no changes were made, output the original source
        llvm::outs() << rewriter_.getSourceMgr().getBufferData(
            rewriter_.getSourceMgr().getMainFileID());
    }
}

std::string XTraceInjectorAction::getRewrittenSource() const {
    const clang::RewriteBuffer *rewrite_buffer = 
        rewriter_.getRewriteBufferFor(rewriter_.getSourceMgr().getMainFileID());
    
    if (!rewrite_buffer) {
        return "";
    }
    
    return std::string(rewrite_buffer->begin(), rewrite_buffer->end());
}

} // namespace xtrace
