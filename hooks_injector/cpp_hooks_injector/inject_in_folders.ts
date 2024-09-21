import * as fs from 'fs';
import { spawn } from 'child_process';
import * as path from 'path';

// Specify absolute paths of the folders to inject the code into
const foldersToInject = [
    path.normalize("D:/cr/src/third_party/blink/renderer/modules/clipboard"),
];

const prefix = path.normalize("D:/cr/src/third_party/blink/");

function getRelativePath(absolutePath: string): string {
    return absolutePath.replace(prefix, '').replaceAll('\\', '/');
}

// This generates a file with the code events to register source code on server

const extensions = [".cc", ".h"];

let CodeRunId = guidGenerator();
let CodeVersion = guidGenerator();
let CodeEvents: any = [];

function guidGenerator() {
    const S4 = function () {
        return (((1 + Math.random()) * 0x10000) | 0).toString(16).substring(1);
    };
    return (S4() + S4() + "-" + S4() + "-" + S4() + "-" + S4() + "-" + S4() + S4() + S4());
}

function findFiles(dir: string, exts: string[], fileList: string[] = []): string[] {
    const files = fs.readdirSync(dir);
    files.forEach(file => {
        const filePath = path.join(dir, file);
        const stat = fs.statSync(filePath);
        if (stat.isDirectory()) {
            // Is it not recrusive for now.
            // findFiles(filePath, exts, fileList);
        } else if (exts.includes(path.extname(file))) {
            fileList.push(filePath);
        }
    });
    return fileList;
}


function processFile(filePath: string): Promise<void> {
    return new Promise((resolve, reject) => {
        const fileContents = fs.readFileSync(filePath, 'utf-8');
        const env = { ...process.env, FileName: getRelativePath(filePath), CodeVersion: CodeVersion };
        const child = spawn('node', ['dist/out.js'], { env });

        let stdout = '';
        let stderr = '';

        child.stdin.write(fileContents);
        child.stdin.end();

        child.stdout.on('data', (data) => {
            stdout += data;
        });

        child.stderr.on('data', (data) => {
            stderr += data;
        });

        child.on('close', (code) => {
            if (code !== 0) {
                reject(`Error processing file ${filePath}: ${stderr}`);
            } else {
                fs.writeFileSync(filePath, stdout, 'utf-8');
                resolve();
            }
        });
    });
}
async function main() {
    try {
        let allFiles: string[] = [];
        foldersToInject.forEach(folder => {
            allFiles = findFiles(folder, extensions, allFiles);
        });

        for (const file of allFiles) {
            addSourceFile(getRelativePath(file), fs.readFileSync(file, 'utf-8'), CodeVersion);
            try {

                await processFile(file);
            } catch (err) {
                console.error('Error processing file:', err);
            }
        }

        // Write the code events to a file
        fs.writeFileSync('code_events.json', JSON.stringify(CodeEvents, null, 2));

        console.log('All files processed successfully for CodeVersion: ', CodeVersion);
    } catch (error) {
        console.error('Error:', error);
    }
}

main();


function addSourceFile(relativeFilePath: string, code: string, codeVersion: string): void {
    const eventType = "ADD_SOURCE_FILE";

    const payload: string[] = [];
    payload.push(relativeFilePath);
    payload.push(code);
    payload.push(codeVersion);

    DispatchCodeRunEvent(CodeRunId, eventType, payload);
}

function DispatchCodeRunEvent(cvId: string, eventType: string, payload: any) {
    const payload_json = JSON.stringify(payload);
    const codeevent = [cvId, eventType, payload_json];
    CodeEvents.push(JSON.stringify(codeevent));

}
