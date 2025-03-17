const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const { e2eHookedPath, binFolderPath, xTracePath, baseFolderForXTrace } = require('./paths');

// Ensure the bin folder exists
if (!fs.existsSync(binFolderPath)) {
    fs.mkdirSync(binFolderPath);
}

// Copy xTrace folder recursive to hooks_injector/testing/snapshot-tester/__tests__/__snapshots__/third_party
function copyXTraceFolder() {
    fs.cpSync(xTracePath, path.join(e2eHookedPath, 'third_party', 'xtrace'), { recursive: true }, (err) => {
        if (err) {
            console.error(`Error copying xTrace folder: ${err}`);
        } else {
            console.log('xTrace folder copied successfully.');
        }
    });
    fs.cpSync(baseFolderForXTrace, path.join(e2eHookedPath, 'base'), { recursive: true }, (err) => {
        if (err) {
            console.error(`Error copying base folder: ${err}`);
        } else {
            console.log('base folder copied successfully.');
        }
    });
}

// Compile a single .cc file
function compileFile(file, outputBinaryPath) {
    if(!outputBinaryPath) {
        outputBinaryPath = path.join(binFolderPath, path.basename(file, '.cc'));
    }
    try {
        // Delete the existing binary if it exists
        if (fs.existsSync(outputBinaryPath)) {
            fs.unlinkSync(outputBinaryPath);
            console.log(`Deleted existing binary: ${outputBinaryPath}`);
        }
        // Compile the .cc file
        execSync(`g++ -Wall -Wextra -std=c++20 -DXTRACE_LOCAL_RUN -o ${outputBinaryPath} ${file}`, { stdio: 'inherit' });
        console.log(`Compiled ${file} to ${outputBinaryPath}`);
        return outputBinaryPath;
    } catch (error) {
        console.error(`Failed to compile ${file}: ${error.message}`);
    }
}

// Run a single binary and rename the log file
function runBinary(binaryPath) {
    const file = path.basename(binaryPath);
    const xtraceRunLogPath = path.join(process.cwd(), 'xtrace.run.log');
    const newLogPath = path.join(binFolderPath, `${file}.run.log`);

    try {
        // Delete xtrace.run.log if it exists
        if (fs.existsSync(xtraceRunLogPath)) {
            fs.unlinkSync(xtraceRunLogPath);
            console.log(`Deleted existing xtrace.run.log`);
        }
        // Run the binary, set env XTRACE_SEED_0 to true
        execSync(`${binaryPath}`, { stdio: 'inherit', env: { ...process.env, XTRACE_SEED_0: 'true' } });

        // Rename the xtrace.run.log to binary_name.run.log
        fs.renameSync(xtraceRunLogPath, newLogPath);
        console.log(`Renamed ${xtraceRunLogPath} to ${newLogPath}`);

        return newLogPath;
    } catch (error) {
        console.error(`Failed to run ${binaryPath}: ${error.message}`);
    }
}

// Main function to process all .cc files
function main() {
    copyXTraceFolder();

    // Read all .cc files in the cpp directory
    fs.readdirSync(e2eHookedPath).forEach(file => {
        if (path.extname(file) === '.cc') {
            const filePath = path.join(e2eHookedPath, file);
            const outputBinaryPath = path.join(binFolderPath, path.basename(file, '.cc'));
            compileFile(filePath, outputBinaryPath);
        }
    });

    // Run each binary and rename the log file
    fs.readdirSync(binFolderPath).forEach(file => {
        if (path.extname(file) === '') {
            const binaryPath = path.join(binFolderPath, file);
            runBinary(binaryPath);
        }
    });
}

// main();

module.exports = {
    copyXTraceFolder,
    compileFile,
    runBinary,
};