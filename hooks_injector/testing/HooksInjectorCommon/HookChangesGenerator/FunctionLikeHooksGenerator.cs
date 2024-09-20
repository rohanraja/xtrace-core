using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.HookChangesGenerator
{
	public class FunctionLikeHooksGenerator : BaseHooksGenerator
    {
		public FunctionLikeHooksGenerator(IHookTemplates hookTemplates = null): base(hookTemplates)
		{
		}

		public override void GenerateHookChanges(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			FunctionLikeNodeProperties functionDeclarationProperties = node.Properties as FunctionLikeNodeProperties;
			string currentMethodName = "";

			if(functionDeclarationProperties != null)
    			currentMethodName = functionDeclarationProperties.Name;

			List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

			string onMethodEnterLine = hookTemplates.OnMethodEnterHook(filePath, currentMethodName);
            syntaxNodes.Add(new SyntaxStringNode(onMethodEnterLine));

			if (node.Children.Count == 0)
				return;
			
			if (functionDeclarationProperties.Statements.Count > 0)
			{
				var firstChild = functionDeclarationProperties.Statements[0] ;
				if(firstChild.Kind == SyntaxNodeKind.SuperCallStatement)
				{
					firstChild = null;
					if (functionDeclarationProperties.Statements.Count > 1)
						firstChild = functionDeclarationProperties.Statements[1];
				}
				if(firstChild != null)
					treeChangeInfo.AddNodesBefore(firstChild, syntaxNodes);

			}
			else
				treeChangeInfo.CreateChildNodes(node.Children[0], syntaxNodes);

			ContinueWalk(node, treeChangeInfo);
		}

	}
}
