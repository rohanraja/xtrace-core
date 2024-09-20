using System;
using HooksInjectorCommon.Entities;
using HooksInjectorCommon.LangInterfaces;

namespace HooksInjectorCommon.CoreInterfaces
{
    public interface IHookChangesGenerator
    {
		void GenerateHookChanges(ISyntaxNode syntaxNode, ITreeChangeInfo treeChangeInfo, string fileName);
	}
}
