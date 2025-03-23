const { exec } = require('child_process');

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

module.exports = {
    codeOpen
}