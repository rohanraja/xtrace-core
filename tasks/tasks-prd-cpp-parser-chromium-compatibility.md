# Task List: xTrace C++ Parser Chromium Compatibility

Based on the PRD `prd-cpp-parser-chromium-compatibility.md`, this task list guides implementation of enhanced C++ parsing for complete Chromium codebase compatibility.

## Relevant Files

- `hooks_injector/cpp_hooks_injector/package.json` - Update tree-sitter-cpp dependency version
- `hooks_injector/cpp_hooks_injector/parser.ts` - Enhance CodeParser class with custom post-processing
- `hooks_injector/cpp_hooks_injector/logger.ts` - Fix generateVariableUpdateCode null valueType errors
- `hooks_injector/cpp_hooks_injector/index.ts` - Add enhanced error reporting and validation
- `hooks_injector/cpp_hooks_injector/formatter.ts` - Ensure compatibility with enhanced parsing output
- `hooks_injector/cpp_hooks_injector/config.ts` - Add configuration for parser enhancements
- `hooks_injector/cpp_hooks_injector/chromium-patterns.ts` - New file for Chromium-specific AST processing
- `hooks_injector/cpp_hooks_injector/type-inference.ts` - New file for improved type inference logic
- `hooks_injector/cpp_hooks_injector/validation.ts` - New file for syntax validation utilities
- `hooks_injector/cpp_hooks_injector/tests/parser.test.ts` - Unit tests for parser enhancements
- `hooks_injector/cpp_hooks_injector/tests/chromium-patterns.test.ts` - Tests for Chromium pattern handling
- `hooks_injector/cpp_hooks_injector/tests/regression/` - Directory for regression test files
- `hooks_injector/cpp_hooks_injector/tests/fixtures/` - Test fixtures with Chromium code samples
- `scripts/test-parser-compatibility.js` - New automated testing script
- `scripts/validate-chromium-parsing.js` - New validation script for Chromium file processing

### Notes

- Tests should use existing npm test infrastructure in hooks_injector/cpp_hooks_injector
- Use `npm run test` to execute the test suite
- Regression tests should include actual Chromium file samples from the whitelist
- Consider using tree-sitter-cli for debugging AST parsing issues during development

## Tasks

- [ ] 1.0 Upgrade and Enhance Core Parser Dependencies
  - [ ] 1.1 Research latest tree-sitter-cpp version and compatibility with tree-sitter 0.21.1
  - [ ] 1.2 Update package.json with latest tree-sitter-cpp version
  - [ ] 1.3 Test existing parsing functionality after dependency upgrade
  - [ ] 1.4 Document any breaking changes from dependency upgrades
  - [ ] 1.5 Update build scripts if needed for new dependency versions

- [ ] 2.0 Implement Custom AST Post-Processing for Chromium Patterns
  - [ ] 2.1 Create chromium-patterns.ts module for pattern-specific handling
  - [ ] 2.2 Implement CORE_EXPORT annotation processing in parser.ts
  - [ ] 2.3 Add template specialization detection and handling
  - [ ] 2.4 Implement complex macro expansion support
  - [ ] 2.5 Add nested declaration and complex type definition processing
  - [ ] 2.6 Create pattern matching for pointer/reference declarators
  - [ ] 2.7 Add support for Chromium-specific namespace patterns
  - [ ] 2.8 Implement fallback parsing strategies for unrecognized patterns

- [ ] 3.0 Fix Type Inference and Error Handling in Code Generation
  - [ ] 3.1 Create type-inference.ts module for improved type detection
  - [ ] 3.2 Fix null valueType errors in generateVariableUpdateCode method
  - [ ] 3.3 Implement proper type inference for complex C++ types
  - [ ] 3.4 Add comprehensive error handling with detailed diagnostics
  - [ ] 3.5 Create validation.ts module for syntax validation utilities
  - [ ] 3.6 Add pre-output validation using tree-sitter parsing
  - [ ] 3.7 Implement detailed error reporting with line numbers and node types
  - [ ] 3.8 Add logging for parsing statistics and success rates

- [ ] 4.0 Build Automated Testing and Validation Framework
  - [ ] 4.1 Create regression test suite with existing successful parsing cases
  - [ ] 4.2 Implement automated failure detection system
  - [ ] 4.3 Create test-parser-compatibility.js script for batch testing
  - [ ] 4.4 Add test fixtures with real Chromium file samples
  - [ ] 4.5 Implement parsing success/failure rate reporting
  - [ ] 4.6 Create validation script for Chromium build environment testing
  - [ ] 4.7 Add progress indicators for batch processing
  - [ ] 4.8 Implement test runner that processes files one-by-one until failure

- [ ] 5.0 Integrate Enhanced Parser with Existing xTrace Workflow
  - [ ] 5.1 Ensure compatibility with build_each_file_separately workflow
  - [ ] 5.2 Preserve existing JSON5 configuration system compatibility
  - [ ] 5.3 Maintain backward compatibility with existing hook injection patterns
  - [ ] 5.4 Add configuration options for parser enhancement features
  - [ ] 5.5 Update documentation for new parsing capabilities
  - [ ] 5.6 Test integration with existing VS Code tasks and scripts
  - [ ] 5.7 Validate end-to-end workflow from parsing to Chromium compilation
  - [ ] 5.8 Create troubleshooting guide for common parsing issues
