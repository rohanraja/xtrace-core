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
    let fullStderr = '';

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
      fullStderr += data.toString();
      fullStdout += data.toString(); // Include stderr in full output for compatibility
      if(onOutput){
        onOutput(data.toString());
      }
    });

    child.on('close', (code) => {
      // console.log(`Close invoked on command ${command}`);
      
      // Enhanced error detection for build failures
      const combinedOutput = fullStdout + fullStderr;
      const hasBuildFailure = detectBuildFailure(combinedOutput, command);
      
      if (code !== 0) {
        reject(`${fullStdout}\nExecution failed with code ${code}`);
      } else if (hasBuildFailure) {
        reject(`${fullStdout}\nBuild failure detected in output`);
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

/**
 * Detect build failures in command output even when exit code is 0
 * @param {string} output - Combined stdout and stderr output
 * @param {string} command - The command that was executed
 * @returns {boolean} True if build failure indicators are found
 */
function detectBuildFailure(output, command) {
  // Common build failure patterns
  const buildFailurePatterns = [
    /FAILED:/i,
    /BUILD FAILED/i,
    /compilation terminated/i,
    // /error:/i,
    /fatal error:/i,
    // /\berror\b.*:\s*\d+/i, // Error with line numbers
    /undefined reference to/i,
    /multiple definition of/i,
    /permission denied/i,
    /no such file or directory/i,
    /cannot find -l/i, // Missing library
    /collect2: error:/i,
    /ld: error:/i,
    /ninja: build stopped/i,
    /autoninja: error/i,
  ];

  // Check for build failure patterns
  const hasFailurePattern = buildFailurePatterns.some(pattern => pattern.test(output));

  // Print the pattern matches for debugging
  if (hasFailurePattern) {
    console.log(`Build failure detected in command: ${command}`);
    buildFailurePatterns.forEach(pattern => {
      const match = output.match(pattern);
      if (match) {
        console.log(`Matched pattern: ${pattern} - ${match[0]}`);
      }
    });
  }
  
  // Additional checks for specific build commands
  if (command.includes('autoninja') || command.includes('ninja')) {
    // For ninja builds, also check for specific ninja failure indicators
    const ninjaFailurePatterns = [
      /\[\d+\/\d+\] FAILED:/,
      /ninja: build stopped: subcommand failed/,
      /FAILED:/
    ];
    const hasNinjaFailure = ninjaFailurePatterns.some(pattern => pattern.test(output));
    return hasFailurePattern || hasNinjaFailure;
  }
  
  return hasFailurePattern;
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
  run,
  detectBuildFailure
};