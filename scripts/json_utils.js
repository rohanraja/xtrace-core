
const fs = require('fs');
const readline = require('readline');

async function convertFileToJsonArray(inputFilePath, outputFilePath) {
    const fileStream = fs.createReadStream(inputFilePath);
    const rl = readline.createInterface({
        input: fileStream,
        crlfDelay: Infinity
    });

    const jsonArray = [];

    for await (const line of rl) {
        try {
            const parsedLine = JSON.parse(line);
            jsonArray.push(JSON.stringify(parsedLine));
        } catch (error) {
            console.error(`Error parsing line: ${line}`, error);
        }
    }

    fs.writeFileSync(outputFilePath, JSON.stringify(jsonArray, null, 2));
    console.log(`Converted file saved to ${outputFilePath}`);
}

module.exports = {
    convertFileToJsonArray,
}