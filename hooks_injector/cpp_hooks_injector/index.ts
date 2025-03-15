// #!/usr/bin/env ts-node

import Parser, { SyntaxNode, Tree } from 'tree-sitter';
import Cpp from 'tree-sitter-cpp';
import fs from 'fs';
import { spawnSync } from 'child_process';
import { GetConfigFromEnv } from './inject_config';

const config = GetConfigFromEnv();

const fileName = process.env["FileName"] || "main.cc";
const cvid = process.env["CodeVersion"] || "3c4e3b6b-2026-4b15-872c-07ce4463f59b";

const parser = new Parser();
parser.setLanguage(Cpp);

export enum NodeType {
    FunctionDefinition = "function_definition",
    ForRangeLoop = "for_range_loop",
    ForStatement = "for_statement",
    WhileStatement = "while_statement",
    SwitchStatement = "switch_statement",
    IfStatement = "if_statement",
    ElseClause = "else_clause",
    Declaration = "declaration",
}

const methodsToInclude = config.methods_whitelist || [];
const methodsToExclude = config.methods_blacklist || [];
const primitive_types = ["int", "float", "double", "char", "string", "bool"];

export interface CompoundStatementNode extends SyntaxNode {
    statements: SyntaxNode[];
}

let sourceCode = "";

// Read source code from file or stdin
if (process.argv.length > 2) {
    sourceCode = fs.readFileSync(process.argv[2], 'utf-8');
} else {
    sourceCode = fs.readFileSync(0, 'utf-8');
}

// Function to add log lines to the source code
function addLogLines(sourceCode: string): string {
    let modifiedSourceCode = sourceCode.split('\n');
    const tree: Tree = parser.parse(sourceCode.replaceAll("class CORE_EXPORT", "class"), undefined, { bufferSize: sourceCode.length + 10 });

    if (tree.rootNode.hasError) {
        console.error("Input code has syntax errors. Skipping injection");
    }

    function visit(node: SyntaxNode) {
        if (node.type === NodeType.FunctionDefinition) {
            handleFunctionDefinition(node, modifiedSourceCode);
        }
        node.namedChildren.forEach(visit);
    }

    visit(tree.rootNode);

    return "#include \"third_party/xtrace/xtrace.h\"\n" + "#include \"base/strings/to_string.h\"\n" + modifiedSourceCode.join('\n');
}

// Function to handle function definitions and add log lines
function handleFunctionDefinition(node: SyntaxNode, modifiedSourceCode: string[]) {
    const bodyNode: SyntaxNode = (node as any).bodyNode;
    let declaratorNode = (node as any).declaratorNode;
    let methodName = findMethodName(declaratorNode);

    if (!methodName) return;

    if (!shouldIncludeMethod(methodName)) return;

    if (shouldExcludeMethod(methodName)) return;

    if (!bodyNode || !bodyNode.namedChildren) return;

    let shouldResetCodeRun = config.methods_which_split_run.some(methodNameCandidate => methodName.includes(methodNameCandidate));

    let params = findParameters(declaratorNode);

    const statements = bodyNode.namedChildren.filter(x => isValidStatementType(x.type));
    statements.forEach((childNode: SyntaxNode, index: number) => {
        if (isValidStatementType(childNode.type)) {
            addLogLine(childNode, index, statements.length, methodName, shouldResetCodeRun, params, modifiedSourceCode);
        }
        if (childNode.namedChildCount > 0) {
            modifiedSourceCode = handleSyntaxNode(childNode, modifiedSourceCode);
        }
    });
}

// Function to find the method name from the declarator node
function findMethodName(declaratorNode: SyntaxNode): string {
    let methodName = "";
    function findD(decNode: SyntaxNode) {
        if (!methodName && decNode.type.includes("identifier")) {
            methodName = decNode.text.replaceAll("\n", "");
            return;
        }
        for (const child of decNode.namedChildren) {
            findD(child);
        }
    }
    findD(declaratorNode);
    return methodName;
}

// Function to check if a method should be included based on the whitelist
function shouldIncludeMethod(methodName: string): boolean {
    if (methodsToInclude.length === 0) return true;
    return methodsToInclude.some(methodNameCheck => methodName.includes(methodNameCheck));
}

