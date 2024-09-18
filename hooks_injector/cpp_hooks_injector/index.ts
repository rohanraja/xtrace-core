// #!/usr/bin/env ts-node

import Parser, { SyntaxNode, Tree } from 'tree-sitter';
import Cpp from 'tree-sitter-cpp';
import fs from 'fs';

const fileName = process.env["FileName"] || "clipboard_promise.cc";
const cvid = process.env["CodeVersion"] || "3c4e3b6b-2026-4b15-872c-07ce4463f59b";

const parser = new Parser();
parser.setLanguage(Cpp);

export enum NodeType {
    FunctionDefinition = "function_definition",
    ForRangeLoop = "for_range_loop",
    ForStatement = "for_statement",
    IfStatement = "if_statement",
    ElseClause = "else_clause",
}

export interface CompoundStatementNode extends SyntaxNode {
    statements: SyntaxNode[];
}
// Read sourcecode from stdin stream
const sourceCode = fs.readFileSync(0, 'utf-8');

function addLogLines(sourceCode: string): string {
    const tree: Tree = parser.parse(sourceCode);
    let modifiedSourceCode = sourceCode.split('\n');

    function visit(node: SyntaxNode) {
        if (node.type === NodeType.FunctionDefinition) {
            const bodyNode: SyntaxNode = (node as any).bodyNode
            const declaratorNode = (node as any).declaratorNode;
            // Find any node which has identifier in type
            let methodName = "";
            function findD(decNode: SyntaxNode) {

                if (!methodName && decNode.type.includes("identifier")) {
                    methodName = decNode.text;
                    return;
                }
                for (const child of decNode.namedChildren) {
                    findD(child);
                }
            }

            findD(declaratorNode);
            
            if(!bodyNode || !bodyNode.namedChildren) {
                return;
            }

            const statements = bodyNode.namedChildren.filter(x => isValidStatementType(x.type));
            statements.forEach((childNode: SyntaxNode, index: number) => {
                const lineNumber = childNode.startPosition.row;
                const columnNumber = childNode.startPosition.column;

                let lineData = "";

                if (index === 0) {
                    lineData += `XTrace *xtrace = XTrace::getInstance(); `
                    lineData += `std::string xtrace_mrid = xtrace->OnMethodEnter("${fileName}", "${methodName}", "${cvid}" ); `;
                }

                lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `

                if (index === statements.length - 1) {
                    lineData += `xtrace->FlushAllEventsToJSONFile(); `
                }

                modifiedSourceCode[lineNumber] = modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + modifiedSourceCode[lineNumber].slice(columnNumber).trim();
                if (childNode.namedChildCount > 0) {
                    modifiedSourceCode = handleSyntaxNode(childNode, modifiedSourceCode);
                }
            });
            return;
        }
        node.namedChildren.forEach(visit);
    }

    visit(tree.rootNode);

    return "#include \"third_party/xtrace/xtrace.h\"\n" + modifiedSourceCode.join('\n');
}
function handleSyntaxNode(node: SyntaxNode, modifiedSourceCode: string[]): string[] {
    let statements;
    // let statements: SyntaxNode[] = [];

    switch (node.type) {
        case NodeType.IfStatement:
        case NodeType.ElseClause: {
            statements = node.consequenceNode.namedChildren;
            if(node.alternativeNode) {
                statements = statements.concat(node.alternativeNode.namedChildren);
            }
            statements
            break;
        }
        case NodeType.ForStatement:
        case NodeType.ForRangeLoop: {
            statements = node.bodyNode.namedChildren;
            break;
        }
        default: {
            statements = node.namedChildren;
            break;
        }
    }
    statements.filter(x => isValidStatementType(x.type)).forEach((childNode: SyntaxNode, index: number) => {
        const lineNumber = childNode.startPosition.row;
        const columnNumber = childNode.startPosition.column;
        let lineData = "";
        lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `;
        modifiedSourceCode[lineNumber] = modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + modifiedSourceCode[lineNumber].slice(columnNumber).trim();
        if (childNode.namedChildCount > 0) {
            modifiedSourceCode = handleSyntaxNode(childNode, modifiedSourceCode);
        }
    });

    return modifiedSourceCode;
}

function isValidStatementType(type: string) {
    return !type.includes("else") && type.includes("statement") || type.includes("declaration") || type.includes("definition") || type.includes("for_range_loop");
}

// console.log(`Injecting.... ${sourceCode}`);
const modifiedSourceCode = addLogLines(sourceCode);
console.log(modifiedSourceCode);