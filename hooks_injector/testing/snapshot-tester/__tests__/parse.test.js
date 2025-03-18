const fs = require('fs');
const path = require('path');
const { parseFile, buildParser, parseFileToHookedFolder } = require('../parse-file');

const { cppFolderPath, snapshotFolderPath } = require('../../../../utils/paths');

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
      const outFile = parseFileToHookedFolder(filePath);
      const result = fs.readFileSync(outFile, 'utf-8');
      expect(result).toMatchSnapshot();
    });
  });
});