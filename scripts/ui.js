const { exec, execSync } = require('child_process');
const path = require('path');
const fs = require('fs');
const JSON5 = require('json5');
const { compileFile, runBinary, copyXTraceFolder } = require("../hooks_injector/testing/snapshot-tester/compile");
const { parseFileToHookedFolder, buildParser } = require('../hooks_injector/testing/snapshot-tester/parse-file');
const { getBinaryLogFilePath, getOutputBinaryPath } = require('../hooks_injector/testing/snapshot-tester/paths');

var start = (process.platform == 'darwin'? 'open': process.platform == 'win32'? 'start': 'xdg-open');

const open = (url) => {
    exec(`${start} ${url}`);
};


global.hook_and_run_file = (params) => {
    buildParser();
    copyXTraceFolder();
    const fileFromParams = params[0];
    console.log(`Hooking file: ${fileFromParams}`);
    const hookedCodeFile = parseFileToHookedFolder(fileFromParams);
    codeOpen(hookedCodeFile);
    let outputBinaryPath;
    try{
        outputBinaryPath = compileFile(hookedCodeFile);
        const logsFile = runBinary(outputBinaryPath);
        codeOpen(logsFile);
        const logs = fs.readFileSync(logsFile, 'utf-8');
        console.log(logs.toString());
    }catch(e){
        const outputBinaryPath = getOutputBinaryPath(hookedCodeFile);
        const compileLogsFile = getBinaryLogFilePath(outputBinaryPath);
        codeOpen(compileLogsFile);
    }

}

global.hook_active_file = (params) => {
    buildParser();
    const fileFromParams = params[0];
    console.log(`Hooking file: ${fileFromParams}`);
    const hookedCodeFile = parseFileToHookedFolder(fileFromParams);
    console.log(hookedCodeFile);
    codeOpen(hookedCodeFile);
}

global.compile_run_active_file = (params) => {
    copyXTraceFolder();
    const fileFromParams = params[0];
    console.log(`Running file: ${fileFromParams}`);
    const outputBinaryPath = compileFile(fileFromParams);
    // execute the binary
    const logs = execSync(outputBinaryPath);
    console.log(logs.toString());
}


global.open_whitelist_files = (params) => {
    const file = fs.readFileSync('tmp/active_run_file', 'utf-8');
    const baseContent = JSON5.parse(fs.readFileSync(file, 'utf8'));
    const srcDir = path.join(baseContent.cr_path, 'src');
    const whiteListFiles = baseContent.files_whitelist.map(f => path.join(srcDir, f));
    codeOpen(srcDir);

    // Open all files in the whitelist
    whiteListFiles.forEach(file => {
        codeOpen(file);
    });
}


global.open_src_dir = (params) => {
    const file = fs.readFileSync('tmp/active_run_file', 'utf-8');
    const baseContent = JSON5.parse(fs.readFileSync(file, 'utf8'));
    const srcDir = path.join(baseContent.cr_path, 'src');
    codeOpen(srcDir);
}

global.open_last_log = (params) => {
    const logFile = fs.readFileSync('tmp/last_log_file', 'utf-8');
    codeOpen(logFile);
}

global.open_active_runconfig = (params) => {
    const file = fs.readFileSync('tmp/active_run_file', 'utf-8');
    codeOpen(file);
}

global.open_last_recording = (params) => {
    const url = fs.readFileSync('tmp/last_recording_url', 'utf-8');
    console.log(`Opening last recording: ${url}`);
    // Open url in browser
    open(url);
}
function removeUntilFirstBrace(str) {
    const index = str.indexOf('{');
    if (index !== -1) {
        return str.substring(index);
    }
    return str; // Return the original string if '{' is not found
}

global.create_config_from_cl = async (params) => {
    // const cl = "6225479";
    // const cl = "6225479"; // Sw
    const cl = "6229607"; // SM

    const details_json_url = `https://chromium-review.googlesource.com/changes/chromium%2Fsrc~${cl}/detail?O=1996394`;
    try {
        const response = await fetch(details_json_url);
        const d = await response.text();
        const data = JSON.parse(removeUntilFirstBrace(d));
        console.log(data);
        

        const revision_number_last = data.revisions[data.current_revision]._number;
        const cl_title = data.subject;

        const files_url = `https://chromium-review.googlesource.com/changes/chromium%2Fsrc~${cl}/revisions/${revision_number_last}/files`
        const files_response = await fetch(files_url);
        const files_data = await files_response.text();
        const files = JSON.parse(removeUntilFirstBrace(files_data));
        const all_files = Object.keys(files);

        // Create the config file content
        const configContent = {
            cl,
            revision_number_last,
            cl_title,
            all_files
        };

        // Get 
        const name = `${cl}-${revision_number_last}`
        const dir = path.join(__dirname, '../run_configs');
        const baseFilePath = path.join(dir, 'base.json5');
        const newFilePath = path.join(dir, `${name}.json5`);

        const baseContent = JSON5.parse(fs.readFileSync(baseFilePath, 'utf8'));

        // Find first file which has "web_test"
        const web_test_file = configContent.all_files.find(f => f.endsWith('.html'));

        // Get all files which end with .cc
        const cc_files = configContent.all_files.filter(f => f.endsWith('.cc') && !f.endsWith("test.cc"));

        const outP = {
            ...baseContent,
            cl: configContent.cl, 
            patch_set: configContent.revision_number_last, 
            name: configContent.cl_title, 
            methods_which_split_run: [],
            web_test: `../../${web_test_file}`,
            files_whitelist: cc_files,
            web_page: "",

        };

        fs.writeFileSync(newFilePath, JSON.stringify(outP, null, 2));
        console.log(`Config file created at ${newFilePath}`);

        const activeRunFile = path.join(__dirname, '../tmp/active_run_file');
        fs.writeFileSync(activeRunFile, `${newFilePath}`);
        codeOpen(newFilePath);
        return configContent;
    } catch (error) {
        console.error(`Error fetching CL details: ${error}`);
        return null;
    }
}


// Utilities



function codeOpen(fileName){
    // exec command to open file in vscode
    exec(`code ${fileName}`, (err, stdout, stderr) => {
        if (err) {
            console.error(err);
            return;
        }
        console.log(stdout);
    });
}

function runAction(actionName, parameterList) {
    if (typeof global[actionName] === 'function') {
        global[actionName](parameterList);
    } else {
        console.log(`Unknown action: ${actionName}`);
    }
}

// Get command line arguments
const args = process.argv.slice(2);
const actionName = args[0];
const parameterList = args.slice(1);

// Run the action
runAction(actionName, parameterList);