// Function to check if a method should be excluded based on the blacklist
function shouldExcludeMethod(methodName: string): boolean {
    return methodsToExclude.some(methodNameCheck => methodName.includes(methodNameCheck));
}

// Function to find parameters from the declarator node
function findParameters(declaratorNode: SyntaxNode): SyntaxNode | undefined {
    let params = declaratorNode.namedChildren.find(x => x.type === "parameter_list");
    if (!params) {
        declaratorNode = declaratorNode.namedChildren.find(x => x.type === "function_declarator");
        if (declaratorNode) {
            params = declaratorNode.namedChildren.find(x => x.type === "parameter_list");
        }
    }
    return params;
}

// Function to add log lines to the source code
function addLogLine(childNode: SyntaxNode, index: number, totalStatements: number, methodName: string, shouldResetCodeRun: boolean, params: SyntaxNode | undefined, modifiedSourceCode: string[]) {
    const lineNumber = childNode.startPosition.row;
    const endLineNumber = childNode.endPosition.row;
    const columnNumber = childNode.startPosition.column;

    let lineData = "";
    let lineDataAfterExec = "";

    if (index === 0) {
        lineData += `blink::XTrace *xtrace = blink::XTrace::getInstance(); `;
        if (shouldResetCodeRun) {
            lineData += `xtrace->ResetCodeRunId("${methodName}"); `;
        }
        lineData += `std::string xtrace_mrid = xtrace->OnMethodEnter("${fileName}", "${methodName}", "${cvid}" );\n `;
        if (params) {
            params.namedChildren.forEach((param) => {
                const identifierNode = param.namedChildren.find((x) => x.type.includes("identifier"));
                let identifierStr = "";
                if(param.declaratorNode?.type == "identifier"){
                    identifierStr = param.declaratorNode?.text;
                }else{
                    identifierStr = param.declaratorNode?.namedChildren?.filter((x) => x.type.includes("identifier"))[0]?.text;
                    
                    // const pointerTypes= param.declaratorNode?.namedChildren?.filter((x) => x.type.includes("pointer_declarator"))[0];
                    // if(pointerTypes){
                    //     identifierStr = pointerTypes.namedChildren.filter((x) => x.type.includes("identifier"))[0]?.text;
                    // }
                }
                const isPointerType = param.declaratorNode?.type == "pointer_declarator";
                const primitiveTypeNode = param.namedChildren.find((x) => x.type.includes("primitive_type"));
                let primitive_type = primitiveTypeNode ? primitiveTypeNode.text : null;

                if (!primitive_type) {
                    const type_str = param.typeNode.text;
                    if (primitive_types.includes(type_str.toLowerCase())) {
                        primitive_type = type_str;
                    }
                }

                if (identifierStr) {
                    if (isPointerType) {
                        lineData += `xtrace->LocalVarUpdate(xtrace_mrid,"${identifierStr}",  ${identifierStr} ? base::ToString(*${identifierStr}) : "");\n`;
                    } else {
                        lineData += `xtrace->LocalVarUpdate(xtrace_mrid,"${identifierStr}", base::ToString(${identifierStr}));\n`;
                    }
                }
            });
        }
    }

    const assignmentStatement = childNode.namedChildren.filter((x) => x.type.includes("init_declarator") || x.type.includes("assignment_expression"));
    if (assignmentStatement && childNode.type != NodeType.ForStatement) {
        let identifiers, valueTypes, pointerTypes;
        assignmentStatement.forEach((param) => {
            identifiers = param.namedChildren.find((x) => x.type.includes("identifier"));
            valueTypes = param.namedChildren.find((x) => x.type.includes("number_literal") || x.type.includes("string_literal") || x.type.includes("identifier"));
            pointerTypes = param.namedChildren.find((x) => x.type.includes("pointer_declarator"));
            if (pointerTypes) {
                identifiers = pointerTypes.namedChildren.find((x) => x.type.includes("identifier"));
            }
        });
        let valueType = valueTypes ? valueTypes.type : null;
        let identifier = identifiers ? identifiers.text : null;
        let isPrimitive = false;
        let primitive_type = "";
        if (valueType === "identifier") {
            const primitiveTypeNode = childNode.namedChildren.find((x) => x.type.includes("primitive_type"));
            if (primitiveTypeNode) {
                isPrimitive = true;
            } else if ((childNode as any).typeNode) {
                const type_str = (childNode as any).typeNode.text;
                if (primitive_types.includes(type_str.toLowerCase())) {
                    isPrimitive = true;
                    primitive_type = type_str;
                }
            }
        }

        if (identifier) {
            if (pointerTypes) {
                lineDataAfterExec += `xtrace->LocalVarUpdate(xtrace_mrid, "${identifier}", ${identifier} ? base::ToString(*${identifier}) : "");\n`;
            } else {
                lineDataAfterExec += `xtrace->LocalVarUpdate(xtrace_mrid, "${identifier}", base::ToString(${identifier}));\n`;
            }
        }
    }

    lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `

    if (index === totalStatements - 1) {
        lineData += `xtrace->FlushAllEventsToJSONFile(); `
    }

    modifiedSourceCode[lineNumber] = modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + modifiedSourceCode[lineNumber].slice(columnNumber).trim();
    modifiedSourceCode[endLineNumber] += lineDataAfterExec;
}

// Function to handle different syntax nodes
function handleSyntaxNode(node: any, modifiedSourceCode: string[]): string[] {
    let statements = [];

    switch (node.type) {
        case NodeType.IfStatement: {
            statements = node.consequenceNode.namedChildren;
            if (node.alternativeNode) {
                statements = statements.concat(node.alternativeNode);
            }
            break;
        }
        case NodeType.SwitchStatement:
        case NodeType.ForStatement:
        case NodeType.WhileStatement:
        case NodeType.ForRangeLoop: {
            statements = node.bodyNode.namedChildren;
            break;
        }
        case NodeType.ElseClause: {
            if (node.namedChildren[0].type.includes("compound")) {
                statements = node.namedChildren[0].namedChildren;
            } else if (node.namedChildren[0].type.includes("if")) {
                node = node.namedChildren[0];
                statements = node.consequenceNode.namedChildren;
                if (node.alternativeNode) {
                    statements = statements.concat(node.alternativeNode);
                }
            } else {
                statements = node.namedChildren;
            }
            break;
        }
        case NodeType.Declaration: {
            break;
        }
        default: {
            statements = node.namedChildren;
            break;
        }
    }
    statements.forEach((childNode: SyntaxNode, index: number) => {
        if (isValidStatementType(childNode.type)) {
            const lineNumber = childNode.startPosition.row;
            const columnNumber = childNode.startPosition.column;
            let lineData = "";
            lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `;
            modifiedSourceCode[lineNumber] = modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + modifiedSourceCode[lineNumber].slice(columnNumber).trim();
        }
        if (childNode.namedChildCount > 0) {
            modifiedSourceCode = handleSyntaxNode(childNode, modifiedSourceCode);
        }
    });

    return modifiedSourceCode;
}

