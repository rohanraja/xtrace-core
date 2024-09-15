// #!/usr/bin/env ts-node

import Parser, { SyntaxNode, Tree } from 'tree-sitter';
import Cpp from 'tree-sitter-cpp';
import fs from 'fs';

const fileName = process.env["FileName"] || "main.cc";
const cvid = process.env["CodeVersion"] || "3c4e3b6b-2026-4b15-872c-07ce4463f59b";

const parser = new Parser();
parser.setLanguage(Cpp);

export enum NodeType {
    FunctionDefinition = "function_definition",
}

// Read sourcecode from stdin stream
const sourceCode = fs.readFileSync(0, 'utf-8');

function addLogLines(sourceCode: string): string {
    const tree: Tree = parser.parse(sourceCode);
    let modifiedSourceCode = sourceCode.split('\n');

    function visit(node: SyntaxNode) {
        if(node.type === NodeType.FunctionDefinition) {
            const bodyNode: SyntaxNode = (node as any).bodyNode
            const declaratorNode = (node as any).declaratorNode;
            // Find any node which has identifier in type
            let methodName = "";
            function findD(decNode: SyntaxNode){

                if(!methodName && decNode.type.includes("identifier")) {
                    methodName = decNode.text;
                    return;
                }
                for(const child of decNode.namedChildren){
                    findD(child);
                }
            }

            findD(declaratorNode);

            const statements = bodyNode.namedChildren.filter(x => isValidStatementType(x.type));
            statements.forEach((childNode: SyntaxNode, index: number) => {
                const lineNumber = childNode.startPosition.row;
                const columnNumber = childNode.startPosition.column;
                
                let lineData = "";

                if(index===0){
                    lineData += `XTrace *xtrace = XTrace::getInstance(); `
                    lineData += `std::string xtrace_mrid = xtrace->OnMethodEnter("${fileName}", "${methodName}", "${cvid}" ); `;
                }

                lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `

                if(index===statements.length-1){
                    lineData += `xtrace->FlushAllEventsToJSONFile(); `
                }

                modifiedSourceCode[lineNumber] = modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + modifiedSourceCode[lineNumber].slice(columnNumber).trim();
            });
            return;
        }
        node.namedChildren.forEach(visit);
    }

    visit(tree.rootNode);

    return modifiedSourceCode.join('\n');
}


function isValidStatementType(type: string){
    return type.includes("statement") || type.includes("declaration") || type.includes("definition");
}

console.log(`Injecting.... ${sourceCode}`);
const modifiedSourceCode = addLogLines(sourceCode);
console.log(modifiedSourceCode);