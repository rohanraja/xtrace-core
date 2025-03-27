const { exec } = require('child_process');

const execCommand = (command) => {
  return new Promise((resolve, reject) => {
    exec(command, (error, stdout, stderr) => {
      if (error) {
        console.error(`Error executing command: ${command}\n${error}`);
        resolve(error);
        return;
      }
      if (stderr) {
        console.error(`Error output: ${stderr}`);
        resolve(error);
        return;
      }
      console.log(`Output: ${stdout}`);
      resolve(stdout);
    });
  });
};

const runCommands = async () => {
  try {
    await execCommand('mkdir tmp');
    await execCommand('mkdir runs');
    await execCommand('cd hooks_injector/cpp_hooks_injector && npm install && npm install -g clang-format');
    await execCommand('cd scripts && npm install');
    await execCommand('cd common && npm install');
    await execCommand('cd hooks_injector/testing/snapshot-tester && npm install');
  } catch (error) {
    console.error('An error occurred:', error);
  }
};

runCommands();