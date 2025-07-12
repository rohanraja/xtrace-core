const fs = require('fs');
const path = require('path');
const { parseFileWithConfig, buildParser, parseFileToHookedFolderWithConfig } = require('../parse-file');

const { cppFolderPath, snapshotFolderPath } = require('../../../../utils/paths');

describe('SkipVariablesHooking Tests', () => {
  const testFileName = 'skip_variables_test.cc';
  const testFilePath = path.join(cppFolderPath, testFileName);

  beforeAll(async () => {
    // Trigger the build step
    await buildParser();

    // Ensure the snapshot folder exists
    if (!fs.existsSync(snapshotFolderPath)) {
      fs.mkdirSync(snapshotFolderPath);
    }
  });

  test('skipVariablesHooking: false (default behavior)', () => {
    const config = {
      skipVariablesHooking: false
    };
    
    const result = parseFileWithConfig(testFilePath, config);
    
    // Should contain LocalVarUpdate calls for variables
    expect(result).toContain('LocalVarUpdate');
    expect(result).toContain('OnMethodEnter');
    expect(result).toContain('LogLineRun');
    
    // Should track variables like localVar1, localVar2, etc.
    expect(result).toContain('"localVar1"');
    expect(result).toContain('"localVar2"');
    expect(result).toContain('"param1"');
    expect(result).toContain('"param2"');
    
    expect(result).toMatchSnapshot('skip-variables-false');
  });

  test('skipVariablesHooking: true (skip variable tracking)', () => {
    const config = {
      skipVariablesHooking: true
    };
    
    const result = parseFileWithConfig(testFilePath, config);
    
    // Should NOT contain LocalVarUpdate calls
    expect(result).not.toContain('LocalVarUpdate');
    
    // Should still contain method entry and line logging
    expect(result).toContain('OnMethodEnter');
    expect(result).toContain('LogLineRun');
    
    // Should NOT track variables
    expect(result).not.toContain('"localVar1"');
    expect(result).not.toContain('"localVar2"');
    expect(result).not.toContain('"param1"');
    expect(result).not.toContain('"param2"');
    
    expect(result).toMatchSnapshot('skip-variables-true');
  });

  test('backward compatibility: no config provided', () => {
    const result = parseFileWithConfig(testFilePath, {});
    
    // Should behave like skipVariablesHooking: false (default)
    expect(result).toContain('LocalVarUpdate');
    expect(result).toContain('OnMethodEnter');
    expect(result).toContain('LogLineRun');
    
    expect(result).toMatchSnapshot('skip-variables-default');
  });

  test('method entry logging preserved with skipVariablesHooking: true', () => {
    const config = {
      skipVariablesHooking: true
    };
    
    const result = parseFileWithConfig(testFilePath, config);
    
    // Should contain OnMethodEnter for all methods
    const methodEntryMatches = result.match(/OnMethodEnter/g);
    expect(methodEntryMatches).toBeTruthy();
    expect(methodEntryMatches.length).toBeGreaterThan(0);
    
    // Should contain method names in OnMethodEnter calls
    expect(result).toContain('TestMethod');
    expect(result).toContain('GetValue');
    expect(result).toContain('GlobalFunction');
  });

  test('line logging preserved with skipVariablesHooking: true', () => {
    const config = {
      skipVariablesHooking: true
    };
    
    const result = parseFileWithConfig(testFilePath, config);
    
    // Should contain LogLineRun calls for line tracking
    const lineRunMatches = result.match(/LogLineRun/g);
    expect(lineRunMatches).toBeTruthy();
    expect(lineRunMatches.length).toBeGreaterThan(0);
  });

  test('performance difference: less generated code with skipVariablesHooking: true', () => {
    const configFalse = { skipVariablesHooking: false };
    const configTrue = { skipVariablesHooking: true };
    
    const resultWithVars = parseFileWithConfig(testFilePath, configFalse);
    const resultWithoutVars = parseFileWithConfig(testFilePath, configTrue);
    
    // Result without variable tracking should be smaller
    expect(resultWithoutVars.length).toBeLessThan(resultWithVars.length);
    
    // Count LocalVarUpdate calls
    const varsWithTracking = (resultWithVars.match(/LocalVarUpdate/g) || []).length;
    const varsWithoutTracking = (resultWithoutVars.match(/LocalVarUpdate/g) || []).length;
    
    expect(varsWithTracking).toBeGreaterThan(0);
    expect(varsWithoutTracking).toBe(0);
  });
});
