using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.HookChangesGenerator
{
	public class ModuleHooksGenerator : BaseHooksGenerator
    {
		public ModuleHooksGenerator(IHookTemplates hookTemplates = null): base(hookTemplates)
		{
		}

		public override void GenerateHookChanges(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			var functionDeclarationProperties = node.Properties as ModuleDeclarationProperties;
			string currentMethodName = "";

			if(functionDeclarationProperties != null)
    			currentMethodName = functionDeclarationProperties.Name;

			List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

			string onMethodEnterLine = hookTemplates.OnMethodEnterHook(filePath, currentMethodName);
            syntaxNodes.Add(new SyntaxStringNode(onMethodEnterLine));

			if (node.Children.Count == 0)
				return;
			
			if (node.Children[0].Children.Count > 0)
			{
				var firstChild = node.Children[0].Children[0];
				treeChangeInfo.AddNodesBefore(firstChild, syntaxNodes);

			}
			else
				treeChangeInfo.CreateChildNodes(node.Children[0], syntaxNodes);

			ContinueWalk(node, treeChangeInfo);
		}

	}
}
