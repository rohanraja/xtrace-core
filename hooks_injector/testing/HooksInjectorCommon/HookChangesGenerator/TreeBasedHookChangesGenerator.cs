using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.CoreComponents;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.HookChangesGenerator
{
	public class TreeBasedHookChangesGenerator : IHookChangesGenerator
    {
		protected readonly IHookTemplates hookTemplates;
		protected string filePath = "";
		protected bool isCSProj = false;

		protected string currentMethodName = "";

		public TreeBasedHookChangesGenerator(IHookTemplates hookTemplates = null)
		{
			this.hookTemplates = hookTemplates;
		}

		public void GenerateHookChanges(ISyntaxNode syntaxNode, ITreeChangeInfo treeChangeInfo, string filename)
		{
			filePath = filename;

			BaseHooksGenerator.filePath = filename;

			BaseHooksGenerator baseHooksGenerator = new BaseHooksGenerator(hookTemplates);
			baseHooksGenerator.WalkTree(syntaxNode, treeChangeInfo);
		}


	}
}
