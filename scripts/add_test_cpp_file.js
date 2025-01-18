const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const createCppFiles = (name) => {
  const dir = path.join(__dirname, '../hooks_injector/testing/cpp/cr');
  const cppFilePath = path.join(dir, `${name}.cc`);
  const expectedFilePath = path.join(dir, `${name}.expected.cc`);

  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }

  fs.writeFileSync(cppFilePath, ``);
  fs.writeFileSync(expectedFilePath, ``);

  console.log(`Created ${cppFilePath}`);
  console.log(`Created ${expectedFilePath}`);

  // Open the files in VSCode
  exec(`code ${cppFilePath} ${expectedFilePath}`, (error, stdout, stderr) => {
    if (error) {
      console.error(`Error opening files in VSCode: ${error.message}`);
      return;
    }
    if (stderr) {
      console.error(`Error output: ${stderr}`);
      return;
    }
    console.log(`Files opened in VSCode: ${stdout}`);
  });
};

const main = () => {
  if (process.argv.length !== 3) {
    console.error('Usage: node add_test_cpp_file.js <name>');
    process.exit(1);
  }

  let name = process.argv[2];
  // If name has ".cc" extension, remove it
  if (name.endsWith('.cc')) {
    name = name.slice(0, -3);
  }
  createCppFiles(name);
};

main();