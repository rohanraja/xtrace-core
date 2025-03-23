const { run } = require('../common/utils/process_utils.js');
const path = require('path');
const fs = require('fs');
const { envPath } = require('../utils/paths.js');
require('dotenv').config({ path: envPath });

let sno = 0;

// Wrapper around "run_e2e.js" to captures the output in a log file
async function main(){

    // Start time for perf
    const startTime = new Date().getTime();
    // Get current date time in format like "Sep 25 2021 03:00 PM" in IST
    const date = new Date().toLocaleString('en-US', { timeZone: 'Asia/Kolkata' }).replace(/:/g, '-').replace(/ /g, '_').replace(/,/g, '').replace(/\\/g, '-').replace(/\//g, '-');
    console.log(date);
    let logFolder = path.join(__dirname, '..', 'logs/');

    let cmd = "node scripts/run_e2e.js";

    let filePath = "";

    if(process.argv.length > 2){
        // Provide filename
        const arg = process.argv[2];
        filePath = arg;
        cmd = cmd + " " + arg;
    }else{
        // Read filename from "tmp/active_run_file" and use it as cmd arg
        if(fs.existsSync('tmp/active_run_file')){
            const arg = fs.readFileSync('tmp/active_run_file', 'utf-8');
            filePath = arg;
            cmd = cmd + " " + arg;
        }
    }

    // get filename from path
    const fileNameOnly = path.basename(filePath);

    const fileSafeName = fileNameOnly

    // Find the next serial number for the log file
    while(fs.existsSync(logFolder +
        fileSafeName + `_${sno}`  + ".md")){
        sno++;
    }

    const logFile = logFolder + fileSafeName + `_${sno}`  + ".md";

    // Copy log filename to tmp/last_log_file
    fs.writeFileSync(path.join('tmp/last_log_file'), logFile);

    const onOutput = (data) => {
        // Append data to logfile
        fs.appendFileSync(logFile, data);
    };

    // Write starting date, time to log file
    fs.appendFileSync(logFile, "## Starting new run at :" + date + "\n");

    let outPut = "";
    try{
        outPut = await run(cmd, process.cwd(), {...process.env, "XTRACE_SNO": sno}, onOutput);
    }catch(e){
        console.log(`Error while running main e2e command ${e}`);
        outPut = e;
        fs.appendFileSync(logFile, e);
    }

    const endTime = new Date().getTime();
    const timeTaken = endTime - startTime;
    const timeTakenInMinutes = timeTaken / 60000;
    console.log(`Time taken: ${timeTakenInMinutes} minutes`);
    fs.appendFileSync(logFile, `## Time taken: ${timeTakenInMinutes} minutes`);
}

main();