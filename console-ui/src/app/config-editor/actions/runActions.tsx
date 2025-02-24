
"use server";

import path from "path";
import { getConfigsPath } from "./path_utils";
import { execFile } from "child_process";
const { spawn } = require('child_process');

export async function runE2E(fileName: string){
    const configsPath = getConfigsPath();
    const filePath = path.join(configsPath, fileName);

    // Run node scripts/run.js with filePath as argument and pwd/.. as cwd

    const cwd = path.join(configsPath, "..");

    const child = execFile('/home/rohan-linux/.nvm/versions/node/v18.15.0/bin/node', ['./scripts/run.js', filePath], { cwd, env: {...process.env, "DISPLAY": ":0"} });
    // const child = spawn('node', ['scripts/run.js', filePath], { cwd });
    child.stdout.on('data', (data: string) => {
        console.log(`stdout: ${data}`);
    }
    );
    child.stderr.on('data', (data: string) => {
        console.error(`stderr: ${data}`);
    }
    );
    child.on('close', (code: number) => {
        console.log(`child process exited with code ${code}`);
    }
    );

}