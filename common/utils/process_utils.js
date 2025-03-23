const { execSync } = require('child_process');


// Works on all OS
function killAllProcessWithName(processName) {
    const osName = process.platform;
    let command;
    if (osName === 'win32') {
        command = `taskkill /F /IM ${processName}`;
    } else if (osName === 'darwin') {
        command = `pkill -f '${processName}'`;
    } else {
        command = `pkill -f '${processName}'`;
    }
    try {
        execSync(command);
        console.log(`Successfully killed all processes with name: ${processName}`);
    } catch (error) {
        console.error(`Error killing processes with name ${processName}:`, error.message);
    }
}

module.exports = {
  killAllProcessWithName
};