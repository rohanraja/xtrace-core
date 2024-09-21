const run = require('./utils.js').run;
const path = require('path');
const fs = require('fs');
// Wrapper around "run_e2e.js" to captures the output in a log file


async function main(){

    // Get current date time in format like "Sep 25 2021 03:00 PM" in IST
    const date = new Date().toLocaleString('en-US', { timeZone: 'Asia/Kolkata' }).replace(/:/g, '-').replace(/ /g, '_').replace(/,/g, '').replace(/\\/g, '-').replace(/\//g, '-');
    console.log(date);
    const logFile = path.join(__dirname, '..', 'logs');

    let cmd = "node scripts/run_e2e.js";

    if(process.argv.length > 2){
        // Get all the steps to run
        const arg = process.argv[2];
        cmd = cmd + " " + arg;
    }

    const outPut = await run(cmd, process.cwd());

    // Write output to log file
    fs.writeFileSync(logFile + "/" + date + ".log", outPut);
}

main();