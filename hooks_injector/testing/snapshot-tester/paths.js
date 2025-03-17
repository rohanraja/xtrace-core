const path = require('path');

const testsFolderPath = path.join(__dirname, "__tests__");
const cppFolderPath = path.join(testsFolderPath, 'cpp');
const e2eHookedPath = path.join(testsFolderPath, 'e2e_hooked');
const e2eCppPath = path.join(testsFolderPath, 'e2e_cpp');
const binFolderPath = path.join(testsFolderPath, 'bin');
const snapshotFolderPath = path.join(testsFolderPath, '__snapshots__');
const repoRoot = path.join(__dirname, '..', '..', "..");
const xTracePath = path.join(repoRoot, 'cpp_recorder', 'xtrace');

module.exports = {
    cppFolderPath,
    e2eHookedPath,
    e2eCppPath,
    binFolderPath,
    snapshotFolderPath,
    repoRoot,
    xTracePath
};
