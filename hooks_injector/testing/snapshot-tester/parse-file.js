const { execSync } = require("child_process");
const { cppHookInjectorPath } = require("../../../utils/paths");
const path = require("path");
const fs = require("fs");

/*
Given a cpp file path, triggers hooks_injector/cpp_hooks_injector/index.ts
with the filepath as argument and returns the output of the hooks_injector
*/
function parseFile(filePath) {

    const fileNameOnly = path.basename(filePath);

    const output = execSync(
        `node dist/out.js ${filePath}`, {
            cwd: cppHookInjectorPath,
            env: {
                ...process.env,
                FileName: fileNameOnly,
                "CodeVersion": "GUID_FROM_TEST"
            }
        }
    );
    return output.toString();
}

function parseFileToHookedFolder(filePath) {
    const fileDir = filePath.split(path.sep).slice(0, -1).join(path.sep);
    const outPutDir = `${fileDir}_hooked`;
    const fileNameOnly = path.basename(filePath);
    const outFileName = path.join(outPutDir, fileNameOnly);
    const outPut = parseFile(filePath);
    if (!fs.existsSync(outPutDir)) {
        fs.mkdirSync(outPutDir);
    }
    fs.writeFileSync(outFileName, outPut);
    return outFileName;
}

async function buildParser() {

    const output = execSync(
        `npm run build`, {
            cwd: cppHookInjectorPath,
        }
    );
    console.log(output.toString());
    return output.toString();
}

/*
Parse file with custom configuration
*/
function parseFileWithConfig(filePath, config = {}) {
    const fileNameOnly = path.basename(filePath);
    
    // Prepare environment variables
    const env = {
        ...process.env,
        FileName: fileNameOnly,
        "CodeVersion": "GUID_FROM_TEST"
    };
    
    // If config is provided, serialize it as JSON and pass via environment variable
    if (Object.keys(config).length > 0) {
        env.XTRACE_CONFIG = JSON.stringify(config);
    }

    const output = execSync(
        `node dist/out.js ${filePath}`, {
            cwd: cppHookInjectorPath,
            env: env
        }
    );
    return output.toString();
}

function parseFileToHookedFolderWithConfig(filePath, config = {}, suffix = '') {
    const fileDir = filePath.split(path.sep).slice(0, -1).join(path.sep);
    const outPutDir = `${fileDir}_hooked${suffix}`;
    const fileNameOnly = path.basename(filePath);
    const outFileName = path.join(outPutDir, fileNameOnly);
    const outPut = parseFileWithConfig(filePath, config);
    if (!fs.existsSync(outPutDir)) {
        fs.mkdirSync(outPutDir);
    }
    fs.writeFileSync(outFileName, outPut);
    return outFileName;
}

module.exports = {
    parseFile,
    buildParser,
    parseFileToHookedFolder,
    parseFileWithConfig,
    parseFileToHookedFolderWithConfig
}
