const path = require('path');

const repoRoot = path.join(__dirname, '..', '..', "..");


// Tests paths
const snapshotTesterPath = path.join(repoRoot, "hooks_injector", "testing", "snapshot-tester");
const testsFolderPath = path.join(snapshotTesterPath, "__tests__");
const cppFolderPath = path.join(testsFolderPath, 'cpp');
const e2eHookedPath = path.join(testsFolderPath, 'e2e_cpp_hooked');
const e2eCppPath = path.join(testsFolderPath, 'e2e_cpp');
const binFolderPath = path.join(testsFolderPath, 'bin');
const snapshotFolderPath = path.join(testsFolderPath, '__snapshots__');
const cppHookInjectorPath = path.join(repoRoot, 'hooks_injector', 'cpp_hooks_injector');

// Cpp recorder paths
const xTracePath = path.join(repoRoot, 'cpp_recorder', 'xtrace');
const baseFolderForXTrace = path.join(repoRoot, 'cpp_recorder', 'base');

function getOutputBinaryPath(file) {
    return path.join(binFolderPath, path.basename(file, '.cc'));
}

function getBinaryLogFilePath(binaryPath) {
    return `${binaryPath}.log`;
}

module.exports = {
    cppFolderPath,
    e2eHookedPath,
    getOutputBinaryPath,
    getBinaryLogFilePath,
    e2eCppPath,
    binFolderPath,
    snapshotFolderPath,
    repoRoot,
    cppHookInjectorPath,
    xTracePath,
    baseFolderForXTrace
};
