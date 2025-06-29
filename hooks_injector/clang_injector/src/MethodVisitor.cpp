#include "MethodVisitor.h"
#include "clang/AST/Decl.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"
#include <set>

namespace xtrace {

// Configuration - these would typically be loaded from config files
static const std::set<std::string> methods_to_include = {
    // Empty means include all methods
};

static const std::set<std::string> methods_to_exclude = {
    "operator",
    "~",  // destructors
    "__",  // internal methods
    "TEST_",
    "EXPECT_",
    "ASSERT_"
};

static const std::set<std::string> methods_which_split_run = {
    "main",
    "OnStart",
    "Initialize"
};

MethodVisitor::MethodVisitor(clang::ASTContext *context) 
    : context_(context), source_manager_(&context->getSourceManager()) {
}

bool MethodVisitor::VisitFunctionDecl(clang::FunctionDecl *func_decl) {
    // Skip if not a definition (just a declaration)
    if (!func_decl->hasBody()) {
        return true;
    }
    
    // Skip if in system header
    if (source_manager_->isInSystemHeader(func_decl->getLocation())) {
        return true;
    }
    
    // Extract method name
    std::string method_name = extractMethodName(func_decl);
    
    // Skip if method should not be processed
    if (!shouldProcessMethod(method_name)) {
        return true;
    }
    
    // Get filename
    std::string filename = getFilename(func_decl->getLocation());
    
    // Create method info
    MethodInfo method_info(func_decl, filename);
    method_info.name = method_name;
    method_info.start_loc = func_decl->getLocation();
    method_info.should_reset_code_run = shouldResetCodeRun(method_name);
    
    // Get function body start location
    if (clang::Stmt* body = func_decl->getBody()) {
        method_info.body_start_loc = body->getBeginLoc();
    }
    
    // Extract parameters
    for (unsigned i = 0; i < func_decl->getNumParams(); ++i) {
        method_info.parameters.push_back(func_decl->getParamDecl(i));
    }
    
    discovered_methods_.push_back(method_info);
    
    return true;
}

std::string MethodVisitor::extractMethodName(clang::FunctionDecl* func_decl) {
    std::string name = func_decl->getNameAsString();
    
    // Handle method names with class qualifiers
    if (clang::CXXMethodDecl* method_decl = clang::dyn_cast<clang::CXXMethodDecl>(func_decl)) {
        if (clang::CXXRecordDecl* record_decl = method_decl->getParent()) {
            std::string class_name = record_decl->getNameAsString();
            name = class_name + "::" + name;
        }
    }
    
    return name;
}

std::string MethodVisitor::getFilename(clang::SourceLocation loc) {
    clang::FileID file_id = source_manager_->getFileID(loc);
    const clang::FileEntry* file_entry = source_manager_->getFileEntryForID(file_id);
    
    if (!file_entry) {
        return "unknown";
    }
    
    std::string full_path = file_entry->getName().str();
    
    // Extract just the filename from the full path
    size_t last_slash = full_path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return full_path.substr(last_slash + 1);
    }
    
    return full_path;
}

bool MethodVisitor::shouldProcessMethod(const std::string& method_name) {
    // Check exclusions first
    for (const auto& excluded : methods_to_exclude) {
        if (method_name.find(excluded) != std::string::npos) {
            return false;
        }
    }
    
    // If include list is empty, include all (except excluded)
    if (methods_to_include.empty()) {
        return true;
    }
    
    // Check if method is in include list
    for (const auto& included : methods_to_include) {
        if (method_name.find(included) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool MethodVisitor::shouldResetCodeRun(const std::string& method_name) {
    for (const auto& split_method : methods_which_split_run) {
        if (method_name.find(split_method) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace xtrace
