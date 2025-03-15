import Parser, { SyntaxNode, Tree } from 'tree-sitter';
import Cpp from 'tree-sitter-cpp';

export class CodeParser {
    private parser: Parser;

    constructor() {
        this.parser = new Parser();
        this.parser.setLanguage(Cpp);
    }

    parse(sourceCode: string): Tree {
        return this.parser.parse(sourceCode.replaceAll("class CORE_EXPORT", "class"), undefined, { bufferSize: sourceCode.length + 10 });
    }
}