using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.HookChangesGenerator
{
	public class StatementHooksGenerator : BaseHooksGenerator
    {
		public StatementHooksGenerator(IHookTemplates hookTemplates = null): base(hookTemplates)
		{
		}


		public override void GenerateHookChanges(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
            List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

            var statementProps = node.Properties as StatementProperties;

			string logLineRun = hookTemplates.LogLineRunHook(statementProps.LineNo, statementProps.MethodNameCalled);

            syntaxNodes.Add(new SyntaxStringNode(logLineRun));

            treeChangeInfo.AddNodesBefore(node, syntaxNodes);

			AddVarUpdateStatements(node, statementProps, treeChangeInfo);

			if (node.Kind == SyntaxNodeKind.BlockedStatement)
				ContinueWalk(node, treeChangeInfo);
			else
			{
				if(node.Children.Count > 0 && node.Children[0].Kind == SyntaxNodeKind.MethodDeclaration)
					ContinueWalk(node, treeChangeInfo);
			}
		}

		private void AddVarUpdateStatements(ISyntaxNode node, StatementProperties statementProps, ITreeChangeInfo treeChangeInfo)
		{
			foreach(var localVar in statementProps.localVars)
			{
				string varUpdateHook = hookTemplates.LocalVarUpdateHook(localVar);
				treeChangeInfo.AddNodesAfter(node, new List<ISyntaxStringNode>() { new SyntaxStringNode(varUpdateHook) });
			}
		}
	}
}
