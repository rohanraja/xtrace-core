import { SyntaxNode, Tree } from 'tree-sitter';
import { config, fileName, cvid, methodsToInclude, methodsToExclude, primitiveTypes } from './config';

/**
 * Interfaces and Types
 */
interface MethodInfo {
    name: string;
    params?: SyntaxNode;
    shouldResetCodeRun: boolean;
}

interface AssignmentInfo {
    identifier: string | null;
    valueType: string | null;
    isPointer: boolean;
    isPrimitive: boolean;
    primitiveType: string;
}

/**
 * CodeLogger class for adding logging statements to C++ code
 */
export class CodeLogger {
    private modifiedSourceCode: string[];

    constructor(sourceCode: string) {
        this.modifiedSourceCode = sourceCode.split('\n');
    }

    /**
     * Main method to add log lines to the source code
     */
    addLogLines(tree: Tree): string {
        if (tree.rootNode.hasError) {
            console.error("Input code has syntax errors. Skipping injection");
        }

        this.visit(tree.rootNode);

        return this.getModifiedSourceWithIncludes();
    }

    private getModifiedSourceWithIncludes(): string {
        const includes = [
            "#include \"third_party/xtrace/xtrace.h\"",
            "#include \"base/strings/to_string.h\""
        ];
        return includes.join('\n') + '\n' + this.modifiedSourceCode.join('\n');
    }

    /**
     * Tree traversal methods
     */
    private visit(node: SyntaxNode): void {
        if (node.type === "function_definition") {
            this.handleFunctionDefinition(node);
        }
        node.namedChildren.forEach(child => this.visit(child));
    }

    /**
     * Function/Method processing
     */
    private handleFunctionDefinition(node: SyntaxNode, isLambda = false): void {
        const bodyNode: SyntaxNode = (node as any).bodyNode;
        let declaratorNode = (node as any).declaratorNode;
        
        // Get method information
        const methodInfo = this.extractMethodInfo(declaratorNode, isLambda);
        
        // Skip if method should not be processed
        if (!this.shouldProcessMethod(methodInfo, bodyNode)) return;

        // Process statements in the function body
        const statements = this.getValidStatements(bodyNode);
        this.processNodes(statements, methodInfo, true);
    }

    private extractMethodInfo(declaratorNode: SyntaxNode, isLambda: boolean): MethodInfo {
        const methodName = isLambda ? "lambda" : this.findMethodName(declaratorNode);
        const shouldResetCodeRun = config.methods_which_split_run.some(
            candidate => methodName.includes(candidate)
        );
        const params = this.findParameters(declaratorNode);
        
        return { name: methodName, params, shouldResetCodeRun };
    }

    private shouldProcessMethod(methodInfo: MethodInfo, bodyNode?: SyntaxNode): boolean {
        return !!(
            methodInfo.name && 
            this.shouldIncludeMethod(methodInfo.name) && 
            !this.shouldExcludeMethod(methodInfo.name) && 
            bodyNode && 
            bodyNode.namedChildren
        );
    }

    private getValidStatements(bodyNode: SyntaxNode): SyntaxNode[] {
        return bodyNode.namedChildren.filter(x => this.isValidStatementType(x.type));
    }

    /**
     * Unified statement processing methods
     */
    private processNodes(
        statements: SyntaxNode[], 
        methodInfo?: MethodInfo, 
        isTopLevelFunction: boolean = false
    ): void {
        statements.forEach((childNode: SyntaxNode, index: number) => {
            if (this.isValidStatementType(childNode.type)) {
                this.insertLoggingCode(childNode, index, statements.length, methodInfo, isTopLevelFunction);
            }
            
            if (childNode.namedChildCount > 0) {
                this.handleSyntaxNode(childNode);
            }
        });
    }

    /**
     * Node analysis methods
     */
    private findMethodName(declaratorNode: SyntaxNode): string {
        let methodName = "";
        
        const findIdentifier = (decNode: SyntaxNode) => {
            if (!methodName && decNode.type.includes("identifier")) {
                methodName = decNode.text.replaceAll("\n", "");
                return;
            }
            for (const child of decNode.namedChildren) {
                findIdentifier(child);
            }
        };
        
        findIdentifier(declaratorNode);
        return methodName;
    }