// Function to check if a statement type is valid
function isValidStatementType(type: string) {
    return !type.includes("else") && !type.includes("case") && (type.includes("statement") || type.includes("declaration") || type.includes("definition") || type.includes("for_range_loop"));
}

// Function to format the source code using clang-format
function formatSourceCode(sourceCode: string): string {
    const clang_format_path = process.env["CLANG_FORMAT_PATH"] || "clang-format";
    const result = spawnSync(clang_format_path, [], {
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

// Main execution
const modifiedSourceCode = addLogLines(sourceCode);
const formattedSourceCode = formatSourceCode(modifiedSourceCode);
console.log(formattedSourceCode);

const tree: Tree = parser.parse(formattedSourceCode, undefined, { bufferSize: formattedSourceCode.length + 10 });
process.env["XT_OUTPUT_HAS_CLANG_ERROR"] = tree.rootNode.hasError ? "true" : "false";

if (tree.rootNode.hasError) {
    console.error(`XT_OUTPUT_HAS_CLANG_ERROR: ${process.env["XT_OUTPUT_HAS_CLANG_ERROR"]}`);
    let errorNode = null;
    function visit(node: SyntaxNode) {
        if (node.isError) {
            errorNode = node;
            console.error(`XT_OUTPUT_ERROR_NODE: ${node.type} at line ${node.startPosition.row + 1}`);
        }
        node.namedChildren.forEach(visit);
    }
    visit(tree.rootNode);
    if (errorNode) {
        console.error(`XT_OUTPUT_ERROR_NODE: ${errorNode.type} at line ${errorNode.startPosition.row}`);
    }
}