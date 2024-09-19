using System;
namespace HooksInjectorCommon.Entities
{
	public enum SyntaxNodeKind
    {
        ClassDeclaration = 0,
		MethodDeclaration = 1,
        RootKind = 2,
		FunctionDeclaration = 3,
		Statement = 4,
	       BlockedStatement = 5,
        Block = 6,
        Constructor = 7,
        DirectChildStatement = 8,
        ModuleBlock = 9,
		Module = 10,
		SuperCallStatement = 11,
	}

}
