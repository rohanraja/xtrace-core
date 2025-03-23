// #!/usr/bin/env ts-node

import fs from 'fs';
import { CodeParser } from './parser';
import { CodeLogger } from './logger';
import { CodeFormatter } from './formatter';
import Parser, { SyntaxNode, Tree } from 'tree-sitter';

require('dotenv').config({ path: "../../config/.env", override: true });

// Read source code from file or stdin
let sourceCode = "";
if (process.argv.length > 2) {
    sourceCode = fs.readFileSync(process.argv[2], 'utf-8');
} else {
    sourceCode = fs.readFileSync(0, 'utf-8');
}

// Main execution
const parser = new CodeParser();
const tree = parser.parse(sourceCode);

const logger = new CodeLogger(sourceCode);
const modifiedSourceCode = logger.addLogLines(tree);

const formattedSourceCode = CodeFormatter.format(modifiedSourceCode);
console.log(formattedSourceCode);

// Check for syntax errors after formatting
const formattedTree = parser.parse(formattedSourceCode);
process.env["XT_OUTPUT_HAS_CLANG_ERROR"] = formattedTree.rootNode.hasError ? "true" : "false";

if (formattedTree.rootNode.hasError) {
    console.error(`XT_OUTPUT_HAS_CLANG_ERROR: ${process.env["XT_OUTPUT_HAS_CLANG_ERROR"]}`);
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