    private findParameters(declaratorNode: SyntaxNode): SyntaxNode | undefined {
        // Try to find direct parameter list
        let params = declaratorNode.namedChildren.find(x => x.type === "parameter_list");
        
        // If not found, look inside function_declarator
        if (!params) {
            const functionDeclarator = declaratorNode.namedChildren.find(x => 
                x.type === "function_declarator"
            );
            
            if (functionDeclarator) {
                params = functionDeclarator.namedChildren.find(x => 
                    x.type === "parameter_list"
                );
            }
        }
        
        return params;
    }

    private shouldIncludeMethod(methodName: string): boolean {
        if (methodsToInclude.length === 0) return true;
        return methodsToInclude.some(methodNameCheck => 
            methodName.includes(methodNameCheck)
        );
    }

    private shouldExcludeMethod(methodName: string): boolean {
        return methodsToExclude.some(methodNameCheck => 
            methodName.includes(methodNameCheck)
        );
    }

    private isValidStatementType(type: string): boolean {
        return !type.includes("else") && 
               !type.includes("case") && 
               (type.includes("statement") || 
                type.includes("declaration") || 
                type.includes("definition") || 
                type.includes("for_range_loop"));
    }

    /**
     * Assignment handling
     */
    private extractAssignmentInfo(childNode: SyntaxNode): AssignmentInfo {
        const assignmentStatement = childNode.namedChildren.filter((x) => 
            x.type.includes("init_declarator") || 
            x.type.includes("assignment_expression")
        );
        
        let identifier = null;
        let valueType = null;
        let isPointer = false;
        let isPrimitive = false;
        let primitiveType = "";
        
        if (assignmentStatement.length > 0) {
            assignmentStatement.forEach((param) => {
                let identifierNode = param.namedChildren.find((x) => 
                    x.type.includes("identifier")
                );
                
                const valueTypeNode = param.namedChildren.find((x) => 
                    x.type.includes("number_literal") || 
                    x.type.includes("string_literal") || 
                    x.type.includes("identifier")
                );
                
                const pointerTypeNode = param.namedChildren.find((x) => 
                    x.type.includes("pointer_declarator")
                );
                
                if (pointerTypeNode) {
                    identifierNode = pointerTypeNode.namedChildren.find((x) => 
                        x.type.includes("identifier")
                    );
                    isPointer = true;
                }
                
                identifier = identifierNode ? identifierNode.text : null;
                valueType = valueTypeNode ? valueTypeNode.type : null;
            });
            
            // Check if primitive type
            if (valueType === "identifier") {
                const primitiveTypeNode = childNode.namedChildren.find((x) => 
                    x.type.includes("primitive_type")
                );
                
                if (primitiveTypeNode) {
                    isPrimitive = true;
                } else if ((childNode as any).typeNode) {
                    const typeStr = (childNode as any).typeNode.text;
                    if (primitiveTypes.includes(typeStr.toLowerCase())) {
                        isPrimitive = true;
                        primitiveType = typeStr;
                    }
                }
            }
        }
        
        return { 
            identifier, 
            valueType, 
            isPointer, 
            isPrimitive, 
            primitiveType 
        };
    }

    /**
     * Code generation methods
     */
    private insertLoggingCode(
        childNode: SyntaxNode, 
        index: number, 
        totalStatements: number, 
        methodInfo?: MethodInfo,
        isTopLevelFunction: boolean = false
    ): void {
        const lineNumber = childNode.startPosition.row;
        const endLineNumber = childNode.endPosition.row;
        const columnNumber = childNode.startPosition.column;

        let lineData = "";
        let lineDataAfterExec = "";

        // Method entry code (only for first statement in top-level functions)
        if (isTopLevelFunction && index === 0 && methodInfo) {
            lineData += this.generateMethodEntryCode(methodInfo.name, methodInfo.shouldResetCodeRun);
            
            // Add parameter logging if params exist
            if (methodInfo.params) {
                lineData += this.generateParameterLoggingCode(methodInfo.params);
            }
        }

        // For all statements - add line run logging
        lineData += this.generateLineRunCode(lineNumber);

        // Handle variable tracking (for methods with methodInfo and non-for statements)
        if (methodInfo && childNode.type !== "for_statement") {
            const assignmentInfo = this.extractAssignmentInfo(childNode);
            if (assignmentInfo.identifier) {
                lineDataAfterExec += this.generateVariableUpdateCode(
                    assignmentInfo.identifier, 
                    assignmentInfo.isPointer
                );
            }
        }

        // Insert code into the source
        this.modifiedSourceCode[lineNumber] = this.insertAtColumnPosition(
            this.modifiedSourceCode[lineNumber],
            columnNumber,
            lineData
        );
        
        if (lineDataAfterExec) {
            this.modifiedSourceCode[endLineNumber] += lineDataAfterExec;
        }
    }

