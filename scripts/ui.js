const { exec } = require('child_process');
const fs = require('fs');

global.open_last_log = (params) => {
    const logFile = fs.readFileSync('tmp/last_log_file', 'utf-8');
    codeOpen(logFile);
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