using System;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.LangInterfaces;
using HooksInjectorCommon.CoreInterfaces;
using HooksInjectorCommon.HookChangesGenerator;

namespace HooksInjectorCommon.CoreComponents
{
    public class HookInjectionPipeline
    {
		private readonly ICodeTextSyntaxRootFinder codeTextSyntaxRootFinder;
		private readonly ITreeBuilder treeBuilder;
		private readonly ITreeChangeInfoFactory treeChangeInfoFactory;
		private readonly IHookTemplates hookTemplates;
		private readonly IHookChangesGenerator hookChangesGenerator;

		public HookInjectionPipeline(
			ICodeTextSyntaxRootFinder codeTextSyntaxRootFinder,
			ITreeBuilder treeBuilder,
			ITreeChangeInfoFactory treeChangeInfoFactory,
			IHookTemplates hookTemplates
		)
		{
			this.codeTextSyntaxRootFinder = codeTextSyntaxRootFinder;
			this.treeBuilder = treeBuilder;
			this.treeChangeInfoFactory = treeChangeInfoFactory;
			this.hookTemplates = hookTemplates;
			this.hookChangesGenerator = new TreeBasedHookChangesGenerator(hookTemplates);
		}

		public string AddHooksToSourceFile(string fileName, string fileContents)
        {
			var treeRoot = codeTextSyntaxRootFinder.ParseAndGetRootNodeForCode(fileContents, fileName);
			treeBuilder.BuildTree(treeRoot);

			ITreeChangeInfo treeChangesInfo = treeChangeInfoFactory.CreateNewTreeChangeInfo();

			hookChangesGenerator.GenerateHookChanges(treeRoot, treeChangesInfo, fileName);

			return treeChangesInfo.GetNewSource(treeRoot);
        }
    }
}
