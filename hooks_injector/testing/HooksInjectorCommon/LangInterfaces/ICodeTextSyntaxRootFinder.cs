using System;
using HooksInjectorCommon.Entities;

namespace HooksInjectorCommon.LangInterfaces
{
    public interface ICodeTextSyntaxRootFinder
    {
		ISyntaxNode ParseAndGetRootNodeForCode(string codeText, string fileName);
    }
}
