const fs = require('fs');
const path = require('path');
const { exec } = require('child_process');

const createJsonFile = (name) => {
  const dir = path.join(__dirname, '../run_configs');
  const baseFilePath = path.join(dir, 'base.json5');
  const newFilePath = path.join(dir, `${name}.json5`);

  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }

  if (!fs.existsSync(baseFilePath)) {
    console.error(`Base file not found: ${baseFilePath}`);
    process.exit(1);
  }

  const baseContent = fs.readFileSync(baseFilePath, 'utf8');
  fs.writeFileSync(newFilePath, baseContent);

  console.log(`Created ${newFilePath}`);

  // Open the files in VSCode
  exec(`code ${newFilePath}`, (error, stdout, stderr) => {
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

  // Create/update the active run file
    const activeRunFile = path.join(__dirname, '../tmp/active_run_file');
    fs.writeFileSync(activeRunFile, `${newFilePath}`);
};

const main = () => {
  if (process.argv.length !== 3) {
    console.error('Usage: node add_run_config.js <name>');
    process.exit(1);
  }

  const name = process.argv[2];
  createJsonFile(name);
};

main();