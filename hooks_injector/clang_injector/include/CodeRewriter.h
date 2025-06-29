#ifndef CODE_REWRITER_H
#define CODE_REWRITER_H

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/ASTContext.h"
#include "MethodVisitor.h"
#include <string>

namespace xtrace {

class CodeRewriter {
private:
    clang::Rewriter &rewriter_;
    clang::ASTContext *context_;
    
    std::string generateMethodEntryCode(const MethodInfo& method_info);
    std::string generateParameterLoggingCode(const MethodInfo& method_info);
    std::string generateIncludeDirectives();
    
    bool insertAtLocation(clang::SourceLocation loc, const std::string& code);
    bool insertIncludes();
    
public:
    explicit CodeRewriter(clang::Rewriter &rewriter, clang::ASTContext *context);
    
    bool injectMethodEntryCode(const MethodInfo& method_info);
    bool injectAllMethods(const std::vector<MethodInfo>& methods);
};

} // namespace xtrace

#endif // CODE_REWRITER_H
