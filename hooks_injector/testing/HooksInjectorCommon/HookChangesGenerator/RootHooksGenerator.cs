using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.HookChangesGenerator
{
	public class RootHooksGenerator : BaseHooksGenerator
    {
		public RootHooksGenerator(IHookTemplates hookTemplates = null): base(hookTemplates)
		{
		}


		public override void GenerateHookChanges(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
            List<ISyntaxStringNode> syntaxNodes = new List<ISyntaxStringNode>() { };

            string importHook = hookTemplates.ImportLibHook(filePath);

            string onMethodEnterLine = hookTemplates.OnMethodEnterHook(filePath, "RootMethod");

            syntaxNodes.Add(new SyntaxStringNode(importHook));
            syntaxNodes.Add(new SyntaxStringNode(onMethodEnterLine));

            if (node.Children.Count > 0 && !isCSProj)
                treeChangeInfo.AddNodesBefore(node.Children[0], syntaxNodes);

            ContinueWalk(node, treeChangeInfo);
		}

	}
}
