# SkipVariablesHooking Parameter Implementation Tasks

## Overview
Add a `SkipVariablesHooking` parameter to the tree-sitter injector configuration that allows skipping variable hooking while preserving method entry and line logging functionality.

## Task List

### 1. Configuration System Updates

#### 1.1 Update inject_config.ts interface
- [x] Add `skipVariablesHooking?: boolean` to the `InjectConfig` interface
- [x] Set default value to `false` to maintain backward compatibility
- [x] Update JSDoc comments to document the new parameter

**Files to modify:**
- `hooks_injector/cpp_hooks_injector/inject_config.ts`

#### 1.2 Update config.ts exports
- [x] Export the new configuration value from config.ts
- [x] Add helper function to check if variable hooking should be skipped
- [x] Ensure the configuration is properly typed

**Files to modify:**
- `hooks_injector/cpp_hooks_injector/config.ts`

### 2. Logger Implementation Changes

#### 2.1 Update CodeLogger class to respect SkipVariablesHooking
- [x] Modify `insertLoggingCode` method to check SkipVariablesHooking flag
- [x] Skip variable tracking section when flag is enabled
- [x] Ensure method entry and line logging continue to work
- [x] Preserve parameter logging behavior (parameters are different from local variables)

**Files to modify:**
- `hooks_injector/cpp_hooks_injector/logger.ts` (lines ~490-500)

#### 2.2 Update generateParameterLoggingCode behavior
- [x] Parameter logging is now skipped when flag is enabled (implemented via conditional call)
- [x] Added conditional logic to skip parameter variable updates
- [x] Maintained method signature logging (generateMethodEntryCode continues to work)

**Files modified:**
- `hooks_injector/cpp_hooks_injector/logger.ts` (lines ~483-485)

#### 2.3 Update extractAssignmentInfo usage
- [x] Conditionally call extractAssignmentInfo based on SkipVariablesHooking flag
- [x] Ensured no performance impact when variable hooking is disabled
- [x] Kept assignment analysis for other potential uses only when needed

**Files to modify:**
- `hooks_injector/cpp_hooks_injector/logger.ts` (lines ~490-500)

### 3. Configuration File Schema Updates

#### 3.1 Update run_configs JSON5 files
- [x] Added skipVariablesHooking parameter to existing configuration files
- [x] Set appropriate values for different use cases (false for most, true for performance-focused)
- [x] Documented the parameter with clear comments explaining its purpose

**Files modified:**
- `run_configs/03_CBC_WPT_InvestigateFlakyness.json5` (set to false - default behavior)
- `run_configs/02_Editing_visibleUnits_compilefix.json5` (set to false - default behavior)  
- `run_configs/base.json5` (set to false - default behavior)
- `run_configs/ClipboardChange.json5` (set to true - performance-focused example)

#### 3.2 Update default_config.json
- [x] Added skipVariablesHooking with default value false to ensure backward compatibility
- [x] Ensured backward compatibility for existing configurations

**Files modified:**
- `hooks_injector/cpp_hooks_injector/default_config.json`

### 4. HTTP API Updates

#### 4.1 Update serve.ts to support new parameter
- [x] Accept skipVariablesHooking in JSON payload and query parameters
- [x] Pass configuration to CodeLogger appropriately via temporary override mechanism
- [x] Added skipVariablesHooking information to API response
- [x] Implemented proper cleanup with try/finally to ensure override is cleared

**Files modified:**
- `hooks_injector/cpp_hooks_injector/serve.ts` (added parameter handling and temporary override)
- `hooks_injector/cpp_hooks_injector/config.ts` (added setSkipVariablesHookingOverride function)

#### 4.2 Update API request/response documentation
- [x] Added skipVariablesHooking to request schema with detailed parameter documentation
- [x] Updated response examples to include skipVariablesHooking field
- [x] Documented behavior differences when flag is enabled vs disabled
- [x] Added comprehensive usage examples for both JSON and plain text requests
- [x] Updated curl and JavaScript examples to demonstrate skipVariablesHooking usage

**Files modified:**
- `hooks_injector/cpp_hooks_injector/README_HTTP_API.md`

### 5. Testing Updates

#### 5.1 Create unit tests for SkipVariablesHooking
- [x] Test that variable hooking is skipped when flag is true
- [x] Test that method entry and line logging still work
- [x] Test that parameter logging behavior is correct
- [x] Test backward compatibility (flag not present)

**Files created:**
- `hooks_injector/testing/snapshot-tester/__tests__/skipVariablesHooking.test.js`

#### 5.2 Create test input file and test infrastructure
- [x] Create test input file: `__tests__/cpp/skip_variables_test.cc`
- [x] Add parseFileWithConfig and parseFileToHookedFolderWithConfig functions
- [x] Support config overrides via environment variables
- [x] Fix config merging mechanism in GetConfigFromEnv()

**Files created/modified:**
- `hooks_injector/testing/snapshot-tester/__tests__/cpp/skip_variables_test.cc`
- `hooks_injector/testing/snapshot-tester/parse-file.js`
- `hooks_injector/cpp_hooks_injector/inject_config.ts`

