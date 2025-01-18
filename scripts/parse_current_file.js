const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const copyFileAndRunTest = (inputFilePath) => {
  const destDir = path.join(__dirname, '../hooks_injector/cpp_hooks_injector/tests');
  const destFilePath = path.join(destDir, 'inp.cc');

  if (!fs.existsSync(destDir)) {
    fs.mkdirSync(destDir, { recursive: true });
  }

  fs.copyFileSync(inputFilePath, destFilePath);
  console.log(`Copied ${inputFilePath} to ${destFilePath}`);

  const npmTestCommand = `npm run test`;
  exec(npmTestCommand, { cwd: path.join(__dirname, '../hooks_injector/cpp_hooks_injector') }, (error, stdout, stderr) => {
    if (error) {
      console.error(`Error running npm test: ${error.message}`);
      return;
    }
    if (stderr) {
      console.error(`Error output: ${stderr}`);
      return;
    }
    console.log(`Test output: ${stdout}`);
  });
};

const main = () => {
  if (process.argv.length !== 3) {
    console.error('Usage: node copy_and_test.js <input_file_path>');
    process.exit(1);
  }

  const inputFilePath = process.argv[2];
  copyFileAndRunTest(inputFilePath);
};

main();