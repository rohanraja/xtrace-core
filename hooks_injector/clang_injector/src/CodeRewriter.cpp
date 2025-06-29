#include "CodeRewriter.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include <sstream>

namespace xtrace {

// Configuration constants - these would typically come from config files
static const std::string CVID = "GUID_FROM_TEST";

CodeRewriter::CodeRewriter(clang::Rewriter &rewriter, clang::ASTContext *context) 
    : rewriter_(rewriter), context_(context) {
}

bool CodeRewriter::injectAllMethods(const std::vector<MethodInfo>& methods) {
    bool success = true;
    
    // First, inject includes at the top of the file
    if (!insertIncludes()) {
        success = false;
    }
    
    // Then inject method entry code for each method
    for (const auto& method : methods) {
        if (!injectMethodEntryCode(method)) {
            success = false;
        }
    }
    
    return success;
}

bool CodeRewriter::injectMethodEntryCode(const MethodInfo& method_info) {
    if (!method_info.function_decl->hasBody()) {
        return false;
    }
    
    clang::Stmt* body = method_info.function_decl->getBody();
    if (!body) {
        return false;
    }
    
    // Find the location right after the opening brace of the function body
    clang::SourceLocation insertion_loc = body->getBeginLoc();
    
    // Move to the next line after the opening brace
    clang::SourceManager &source_mgr = rewriter_.getSourceMgr();
    insertion_loc = insertion_loc.getLocWithOffset(1);
    
    // Find the end of the current line and move to the beginning of the next line
    const char* buffer_ptr = source_mgr.getCharacterData(insertion_loc);
    while (*buffer_ptr != '\n' && *buffer_ptr != '\0') {
        insertion_loc = insertion_loc.getLocWithOffset(1);
        buffer_ptr = source_mgr.getCharacterData(insertion_loc);
    }
    if (*buffer_ptr == '\n') {
        insertion_loc = insertion_loc.getLocWithOffset(1);
    }
    
    // Generate the method entry code
    std::string method_entry_code = generateMethodEntryCode(method_info);
    
    // Generate parameter logging code if parameters exist
    if (!method_info.parameters.empty() && 
        method_info.name.find("TEST_") == std::string::npos) {
        method_entry_code += generateParameterLoggingCode(method_info);
    }
    
    // Insert the code
    return insertAtLocation(insertion_loc, method_entry_code);
}

std::string CodeRewriter::generateMethodEntryCode(const MethodInfo& method_info) {
    std::ostringstream code;
    
    // Generate proper indentation (assuming 2 spaces)
    code << "  blink::XTrace *xtrace = blink::XTrace::getInstance();\n";
    
    if (method_info.should_reset_code_run) {
        code << "  xtrace->ResetCodeRunId(\"" << method_info.name << "\");\n";
    }
    
    code << "  std::string xtrace_mrid = xtrace->OnMethodEnter(\""
         << method_info.filename << "\", \"" << method_info.name 
         << "\", \"" << CVID << "\");\n";
    
    return code.str();
}

std::string CodeRewriter::generateParameterLoggingCode(const MethodInfo& method_info) {
    std::ostringstream code;
    
    for (const auto* param : method_info.parameters) {
        std::string param_name = param->getNameAsString();
        if (param_name.empty()) {
            continue; // Skip unnamed parameters
        }
        
        // Get the type information
        clang::QualType param_type = param->getType();
        bool is_pointer = param_type->isPointerType();
        bool is_reference = param_type->isReferenceType();
        
        // Generate appropriate logging code based on type
        code << "  ";
        if (is_pointer) {
            code << "xtrace->LocalVarUpdate(xtrace_mrid, \"" << param_name 
                 << "\", " << param_name << " ? base::ToString(*" << param_name 
                 << ") : \"\");\n";
        } else if (is_reference) {
            code << "xtrace->LocalVarUpdate(xtrace_mrid, \"" << param_name 
                 << "\", base::ToString(" << param_name << "));\n";
        } else {
            code << "xtrace->LocalVarUpdate(xtrace_mrid, \"" << param_name 
                 << "\", base::ToString(" << param_name << "));\n";
        }
    }
    
    return code.str();
}

std::string CodeRewriter::generateIncludeDirectives() {
    return "#include \"third_party/xtrace/xtrace.h\"\n"
           "#include \"base/strings/to_string.h\"\n";
}

bool CodeRewriter::insertAtLocation(clang::SourceLocation loc, const std::string& code) {
    if (loc.isInvalid()) {
        return false;
    }
    
    return rewriter_.InsertText(loc, code);
}

bool CodeRewriter::insertIncludes() {
    clang::SourceManager &source_mgr = rewriter_.getSourceMgr();
    clang::FileID main_file_id = source_mgr.getMainFileID();
    clang::SourceLocation file_start = source_mgr.getLocForStartOfFile(main_file_id);
    
    std::string includes = generateIncludeDirectives();
    
    return insertAtLocation(file_start, includes);
}

} // namespace xtrace
