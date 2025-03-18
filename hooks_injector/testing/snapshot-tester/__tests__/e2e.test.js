const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const { parseFile, buildParser } = require('../parse-file');
const { e2eCppPath, e2eHookedPath, binFolderPath } = require('../../../../utils/paths');
const { compileFile, runBinary, copyXTraceFolder } = require('../compile');
const { cleanTextForSnapshot } = require('../utils/snapshot_utils');


describe('Parse File Tests', () => {
  beforeAll(async () => {
    // Trigger the build step
    await buildParser();

    // Ensure the snapshot folder exists
    if (!fs.existsSync(e2eHookedPath)) {
      fs.mkdirSync(e2eHookedPath);
    }
  });

  fs.readdirSync(e2eCppPath).forEach(file => {
    const filePath = path.join(e2eCppPath, file);
    const cppHookedFile = path.join(e2eHookedPath, `${file}`);
    test(`parse file: ${file}`, () => {
      const result = parseFile(filePath);

      // Save the snapshot result to a .cc file
      fs.writeFileSync(cppHookedFile, result);

      expect(result).toMatchSnapshot();

      const outputBinaryPath = path.join(binFolderPath, path.basename(cppHookedFile, '.cc'));

      if (!fs.existsSync(outputBinaryPath)) {
        fs.mkdirSync(outputBinaryPath);
      }

      if(fs.existsSync(outputBinaryPath)) {
        fs.rmSync(outputBinaryPath, { recursive: true, force: true });
        console.log(`Deleted existing ${outputBinaryPath}`);
      }

      copyXTraceFolder();

      compileFile(cppHookedFile, outputBinaryPath);

      // Run the binary

      const logs = runBinary(outputBinaryPath);
      const logsData = fs.readFileSync(logs, 'utf-8');
      expect(cleanTextForSnapshot(logsData)).toMatchSnapshot();

    });
  });
});