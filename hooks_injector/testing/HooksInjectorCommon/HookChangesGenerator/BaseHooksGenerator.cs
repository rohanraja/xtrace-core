using System;
using System.Collections.Generic;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.CoreComponents;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.Entities.NodePropertieClasses;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.HookChangesGenerator
{
	public class BaseHooksGenerator
    {
		protected readonly IHookTemplates hookTemplates;
		private HookChangesGeneratorFactory hookGenFactory;
		public static string filePath = "";
		protected bool isCSProj = false;


		public BaseHooksGenerator(IHookTemplates hookTemplates = null)
		{
			this.hookTemplates = hookTemplates;
			this.hookGenFactory = new HookChangesGeneratorFactory();
		}

		public void WalkTree(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
        {
            if (node == null)
                return;

            ProcessSyntaxNode(node, treeChangeInfo);

        }

        protected void ContinueWalk(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
        {
            foreach (var child in node.Children)
            {
                WalkTree(child, treeChangeInfo);
            }
        }

        private void ProcessSyntaxNode(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
        {

			var hooksGen = hookGenFactory.GetHooksGenerator(node.Kind, hookTemplates);
			hooksGen.GenerateHookChanges(node, treeChangeInfo);

        }


		public virtual void GenerateHookChanges(ISyntaxNode node, ITreeChangeInfo treeChangeInfo)
		{
			ContinueWalk(node, treeChangeInfo);
		}

	}
}
