import { spawnSync } from 'child_process';

export class CodeFormatter {
    static format(sourceCode: string): string {
        const clangFormatPath = process.env["CLANG_FORMAT_PATH"] || "clang-format";
        const result = spawnSync(clangFormatPath, [], {
            input: sourceCode,
            encoding: 'utf-8',
            stdio: ['pipe', 'pipe', 'inherit'],
            shell: true
        });

        if (result.error) {
            throw result.error;
        }

        if (result.status !== 0) {
            throw new Error(`clang-format process exited with code ${result.status}`);
        }

        return result.stdout;
    }
}