    private generateMethodEntryCode(methodName: string, shouldResetCodeRun: boolean): string {
        let code = `blink::XTrace *xtrace = blink::XTrace::getInstance(); `;
        
        if (shouldResetCodeRun) {
            code += `xtrace->ResetCodeRunId("${methodName}"); `;
        }
        
        code += `std::string xtrace_mrid = xtrace->OnMethodEnter("${fileName}", "${methodName}", "${cvid}" );\n `;
        return code;
    }

    private generateParameterLoggingCode(params: SyntaxNode): string {
        let code = "";
        
        params.namedChildren.forEach((param: any) => {
            // Extract parameter identifier
            let identifierStr = "";
            if(param.declaratorNode?.type === "identifier") {
                identifierStr = param.declaratorNode?.text;
            } else {
                const identifierNode = param.declaratorNode?.namedChildren?.find((x) => 
                    x.type.includes("identifier")
                );
                identifierStr = identifierNode ? identifierNode.text : "";
            }
            
            const isPointerType = param.declaratorNode?.type === "pointer_declarator";
            
            if (identifierStr) {
                code += this.generateVariableUpdateCode(identifierStr, isPointerType);
            }
        });
        
        return code;
    }

    private generateVariableUpdateCode(variableName: string, isPointer: boolean): string {
        if (isPointer) {
            return `xtrace->LocalVarUpdate(xtrace_mrid,"${variableName}", ${variableName} ? base::ToString(*${variableName}) : "");\n`;
        } else {
            return `xtrace->LocalVarUpdate(xtrace_mrid,"${variableName}", base::ToString(${variableName}));\n`;
        }
    }

    private generateLineRunCode(lineNumber: number): string {
        return `xtrace->LogLineRun(xtrace_mrid, ${lineNumber}); `;
    }

    private insertAtColumnPosition(line: string, column: number, text: string): string {
        return line.slice(0, column) + text + line.slice(column).trim();
    }

    /**
     * Syntax node handling
     */
    private handleSyntaxNode(node: any): void {
        let statements: SyntaxNode[] = [];
        
        switch (node.type) {
            case "if_statement": 
                this.handleIfStatement(node);
                break;
                
            case "switch_statement":
            case "for_statement":
            case "while_statement":
            case "for_range_loop": 
                this.handleLoopOrSwitchStatement(node);
                break;
                
            case "lambda_expression": 
                this.handleFunctionDefinition(node, true);
                return;
                
            case "else_clause": 
                this.handleElseClause(node);
                break;
                
            case "declaration": 
                // Special handling for declarations if needed
                break;
                
            default: 
                this.handleGenericNode(node);
                break;
        }
    }

    private handleIfStatement(node: any): void {
        const statements = node.consequenceNode.namedChildren;
        this.processNodes(statements);
        
        if (node.alternativeNode) {
            this.processNodes([node.alternativeNode]);
        }
    }

    private handleLoopOrSwitchStatement(node: any): void {
        if (node.bodyNode && node.bodyNode.namedChildren) {
            this.processNodes(node.bodyNode.namedChildren);
        }
    }

    private handleElseClause(node: any): void {
        if (!node.namedChildren || node.namedChildren.length === 0) return;
        
        if (node.namedChildren[0].type.includes("compound")) {
            this.processNodes(node.namedChildren[0].namedChildren);
        } else if (node.namedChildren[0].type.includes("if")) {
            this.handleIfStatement(node.namedChildren[0]);
        } else {
            this.processNodes(node.namedChildren);
        }
    }

    private handleGenericNode(node: any): void {
        if (node.namedChildren) {
            this.processNodes(node.namedChildren);
        }
    }
}