import { SyntaxNode, Tree } from 'tree-sitter';
import { config, fileName, cvid, methodsToInclude, methodsToExclude, primitiveTypes, typesToExclude, shouldSkipVariablesHooking } from './config';

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
    isReference: boolean;  // Added reference tracking
    isPrimitive: boolean;
    primitiveType: string;
}

/**
 * Extracted to a separate interface to reuse when analyzing identifiers
 */
interface IdentifierInfo {
    name: string;
    isPointer: boolean;
    isReference: boolean;  // Added reference tracking
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
                this.handleSyntaxNode(childNode, methodInfo);
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
               !type.includes("attribute") && 
               !type.includes("compound_statement") && 
               (type.includes("statement") || 
                type.includes("declaration") || 
                type.includes("definition") || 
                type.includes("for_range_loop"));
    }

    /**
     * Common utility methods for node analysis
     */
    private extractIdentifierInfo(node: SyntaxNode, isParameter: boolean = false): IdentifierInfo | null {
        // For parameters, we need special handling
        if (isParameter) {
            // Check for pointer or reference type in parameter
            const isPointer = this.isPointerParameter(node);
            const isReference = this.isReferenceParameter(node);
            
            // Extract name from parameter node structure
            let name = "";
            if (node.declaratorNode) {
                name = this.extractParameterName(node.declaratorNode);
            }
            
            if (name) {
                return { name, isPointer, isReference };
            }
        }
        
        // Standard identifier extraction for non-parameters
        // Check if node is identifier itself
        if (node.type === "identifier") {
            return {
                name: node.text,
                isPointer: false,
                isReference: false
            };
        }
        
        // Look for pointer declarator first
        const pointerTypeNode = node.namedChildren.find(x => x.type.includes("pointer_declarator"));
        if (pointerTypeNode) {
            const identifierNode = pointerTypeNode.namedChildren.find(x => x.type.includes("identifier"));
            if (identifierNode) {
                return {
                    name: identifierNode.text,
                    isPointer: true,
                    isReference: false
                };
            }
        }
        
        // Look for reference declarator
        const referenceTypeNode = node.namedChildren.find(x => x.type.includes("reference_declarator"));
        if (referenceTypeNode) {
            const identifierNode = referenceTypeNode.namedChildren.find(x => x.type.includes("identifier"));
            if (identifierNode) {
                return {
                    name: identifierNode.text,
                    isPointer: false,
                    isReference: true
                };
            }
        }
        
        // Look for direct identifier
        const identifierNode = node.namedChildren.find(x => x.type.includes("identifier"));
        if (identifierNode) {
            return {
                name: identifierNode.text,
                isPointer: false,
                isReference: false
            };
        }
        
        return null;
    }

    /**
     * Check if a parameter is a pointer type
     */
    private isPointerParameter(param: any): boolean {
        // Check if the parameter has a pointer_declarator
        if (param.declaratorNode && param.declaratorNode.type === "pointer_declarator") {
            return true;
        }
        
        // Check nested declarator for pointer type
        if (param.declaratorNode && param.declaratorNode.namedChildren) {
            return param.declaratorNode.namedChildren.some(
                (child: SyntaxNode) => child.type === "pointer_declarator"
            );
        }
        
        // Check if parameter has pointer in the type
        if (param.typeNode && param.typeNode.text.includes("*")) {
            return true;
        }
        
        // Check for abstract declarator with pointer
        const abstractDeclarator = param.namedChildren?.find(
            (x: SyntaxNode) => x.type === "abstract_pointer_declarator"
        );
        if (abstractDeclarator) {
            return true;
        }
        
        return false;
    }

    /**
     * Check if a parameter is a reference type
     */
    private isReferenceParameter(param: any): boolean {
        // Check if the parameter has a reference_declarator
        if (param.declaratorNode && param.declaratorNode.type === "reference_declarator") {
            return true;
        }
        
        // Check nested declarator for reference type
        if (param.declaratorNode && param.declaratorNode.namedChildren) {
            return param.declaratorNode.namedChildren.some(
                (child: SyntaxNode) => child.type === "reference_declarator"
            );
        }
        
        // Check if parameter has reference in the type
        if (param.typeNode && param.typeNode.text.includes("&")) {
            return true;
        }
        
        return false;
    }

    /**
     * Extract name from parameter declarator node
     */
    private extractParameterName(declaratorNode: SyntaxNode): string {
        // Direct identifier
        if (declaratorNode.type === "identifier") {
            return declaratorNode.text;
        }
        
        // Identifier in pointer declarator
        if (declaratorNode.type === "pointer_declarator") {
            const nestedIdentifier = declaratorNode.namedChildren.find(
                (child: SyntaxNode) => child.type === "identifier"
            );
            if (nestedIdentifier) {
                return nestedIdentifier.text;
            }
        }
        
        // Recursively search for an identifier in children
        for (const child of declaratorNode.namedChildren) {
            const name = this.extractParameterName(child);
            if (name) {
                return name;
            }
        }
        
        return "";
    }

    /**
     * Assignment handling
     */
    private extractAssignmentInfo(childNode: SyntaxNode): AssignmentInfo {
        // Try to identify declaration types
        const isDeclaration = childNode.type.includes("declaration");
        const hasDeclarator = isDeclaration && childNode.namedChildren.some(x => 
            x.type.includes("declarator") || 
            x.type.includes("init_declarator")
        );
        
        // Find assignment expressions in statements and declarations
        const assignmentStatement = childNode.namedChildren.filter((x) => 
            x.type.includes("init_declarator") || 
            x.type.includes("assignment_expression") ||
            x.type.includes("declarator")
        );
        
        let identifier = null;
        let valueType = null;
        let isPointer = false;
        let isReference = false;
        let isPrimitive = false;
        let primitiveType = "";
        
        // Process declarations with initializations
        if ((hasDeclarator || childNode.type === "expression_statement") && assignmentStatement.length > 0) {
            for (const param of assignmentStatement) {
                // Handle regular assignment expression
                if (param.type === "assignment_expression") {
                    const leftNode = param.namedChildren[0];
                    
                    if (leftNode && leftNode.type === "identifier") {
                        identifier = leftNode.text;
                        isPointer = false; // Simple assignments typically aren't pointers
                        isReference = false;
                    }
                    
                    const valueTypeNode = param.namedChildren[1]; // Right side of assignment
                    valueType = valueTypeNode ? valueTypeNode.type : null;
                    
                    // We found what we needed, can break early
                    if (identifier) break;
                } else {
                    // Try to extract the identifier directly for declarations
                    let identifierInfo = this.extractIdentifierInfo(param);
                    
                    // If not found, search deeper in the node structure
                    if (!identifierInfo) {
                        // Find inside declarator node
                        const declaratorNodes = param.namedChildren.filter(x => 
                            x.type.includes("declarator")
                        );
                        
                        for (const declNode of declaratorNodes) {
                            identifierInfo = this.extractIdentifierInfo(declNode);
                            if (identifierInfo) break;
                        }
                    }
                    
                    // Set values if identifier was found
                    if (identifierInfo) {
                        identifier = identifierInfo.name;
                        isPointer = identifierInfo.isPointer;
                        isReference = identifierInfo.isReference;
                    }
                    
                    // Find the value type
                    const valueTypeNode = param.namedChildren.find((x) => 
                        x.type.includes("number_literal") || 
                        x.type.includes("string_literal") || 
                        x.type.includes("identifier") ||
                        x.type.includes("call_expression")
                    );
                    
                    valueType = valueTypeNode ? valueTypeNode.type : null;
                }
            }
            
            // Check if primitive type
            if (isDeclaration) {
                const primitiveTypeNode = childNode.namedChildren.find((x) => 
                    x.type.includes("primitive_type")
                );
                
                if (primitiveTypeNode) {
                    isPrimitive = true;
                    primitiveType = primitiveTypeNode.text;
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
            isReference,
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
            
            // Add parameter logging if params exist and variable hooking is not skipped
            if (methodInfo.params && !methodInfo.name.includes("TEST_") && !shouldSkipVariablesHooking()) {
                lineData += this.generateParameterLoggingCode(methodInfo.params);
            }
        }

        // Add line run logging
        lineData += this.generateLineRunCode(lineNumber);

        // Handle variable tracking (for methods with methodInfo and non-for statements)
        // Skip variable tracking if SkipVariablesHooking is enabled
        if (methodInfo && childNode.type !== "for_statement" && !shouldSkipVariablesHooking()) {
            const assignmentInfo = this.extractAssignmentInfo(childNode);
            if (assignmentInfo.identifier) {
                lineDataAfterExec += this.generateVariableUpdateCode(
                    assignmentInfo.identifier, 
                    assignmentInfo.isPointer,
                    assignmentInfo.isReference,
                    assignmentInfo.valueType
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
            if (lineNumber === endLineNumber) {
                // Statement is on the same line as start, need to account for inserted code
                const adjustedColumn = childNode.endPosition.column + lineData.length;
                this.modifiedSourceCode[endLineNumber] = this.insertAtColumnPosition(
                    this.modifiedSourceCode[endLineNumber],
                    adjustedColumn,
                    " " + lineDataAfterExec
                );
            } else {
                // Statement spans multiple lines, insert at the end column of the last line
                const endColumn = childNode.endPosition.column;
                this.modifiedSourceCode[endLineNumber] = this.insertAtColumnPosition(
                    this.modifiedSourceCode[endLineNumber],
                    endColumn,
                    " " + lineDataAfterExec
                );
            }
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
            // Use the enhanced method with isParameter flag set to true
            const identifierInfo = this.extractIdentifierInfo(param, true);
            
            if (identifierInfo && identifierInfo.name) {
                code += this.generateVariableUpdateCode(
                    identifierInfo.name, 
                    identifierInfo.isPointer,
                    identifierInfo.isReference,
                );
            }
        });
        
        return code;
    }

    private generateVariableUpdateCode(variableName: string, isPointer: boolean, isReference: boolean = false, valueType: string = ""): string {
        for(const excludedType of typesToExclude) {
            if (variableName.includes(excludedType)) {
                return "";
            }
        }
        if (isPointer) {
            return `xtrace->LocalVarUpdate(xtrace_mrid,"${variableName}", ${variableName} ? base::ToString(*${variableName}) : "");\n`;
        } else if (isReference) {
            return `xtrace->LocalVarUpdate(xtrace_mrid,"${variableName}", base::ToString(${variableName}));\n`;
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
    private handleSyntaxNode(node: any, methodInfo?: MethodInfo): void {
        let statements: SyntaxNode[] = [];
        
        switch (node.type) {
            case "if_statement": 
                this.handleIfStatement(node, methodInfo);
                break;
                
            case "switch_statement":
            case "for_statement":
            case "while_statement":
            case "for_range_loop": 
                this.handleLoopOrSwitchStatement(node, methodInfo);
                break;
                
            case "lambda_expression": 
                this.handleFunctionDefinition(node, true);
                return;
                
            case "else_clause": 
                this.handleElseClause(node, methodInfo);
                break;
                
            case "declaration": 
                // Special handling for declarations if needed
                break;
                
            default: 
                this.handleGenericNode(node, methodInfo);
                break;
        }
    }

    private handleIfStatement(node: any, methodInfo?: MethodInfo): void {
        const statements = node.consequenceNode.namedChildren;
        this.processNodes(statements, methodInfo);
        
        if (node.alternativeNode) {
            this.processNodes([node.alternativeNode], methodInfo);
        }
    }

    private handleLoopOrSwitchStatement(node: any, methodInfo?: MethodInfo): void {
        if (node.bodyNode && node.bodyNode.namedChildren) {
            this.processNodes(node.bodyNode.namedChildren, methodInfo);
        }
    }

    private handleElseClause(node: any, methodInfo?: MethodInfo): void {
        if (!node.namedChildren || node.namedChildren.length === 0) return;
        
        if (node.namedChildren[0].type.includes("compound")) {
            this.processNodes(node.namedChildren[0].namedChildren, methodInfo);
        } else if (node.namedChildren[0].type.includes("if")) {
            this.handleIfStatement(node.namedChildren[0], methodInfo);
        } else {
            this.processNodes(node.namedChildren, methodInfo);
        }
    }

    private handleGenericNode(node: any, methodInfo?: MethodInfo): void {
        if (node.namedChildren) {
            this.processNodes(node.namedChildren, methodInfo);
        }
    }
}