using System;
using System.Collections.Generic;
using HooksInjectorCommon.Entities;

namespace HooksInjectorCommon.LangInterfaces
{
    public interface ITreeChangeInfo
    {
		void AddNodesAfter(ISyntaxNode node, List<ISyntaxStringNode> syntaxStringNodes);
		void AddNodesBefore(ISyntaxNode node, List<ISyntaxStringNode> syntaxStringNodes);
		void ReplaceNode(ISyntaxNode node, ISyntaxStringNode syntaxStringNode);
		void CreateChildNodes(ISyntaxNode node, List<ISyntaxStringNode> syntaxNodes);

		string GetNewSource(ISyntaxNode originalRoot);
    }
}
