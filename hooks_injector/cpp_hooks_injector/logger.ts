import { SyntaxNode } from 'tree-sitter';
import { config, fileName, cvid, methodsToInclude, methodsToExclude, primitiveTypes } from './config';

export class CodeLogger {
    private modifiedSourceCode: string[];

    constructor(sourceCode: string) {
        this.modifiedSourceCode = sourceCode.split('\n');
    }

    addLogLines(tree: Tree): string {
        if (tree.rootNode.hasError) {
            console.error("Input code has syntax errors. Skipping injection");
        }

        this.visit(tree.rootNode);

        return "#include \"third_party/xtrace/xtrace.h\"\n" + "#include \"base/strings/to_string.h\"\n" + this.modifiedSourceCode.join('\n');
    }

    private visit(node: SyntaxNode) {
        if (node.type === "function_definition") {
            this.handleFunctionDefinition(node);
        }
        node.namedChildren.forEach(child => this.visit(child));
    }

    private handleFunctionDefinition(node: SyntaxNode) {
        const bodyNode: SyntaxNode = (node as any).bodyNode;
        let declaratorNode = (node as any).declaratorNode;
        let methodName = this.findMethodName(declaratorNode);

        if (!methodName || !this.shouldIncludeMethod(methodName) || this.shouldExcludeMethod(methodName) || !bodyNode || !bodyNode.namedChildren) return;

        let shouldResetCodeRun = config.methods_which_split_run.some(methodNameCandidate => methodName.includes(methodNameCandidate));
        let params = this.findParameters(declaratorNode);

        const statements = bodyNode.namedChildren.filter(x => this.isValidStatementType(x.type));
        statements.forEach((childNode: SyntaxNode, index: number) => {
            this.addLogLine(childNode, index, statements.length, methodName, shouldResetCodeRun, params);
            if (childNode.namedChildCount > 0) {
                this.handleSyntaxNode(childNode);
            }
        });
    }

    private findMethodName(declaratorNode: SyntaxNode): string {
        let methodName = "";
        const findD = (decNode: SyntaxNode) => {
            if (!methodName && decNode.type.includes("identifier")) {
                methodName = decNode.text.replaceAll("\n", "");
                return;
            }
            for (const child of decNode.namedChildren) {
                findD(child);
            }
        };
        findD(declaratorNode);
        return methodName;
    }

    private shouldIncludeMethod(methodName: string): boolean {
        if (methodsToInclude.length === 0) return true;
        return methodsToInclude.some(methodNameCheck => methodName.includes(methodNameCheck));
    }

    private shouldExcludeMethod(methodName: string): boolean {
        return methodsToExclude.some(methodNameCheck => methodName.includes(methodNameCheck));
    }

    private findParameters(declaratorNode: SyntaxNode): SyntaxNode | undefined {
        let params = declaratorNode.namedChildren.find(x => x.type === "parameter_list");
        if (!params) {
            declaratorNode = declaratorNode.namedChildren.find(x => x.type === "function_declarator");
            if (declaratorNode) {
                params = declaratorNode.namedChildren.find(x => x.type === "parameter_list");
            }
        }
        return params;
    }

    private addLogLine(childNode: SyntaxNode, index: number, totalStatements: number, methodName: string, shouldResetCodeRun: boolean, params: SyntaxNode | undefined) {
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
                        if (primitiveTypes.includes(type_str.toLowerCase())) {
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
        if (assignmentStatement && childNode.type != "for_statement") {
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
                    if (primitiveTypes.includes(type_str.toLowerCase())) {
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

        this.modifiedSourceCode[lineNumber] = this.modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + this.modifiedSourceCode[lineNumber].slice(columnNumber).trim();
        this.modifiedSourceCode[endLineNumber] += lineDataAfterExec;
    }

    private handleSyntaxNode(node: any) {
        let statements = [];

        switch (node.type) {
            case "if_statement": {
                statements = node.consequenceNode.namedChildren;
                if (node.alternativeNode) {
                    statements = statements.concat(node.alternativeNode);
                }
                break;
            }
            case "switch_statement":
            case "for_statement":
            case "while_statement":
            case "for_range_loop": {
                statements = node.bodyNode.namedChildren;
                break;
            }
            case "else_clause": {
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
            case "declaration": {
                break;
            }
            default: {
                statements = node.namedChildren;
                break;
            }
        }
        statements.forEach((childNode: SyntaxNode, index: number) => {
            if (this.isValidStatementType(childNode.type)) {
                const lineNumber = childNode.startPosition.row;
                const columnNumber = childNode.startPosition.column;
                let lineData = "";
                lineData += `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `;
                this.modifiedSourceCode[lineNumber] = this.modifiedSourceCode[lineNumber].slice(0, columnNumber) + lineData + this.modifiedSourceCode[lineNumber].slice(columnNumber).trim();
            }
            if (childNode.namedChildCount > 0) {
                this.handleSyntaxNode(childNode);
            }
        });
    }

    private isValidStatementType(type: string) {
        return !type.includes("else") && !type.includes("case") && (type.includes("statement") || type.includes("declaration") || type.includes("definition") || type.includes("for_range_loop"));
    }
}