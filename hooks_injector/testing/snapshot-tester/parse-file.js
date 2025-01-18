const { execSync } = require("child_process");

/*
Given a cpp file path, triggers hooks_injector/cpp_hooks_injector/index.ts
with the filepath as argument and returns the output of the hooks_injector
*/
function parseFile(filePath) {

    const fileNameOnly = filePath.split("/").pop();

    const output = execSync(
        `node dist/out.js ${filePath}`, {
            cwd: "../../cpp_hooks_injector",
            env: {
                ...process.env,
                FileName: fileNameOnly,
                "CodeVersion": "GUID_FROM_TEST"
            }
        }
    );
    return output.toString();
}
module.exports = parseFile;
