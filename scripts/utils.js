
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
        // console.log(`Resolving command ${command} from onExit waited`);
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
        // console.log(`Rejecting command ${command} from onClose`);
        reject(`${fullStdout}\nExecution failed with code ${code}`);
      } else {
        // if(!isWaited){
        //   console.log(`Resolving command ${command} from onClose`);
        //   resolve(fullStdout);
        // }
          // console.log(`Resolving command ${command} from onClose`);
        resolve(fullStdout);
      }
    });

    child.on('error', (error) => {
      fullStdout += `Error: ${error}`;
      // console.log(`Error in commnd ${command}: ${error}`);
      reject(fullStdout);
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

async function runStep(step_name, fn, filterStr){
    if(filterStr){
        // Get all the steps to run
        const steps = filterStr.split(',');
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