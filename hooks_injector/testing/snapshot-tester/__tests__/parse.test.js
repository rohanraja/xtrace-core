const fs = require('fs');
const path = require('path');
const { parseFile, buildParser } = require('../parse-file');

const { cppFolderPath, snapshotFolderPath } = require('../paths');

describe('Parse File Tests', () => {
  beforeAll(async () => {
    // Trigger the build step
    await buildParser();

    // Ensure the snapshot folder exists
    if (!fs.existsSync(snapshotFolderPath)) {
      fs.mkdirSync(snapshotFolderPath);
    }
  });

  fs.readdirSync(cppFolderPath).forEach(file => {
    const filePath = path.join(cppFolderPath, file);
    const snapshotFilePath = path.join(snapshotFolderPath, `${file}`);
    test(`parse file: ${file}`, () => {
      const result = parseFile(filePath);

      // Save the snapshot result to a .cc file
      fs.writeFileSync(snapshotFilePath, result);

      expect(result).toMatchSnapshot();
    });
  });
});