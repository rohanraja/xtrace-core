const { exec } = require('child_process');
const fs = require('fs');

var start = (process.platform == 'darwin'? 'open': process.platform == 'win32'? 'start': 'xdg-open');

const open = (url) => {
    exec(`${start} ${url}`);
};


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