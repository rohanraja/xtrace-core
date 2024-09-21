// #!/usr/bin/env ts-node

import Parser, { SyntaxNode, Tree } from 'tree-sitter';
import Cpp from 'tree-sitter-cpp';
import fs from 'fs';
import { spawnSync } from 'child_process';

const fileName = process.env["FileName"] || "main.cc";
const cvid = process.env["CodeVersion"] || "3c4e3b6b-2026-4b15-872c-07ce4463f59b";

const parser = new Parser();
parser.setLanguage(Cpp);

export enum NodeType {
    FunctionDefinition = "function_definition",
    ForRangeLoop = "for_range_loop",
    ForStatement = "for_statement",
    SwitchStatement = "switch_statement",
    IfStatement = "if_statement",
    ElseClause = "else_clause",
    Declaration = "declaration",
}

const primitive_types = ["int", "float", "double", "char", "string", "bool"];

export interface CompoundStatementNode extends SyntaxNode {
    statements: SyntaxNode[];
}

let sourceCode = "";

if(process.argv.length > 2){
    sourceCode = fs.readFileSync(process.argv[2], 'utf-8');
}else{
    // Read sourcecode from stdin stream
    sourceCode = fs.readFileSync(0, 'utf-8');
}

function addLogLines(sourceCode: string): string {
    let modifiedSourceCode = sourceCode.split('\n');
    const tree: Tree = parser.parse(sourceCode.replaceAll("class CORE_EXPORT", "class"));
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

            if (!bodyNode || !bodyNode.namedChildren) {
                return;
            }
            const params = declaratorNode.namedChildren.filter((x) => x.type === "parameter_list" /* ParameterDeclaration */)[0];
            const statements = bodyNode.namedChildren.filter(x => isValidStatementType(x.type));
            statements.forEach((childNode: SyntaxNode, index: number) => {
                if (isValidStatementType(childNode.type)) {
                    const lineNumber = childNode.startPosition.row;
                    const endLineNumber = childNode.endPosition.row;
                    const columnNumber = childNode.startPosition.column;

                    let lineData = "";
                    let lineDataAfterExec = "";

                    if (index === 0) {
                        lineData += `XTrace *xtrace = XTrace::getInstance(); `;
                        lineData += `std::string xtrace_mrid = xtrace->OnMethodEnter("${fileName}", "${methodName}", "${cvid}" );\n `;
                        if(params){
                        params.namedChildren.forEach((param) => {
                            const identifierNode = param.namedChildren.filter((x) => x.type.includes("identifier"))[0];
                            let identifierStr = "";
                            if(param.declaratorNode.type == "identifier"){
                                identifierStr = param.declaratorNode.text;
                            }else{
                                identifierStr = param.declaratorNode.namedChildren.filter((x) => x.type.includes("identifier"))[0]?.text;
                            }
                            const primitiveTypeNode = param.namedChildren.filter((x) => x.type.includes("primitive_type"))[0];
                            let primitive_type = primitiveTypeNode ? primitiveTypeNode.text : null;

                            if(!primitive_type){
                                const type_str = param.typeNode.text;

                                if(primitive_types.includes(type_str.toLowerCase())){
                                    primitive_type = type_str;
                                }
                            }


                            if(identifierStr != ""){
                                lineData += `xtrace->LocalVarUpdate(xtrace_mrid,"${identifierStr}", base::ToString(${identifierStr}));\n`;
                            }
                        });
                    }
                    }
                    const assignmentStatement = childNode.namedChildren.filter((x) => x.type.includes("init_declarator") || x.type.includes("assignment_expression"));
                    if (assignmentStatement) {
                        let identifiers, valueTypes;
                        assignmentStatement.forEach((param) => {
                            identifiers= param.namedChildren.filter((x) => x.type.includes("identifier"))[0];
                            valueTypes = param.namedChildren.filter((x) => x.type.includes("number_literal") || x.type.includes("string_literal") || x.type.includes("identifier"))[0];
                        });
                        let valueType = valueTypes ? valueTypes.type : null;
                        let identifier = identifiers ? identifiers.text : null;
                        let isPrimitive = false;
                        let primitive_type = "";
                        if(valueType === "identifier"){
                            const primitiveTypeNode = childNode.namedChildren.filter((x) => x.type.includes("primitive_type"))[0];
                            if(primitiveTypeNode){
                              isPrimitive = true;
                            }else if((childNode as any).typeNode){
                                const type_str = (childNode as any).typeNode.text;

                                if(primitive_types.includes(type_str.toLowerCase())){
                                    isPrimitive = true;
                                    primitive_type = type_str;
                                }

                            }
                        }
            
                        if (identifier != null) {
                            lineDataAfterExec += `xtrace->LocalVarUpdate(xtrace_mrid, "${identifier}", base::ToString(${identifier}));\n`;
                        }
                    }

                    lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `

                    if (index === statements.length - 1) {
                        lineData += `xtrace->FlushAllEventsToJSONFile(); `
                    }

                    modifiedSourceCode[lineNumber] = modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + modifiedSourceCode[lineNumber].slice(columnNumber).trim() ;
                    modifiedSourceCode[endLineNumber] += lineDataAfterExec;
                }
                if (childNode.namedChildCount > 0) {
                    modifiedSourceCode = handleSyntaxNode(childNode, modifiedSourceCode);
                }
            });
            return;
        }
        node.namedChildren.forEach(visit);
    }

    visit(tree.rootNode);

    return "#include \"third_party/xtrace/xtrace.h\"\n" + "#include \"base/strings/to_string.h\"\n" + modifiedSourceCode.join('\n');
}
function handleSyntaxNode(node: any, modifiedSourceCode: string[]): string[] {
    let statements = [];
    // let statements: SyntaxNode[] = [];

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

function isValidStatementType(type: string) {
    return !type.includes("else") && !type.includes("case") && (type.includes("statement") || type.includes("declaration") || type.includes("definition") || type.includes("for_range_loop"));
}

// console.log(`Injecting.... ${sourceCode}`);
const modifiedSourceCode = addLogLines(sourceCode);

const formattedSourceCode = formatSourceCode(modifiedSourceCode);
console.log(formattedSourceCode);

function formatSourceCode(sourceCode: string): string {
    const result = spawnSync("clang-format", [], {
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
