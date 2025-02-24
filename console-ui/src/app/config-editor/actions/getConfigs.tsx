"use server";

import { readdir, readFile } from "fs/promises";
import path from "path";
import fs from "fs";
import { getConfigsPath, getLogPathForFile, getLogsPath } from "./path_utils";

export async function getConfigs() {
    try {
        // Get the list of files in the current working directory
        const configsPath = getConfigsPath();
        const files = await readdir(configsPath);

        const json5Files = files.filter((file) => file.endsWith(".json5"));

        // Convert the list of files into a comma-separated string
        // const fileNames = json5Files.join(", ");

        const configs = json5Files.map((file) => {
            const filePath = path.join(configsPath, file);
            const fileContent = fs.readFileSync(filePath, "utf-8");
            return {
                name: file,
                json: fileContent,
                logs: ["from server"]
            };
        });

        return configs;
    } catch (error) {
        console.error("Error reading directory:", error);
        return [];
    }
}

export async function saveConfig(fileName: string, fileContent: string) {
    try {
        const configsPath = getConfigsPath();
        const filePath = path.join(configsPath, fileName);
        fs.writeFileSync(filePath, fileContent);
        return true;
    } catch (error) {
        console.error("Error saving config:", error);
        return false;
    }
    
}

function getLogsNum(fileName: string): number {
    try {
        let sno = 0;
        while(fs.existsSync(getLogPathForFile(fileName, sno)))
        {
            sno++;
        }
        return sno;
    } catch (error) {
        console.error("Error saving config:", error);
        return -1;
    }
    
}

export async function getLastLogContents(fileName: string) {
    try {
        const lastNum = getLogsNum(fileName) - 1;
        if(lastNum < 0)
            return "ERROR, no logs found";
        const filePath = getLogPathForFile(fileName, lastNum);
        return fs.readFileSync(filePath, "utf-8");
    } catch (error) {
        console.error("Error saving config:", error);
        return "";
    }
    
}