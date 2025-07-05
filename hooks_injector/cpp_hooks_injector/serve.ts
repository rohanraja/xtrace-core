#!/usr/bin/env ts-node

import express from 'express';
import fs from 'fs';
import { execSync } from 'child_process';
import path from 'path';
import { CodeParser } from './parser';
import { CodeLogger } from './logger';
import { CodeFormatter } from './formatter';
import Parser, { SyntaxNode, Tree } from 'tree-sitter';
import {setFileAndVersion} from './config';

require('dotenv').config({ path: "../../config/.env", override: true });

const app = express();
const PORT = process.env.PORT || 3001;

// Middleware to parse JSON bodies
app.use(express.json({ limit: '10mb' }));
app.use(express.text({ limit: '10mb', type: 'text/plain' }));

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ 
        status: 'ok', 
        timestamp: new Date().toISOString(),
        service: 'cpp-hooks-injector'
    });
});

// POST endpoint to inject hooks into C++ code
app.post('/inject', async (req, res) => {
    try {
        let sourceCode = '';
        let filename = '';
        let codeVersion = '';

        // Handle different request types
        if (typeof req.body === 'string') {
            // Plain text body
            sourceCode = req.body;
            filename = req.query.filename as string || 'input.cc';
            codeVersion = req.query.codeVersion as string || '';
        } else if (req.body && typeof req.body === 'object') {
            // JSON body
            if (req.body.code) {
                sourceCode = req.body.code;
                filename = req.body.filename || 'input.cc';
                codeVersion = req.body.codeVersion || '';
            } else if (req.body.sourceCode) {
                sourceCode = req.body.sourceCode;
                filename = req.body.filename || 'input.cc';
                codeVersion = req.body.codeVersion || '';
            } else {
                return res.status(400).json({
                    error: 'Invalid request body. Expected "code" or "sourceCode" field in JSON, or plain text body.'
                });
            }
        } else {
            return res.status(400).json({
                error: 'Request body is required'
            });
        }

        if (!sourceCode.trim()) {
            return res.status(400).json({
                error: 'Source code cannot be empty'
            });
        }

        // Determine which injector to use
        const useLegacyInjector = !(process.env.USE_CLANG_INJECTOR === 'true');
        
        let modifiedSourceCode = "";
        let hasError = false;
        let injectorUsed = "";

        if (useLegacyInjector) {
            // Legacy tree-sitter approach
            injectorUsed = "tree-sitter";
            console.log("Using legacy tree-sitter injector for HTTP request");
            
            process.env["FileName"] = filename;
            process.env["CodeVersion"] = codeVersion;
            setFileAndVersion(filename, codeVersion);


            console.log(`CodeVersion: ${codeVersion}, Filename: ${filename}`);

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
            }
        } else {
            // New Clang LibTooling approach
            injectorUsed = "clang-libtooling";
            console.log("Using new Clang LibTooling injector for HTTP request");
            
            try {
                // Get the path to the Clang injector executable
                const clangInjectorPath = path.join(__dirname, '../../clang_injector/build/xtrace-clang-injector');
                
                // Check if the Clang injector exists
                if (!fs.existsSync(clangInjectorPath)) {
                    throw new Error(`Clang injector not found at ${clangInjectorPath}`);
                }
                
                // Create a temporary file for the source code
                const os = require('os');
                const tempFile = path.join(os.tmpdir(), `xtrace_temp_${Date.now()}_${Math.random().toString(36).substr(2, 9)}.cc`);
                fs.writeFileSync(tempFile, sourceCode);
                
                try {
                    // Prepare command arguments
                    const args = [tempFile, '--', '-std=c++17'];
                    const command = `${clangInjectorPath} ${args.join(' ')}`;
                    
                    // Execute the Clang injector
                    modifiedSourceCode = execSync(command, { 
                        encoding: 'utf-8',
                        stdio: ['pipe', 'pipe', 'inherit']
                    });
                    
                    hasError = false;
                } finally {
                    // Clean up temporary file
                    if (fs.existsSync(tempFile)) {
                        fs.unlinkSync(tempFile);
                    }
                }
                
            } catch (error: any) {
                console.error("Clang injector failed:", error.message);
                console.error("Falling back to legacy tree-sitter injector");
                
                // Fallback to legacy approach
                injectorUsed = "tree-sitter-fallback";
                const parser = new CodeParser();
                const tree = parser.parse(sourceCode);

                const logger = new CodeLogger(sourceCode);
                modifiedSourceCode = logger.addLogLines(tree);

                const formattedSourceCode = CodeFormatter.format(modifiedSourceCode);
                modifiedSourceCode = formattedSourceCode;

                // Check for syntax errors after formatting
                const formattedTree = parser.parse(formattedSourceCode);
                hasError = formattedTree.rootNode.hasError;
            }
        }

        // Return the response
        res.json({
            success: true,
            injectorUsed: injectorUsed,
            hasError: hasError,
            originalCode: sourceCode,
            modifiedCode: modifiedSourceCode,
            filename: filename,
            codeVersion: codeVersion,
            timestamp: new Date().toISOString(),
            statistics: {
                originalLines: sourceCode.split('\n').length,
                modifiedLines: modifiedSourceCode.split('\n').length,
                originalSize: sourceCode.length,
                modifiedSize: modifiedSourceCode.length
            }
        });

    } catch (error: any) {
        console.error('Error processing injection request:', error);
        res.status(500).json({
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        });
    }
});

// Start the server
app.listen(PORT, () => {
    console.log(`C++ Hooks Injector HTTP Server running on port ${PORT}`);
    console.log(`Health check: http://localhost:${PORT}/health`);
    console.log(`Injection endpoint: POST http://localhost:${PORT}/inject`);
    console.log(`Using ${process.env.USE_CLANG_INJECTOR === 'true' ? 'Clang LibTooling' : 'Tree-sitter'} injector`);
});

export default app;