#### 5.3 Run and validate comprehensive test suite
- [x] All 6 skipVariablesHooking tests pass successfully
- [x] Verify output contains no LocalVarUpdate calls when flag is true
- [x] Verify OnMethodEnter and LogLineRun calls are preserved
- [x] Performance test confirms reduced code generation
- [x] Snapshot testing for output verification

**Test Results:**
- ✅ All tests passing (6/6)
- ✅ Snapshots generated successfully  
- ✅ Performance benefits confirmed
- ✅ Backward compatibility verified

### 6. Documentation Updates

#### 6.1 Update DevOnboarding.md
- [ ] Document the new SkipVariablesHooking parameter
- [ ] Explain use cases for when to skip variable hooking
- [ ] Update configuration examples

**Files to modify:**
- `docs/DevOnboarding.md`

#### 6.2 Update configuration documentation
- [ ] Document the parameter in configuration guides
- [ ] Add examples of different configuration scenarios
- [ ] Explain performance implications

**Files to modify:**
- Configuration-related documentation files

### 7. Console UI Updates

#### 7.1 Update config editor UI
- [ ] Add checkbox/toggle for SkipVariablesHooking in config editor
- [ ] Update TypeScript interfaces for configuration
- [ ] Ensure UI validates the new parameter

**Files to modify:**
- `console-ui/src/app/config-editor/page.tsx`
- Related TypeScript interface files

#### 7.2 Update configuration validation
- [ ] Add client-side validation for SkipVariablesHooking
- [ ] Ensure proper type checking
- [ ] Update form handling logic

**Files to modify:**
- Console UI validation and form handling files

### 8. Environment Variable Support

#### 8.1 Add environment variable support
- [ ] Support `XTRACE_SKIP_VARIABLES_HOOKING` environment variable
- [ ] Environment variable should override config file setting
- [ ] Document environment variable in README

**Files to modify:**
- `hooks_injector/cpp_hooks_injector/inject_config.ts`
- `hooks_injector/cpp_hooks_injector/index.ts`

#### 8.2 Update CLI argument parsing
- [ ] Consider adding command-line flag support
- [ ] Update argument parsing logic if needed
- [ ] Document CLI usage

**Files to modify:**
- Command-line interface files

### 9. Performance Testing

#### 9.1 Benchmark performance improvement
- [ ] Measure injection time with SkipVariablesHooking enabled vs disabled
- [ ] Document performance gains in large files
- [ ] Create performance test suite

**Files to create:**
- Performance testing scripts
- Benchmark documentation

#### 9.2 Memory usage analysis
- [ ] Analyze memory usage reduction when skipping variable analysis
- [ ] Document memory usage improvements
- [ ] Create memory profiling tests

### 10. Edge Case Handling

#### 10.1 Handle mixed scenarios
- [ ] Test with files that have both variables and no variables
- [ ] Ensure proper behavior with complex C++ constructs
- [ ] Handle lambda expressions correctly

#### 10.2 Error handling
- [ ] Ensure graceful degradation if configuration is invalid
- [ ] Provide clear error messages for configuration issues
- [ ] Test error scenarios thoroughly

### 11. Migration Guide

#### 11.1 Create migration documentation
- [ ] Document how to update existing configurations
- [ ] Provide migration scripts if needed
- [ ] Create upgrade guide for users

**Files to create:**
- Migration guide documentation

### 12. Validation and Quality Assurance

#### 12.1 Code review checklist
- [ ] Ensure all code follows existing patterns
- [ ] Verify TypeScript types are correct
- [ ] Check for potential breaking changes

#### 12.2 Integration testing
- [ ] Test with real Chromium C++ files
- [ ] Verify no regression in existing functionality
- [ ] Test with various configuration combinations

#### 12.3 End-to-end testing
- [ ] Test complete pipeline with SkipVariablesHooking
- [ ] Verify CI/CD pipeline works correctly
- [ ] Test Docker containerization

## Success Criteria

- [ ] **Functional**: Variable hooking can be completely disabled via configuration
- [ ] **Preserved**: Method entry and line logging continue to work normally
- [ ] **Backward Compatible**: Existing configurations continue to work without changes
- [ ] **Performance**: Measurable performance improvement when variable hooking is disabled
- [ ] **Documented**: All new functionality is properly documented
- [ ] **Tested**: Comprehensive test coverage for the new feature
- [ ] **UI Integrated**: Configuration parameter is accessible through console UI

## Implementation Priority

1. **High Priority**: Configuration system updates (Tasks 1-2)
2. **High Priority**: Core logger implementation (Task 2)
3. **Medium Priority**: Testing and validation (Tasks 5, 12)
4. **Medium Priority**: Documentation updates (Task 6)
5. **Low Priority**: UI and tooling updates (Tasks 7, 11)
6. **Low Priority**: Performance analysis (Task 9)

## Estimated Timeline

- **Phase 1** (Core Implementation): 2-3 days
- **Phase 2** (Testing & Documentation): 2-3 days  
- **Phase 3** (UI & Integration): 1-2 days
- **Phase 4** (Validation & Polish): 1 day

**Total Estimated Time**: 6-9 days

---

*This task list provides a comprehensive roadmap for implementing the SkipVariablesHooking parameter while maintaining code quality, backward compatibility, and thorough testing.*
