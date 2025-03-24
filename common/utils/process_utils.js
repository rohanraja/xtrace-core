const { execSync } = require('child_process');
const { spawn } = require('child_process');

async function run(command, cwd_p, env, onOutput) {
  const cwd = cwd_p || process.cwd();
  const [cmd, ...args] = command.split(' ');
  console.log(`Running command: ${command} in ${cwd}`);
  const isWaited = env && env.WAIT_FOR_EXIT === 'true';

  return new Promise((resolve, reject) => {
    const child = spawn(cmd, args, { cwd, env, shell: true });
    let fullStdout = '';

    // On kill of parent process, kill the child process
    process.on('SIGINT', () => {
      // console.log('SIGINT');
      child.kill();
      if(isWaited){
        // console.log(`Resolving command ${command} from SIGINT waited`);
        resolve(fullStdout);
      }
      process.exit();
    });

    process.on('exit', async () => {
      // console.log('Exiting for command - ' + command);
      // Send Ctrl+C to child process
      child.kill('SIGINT');
      // Wait 5 seconds
      if(isWaited){
        resolve(fullStdout);
      }
    });

    child.stdout.on('data', (data) => {
      process.stdout.write(data);
      fullStdout += data.toString();
      if(onOutput){
        onOutput(data.toString());
      }
    });

    child.stderr.on('data', (data) => {
      process.stderr.write(`Error: ${data}`);
      fullStdout += data.toString();
      if(onOutput){
        onOutput(data.toString());
      }
    });

    child.on('close', (code) => {
      // console.log(`Close invoked on command ${command}`);
      if (code !== 0) {
        reject(`${fullStdout}\nExecution failed with code ${code}`);
      } else {
        resolve(fullStdout);
      }
    });

    child.on('error', (error) => {
      fullStdout += `Error: ${error}`;
      reject(fullStdout);
    });
  });
}



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
  killAllProcessWithName,
  run
};