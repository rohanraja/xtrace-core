// #!/usr/bin/env ts-node

import fs from 'fs';
import { execSync } from 'child_process';
import path from 'path';
import { CodeParser } from './parser';
import { CodeLogger } from './logger';
import { CodeFormatter } from './formatter';
import Parser, { SyntaxNode, Tree } from 'tree-sitter';

require('dotenv').config({ path: "../../config/.env", override: true });

// Determine which injector to use
const useLegacyInjector = !(process.env.USE_CLANG_INJECTOR === 'true');

// Read source code from file or stdin
let sourceCode = "";
let inputFile = "";
if (process.argv.length > 2) {
    inputFile = process.argv[2];
    sourceCode = fs.readFileSync(inputFile, 'utf-8');
} else {
    sourceCode = fs.readFileSync(0, 'utf-8');
    inputFile = "/dev/stdin";
}

let modifiedSourceCode = "";
let hasError = false;

if (useLegacyInjector) {
    // Legacy tree-sitter approach
    console.error("Using legacy tree-sitter injector");
    
    const parser = new CodeParser();
    const tree = parser.parse(sourceCode);

    const logger = new CodeLogger(sourceCode);
    modifiedSourceCode = logger.addLogLines(tree);

    const formattedSourceCode = CodeFormatter.format(modifiedSourceCode);
    modifiedSourceCode = formattedSourceCode;

    // Check for syntax errors after formatting
    const formattedTree = parser.parse(formattedSourceCode);
    hasError = formattedTree.rootNode.hasError;
    
    if (hasError) {
        console.error(`XT_OUTPUT_HAS_CLANG_ERROR: true`);
        let errorNode = null;
        const visit = (node: SyntaxNode) => {
            if (node.isError) {
                errorNode = node;
                console.error(`XT_OUTPUT_ERROR_NODE: ${node.type} at line ${node.startPosition.row + 1}`);
            }
            node.namedChildren.forEach(visit);
        };
        visit(formattedTree.rootNode);
        if (errorNode) {
            console.error(`XT_OUTPUT_ERROR_NODE: ${errorNode.type} at line ${errorNode.startPosition.row}`);
        }
    }
} else {
    // New Clang LibTooling approach
    console.error("Using new Clang LibTooling injector");
    
    try {
        // Get the path to the Clang injector executable
        const clangInjectorPath = path.join(__dirname, '../../clang_injector/build/xtrace-clang-injector');
        
        // Check if the Clang injector exists
        if (!fs.existsSync(clangInjectorPath)) {
            console.error(`Clang injector not found at ${clangInjectorPath}`);
            console.error("Please build the Clang injector first: cd ../clang_injector && npm run build");
            process.exit(1);
        }
        
        let actualInputFile = inputFile;
        let tempFile = null;
        
        // Handle stdin input by creating a temporary file
        if (inputFile === "/dev/stdin") {
            const os = require('os');
            tempFile = path.join(os.tmpdir(), `xtrace_temp_${Date.now()}.cc`);
            fs.writeFileSync(tempFile, sourceCode);
            actualInputFile = tempFile;
        }
        
        try {
            // Prepare command arguments
            const args = [actualInputFile, '--', '-std=c++17'];
            const command = `${clangInjectorPath} ${args.join(' ')}`;
            
            // Execute the Clang injector
            modifiedSourceCode = execSync(command, { 
                encoding: 'utf-8',
                stdio: ['pipe', 'pipe', 'inherit'] // inherit stderr for diagnostic messages
            });
            
            hasError = false; // Clang injector handles its own error reporting
        } finally {
            // Clean up temporary file
            if (tempFile && fs.existsSync(tempFile)) {
                fs.unlinkSync(tempFile);
            }
        }
        
    } catch (error: any) {
        console.error("Clang injector failed:", error.message);
        console.error("Falling back to legacy tree-sitter injector");
        
        // Fallback to legacy approach
        const parser = new CodeParser();
        const tree = parser.parse(sourceCode);

        const logger = new CodeLogger(sourceCode);
        modifiedSourceCode = logger.addLogLines(tree);

        const formattedSourceCode = CodeFormatter.format(modifiedSourceCode);
        modifiedSourceCode = formattedSourceCode;

        // Check for syntax errors after formatting
        const formattedTree = parser.parse(formattedSourceCode);
        hasError = formattedTree.rootNode.hasError;
        
        if (hasError) {
            console.error(`XT_OUTPUT_HAS_CLANG_ERROR: true`);
        }
    }
}

// Output the final result
console.log(modifiedSourceCode);

// Set environment variable for error status
process.env["XT_OUTPUT_HAS_CLANG_ERROR"] = hasError ? "true" : "false";