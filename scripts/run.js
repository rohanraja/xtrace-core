const run = require('./utils.js').run;
const path = require('path');
const fs = require('fs');
const JSON5 = require('json5');
// Wrapper around "run_e2e.js" to captures the output in a log file

let json_config = "";

if(process.argv.length > 2){
    json_config = fs.readFileSync(process.argv[2], 'utf-8');
}else{
    // Read sourcecode from stdin stream
    json_config = fs.readFileSync(0, 'utf-8');
}

const config = JSON5.parse(json_config);

let sno = 0;

async function main(){

    // Start time for perf
    const startTime = new Date().getTime();
    // Get current date time in format like "Sep 25 2021 03:00 PM" in IST
    const date = new Date().toLocaleString('en-US', { timeZone: 'Asia/Kolkata' }).replace(/:/g, '-').replace(/ /g, '_').replace(/,/g, '').replace(/\\/g, '-').replace(/\//g, '-');
    console.log(date);
    let logFolder = path.join(__dirname, '..', 'logs/');

    let cmd = "node scripts/run_e2e.js";

    if(process.argv.length > 2){
        // Get all the steps to run
        const arg = process.argv[2];
        cmd = cmd + " " + arg;
    }

    const fileSafeName = config.name.replace(/[^a-z0-9]/gi, '_').toLowerCase();

    // Find the next serial number for the log file
    while(fs.existsSync(logFolder +
        fileSafeName + `_${sno}`  + ".log")){
        sno++;
    }

    const logFile = logFolder + fileSafeName + `_${sno}`  + ".log";

    const onOutput = (data) => {
        // Append data to logfile
        fs.appendFileSync(logFile, data);
    };

    let outPut = "";
    try{
        outPut = await run(cmd, process.cwd(), {...process.env, "XTRACE_SNO": sno}, onOutput);
    }catch(e){
        outPut = e;
        fs.appendFileSync(logFile, e);
    }

    const endTime = new Date().getTime();
    const timeTaken = endTime - startTime;
    const timeTakenInMinutes = timeTaken / 60000;
    console.log(`Time taken: ${timeTakenInMinutes} minutes`);
    fs.appendFileSync(logFile, `Time taken: ${timeTakenInMinutes} minutes`);

    // Write output to log file
    // fs.writeFileSync(logFile + "/" + date + ".log", outPut);
}

main();