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
    await execCommand('cd hooks_injector/cpp_hooks_injector && npm install && npm install -g clang-format && npm run build && npm run test');
    await execCommand('cd hooks_injector/cpp_formatter && npm install');
    await execCommand('cd scripts && npm install');
    await execCommand('sudo apt-get update && sudo apt-get install -y dotnet-sdk-8.0');
    await execCommand('cd hooks_injector/testing/HooksInjectorCommonTests && dotnet test');
  } catch (error) {
    console.error('An error occurred:', error);
  }
};

runCommands();