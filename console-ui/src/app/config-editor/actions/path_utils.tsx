// "use server";
import path from "path";
import fs from "fs";

export function getConfigsPath(){
    const configsPath = path.join(process.cwd(), "../run_configs");
    return configsPath;
}


export function getLogsPath(){
    return path.join(process.cwd(), "../logs");
}

export function getLogPathForFile(filename: string, logNo: number){
    const logsPath = getLogsPath();
    return path.join(logsPath, filename + `_${logNo}`  + ".log");
}