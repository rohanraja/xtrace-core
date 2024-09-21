
const { exec } = require('child_process');
const { spawn } = require('child_process');
const FormData = require('form-data');
const path = require('path');
const util = require('util');
const fs = require('fs');
const axios = require('axios');

const test_input = {
    "folders_to_reset": [
        "Q:\\cr\\src\\third_party\\blink\\renderer\\modules\\clipboard"
    ],
    cr_path: "D:/cr"
}


async function run(command, cwd_p, env) {
  const cwd = cwd_p || process.cwd();
  const [cmd, ...args] = command.split(' ');

  return new Promise((resolve, reject) => {
    const child = spawn(cmd, args, { cwd, env, shell: true });
    let fullStdout = '';

    // On kill of parent process, kill the child process
    process.on('SIGINT', () => {
      console.log('SIGINT');
      child.kill();
      process.exit();
    });

    process.on('exit', async () => {
      console.log('Exiting');
      // Send Ctrl+C to child process
      child.kill('SIGINT');
      // Wait 5 seconds
    });

    child.stdout.on('data', (data) => {
      process.stdout.write(data);
      fullStdout += data.toString();
    });

    child.stderr.on('data', (data) => {
      process.stderr.write(`Error: ${data}`);
    });

    child.on('close', (code) => {
      if (code !== 0) {
        reject(`Execution failed with code ${code}`);
      } else {
        resolve(fullStdout);
      }
    });

    child.on('error', (error) => {
      reject(`Execution failed: ${error}`);
    });
  });
}


async function uploadFile(filePath, url) {
    const form = new FormData();
    form.append('file', fs.createReadStream(filePath));
  
    try {
      const response = await axios.post(url, form, {
        headers: {
          ...form.getHeaders(),
        },
      });
      console.log('File uploaded successfully:', response.data);
    } catch (error) {
      console.error('Error uploading file:', error);
    }
}

async function runStep(step_name, fn){
    if(process.argv.length > 2){
        // Get all the steps to run
        const steps = process.argv[2].split(',');
        if(!steps.includes(step_name)){
            console.log(`-- Skipping step: ${step_name}`);
            return;
        }
    }
    console.log(`*** Starting step: ${step_name}`);
    try {
        await fn();
        console.log(`***Completed step: ${step_name}`);
    } catch (error) {
        console.error(`XXXXX Error in step: ${step_name}`, error);
    }
}

module.exports = {
    run,
    uploadFile,
    runStep
}