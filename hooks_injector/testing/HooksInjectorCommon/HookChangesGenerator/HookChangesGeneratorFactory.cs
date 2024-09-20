using System;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.CoreComponents;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;
using System.Collections.Generic;

namespace HooksInjectorCommon.HookChangesGenerator
{
    public class HookChangesGeneratorFactory
    {
        public HookChangesGeneratorFactory()
        {
        }

		public BaseHooksGenerator GetHooksGenerator(SyntaxNodeKind kind, IHookTemplates hookTemplates)
		{
			if (IsStatement(kind))
				return new StatementHooksGenerator(hookTemplates);

			if (kind == SyntaxNodeKind.RootKind)
				return new RootHooksGenerator(hookTemplates);

			if (kind == SyntaxNodeKind.Module)
				return new ModuleHooksGenerator(hookTemplates);

			if (IsCodeRunner(kind))
				return new FunctionLikeHooksGenerator(hookTemplates);

			return new BaseHooksGenerator(hookTemplates);
		}

		List<SyntaxNodeKind> statementKinds = new List<SyntaxNodeKind>() { 
			SyntaxNodeKind.Statement,
			SyntaxNodeKind.BlockedStatement,
			SyntaxNodeKind.DirectChildStatement,
		};

        List<SyntaxNodeKind> codeRunnerKinds = new List<SyntaxNodeKind>() {
			SyntaxNodeKind.MethodDeclaration,
			SyntaxNodeKind.FunctionDeclaration,
			SyntaxNodeKind.Constructor,
        };

		protected bool IsStatement(SyntaxNodeKind kind)
        {
			return statementKinds.Contains(kind);
        }

        protected bool IsCodeRunner(SyntaxNodeKind kind)
        {
			return codeRunnerKinds.Contains(kind);
        }
    }
}
