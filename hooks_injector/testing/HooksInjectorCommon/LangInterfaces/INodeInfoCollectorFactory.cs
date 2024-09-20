using System;
using HooksInjectorCommon.Entities;

namespace HooksInjectorCommon.LangInterfaces
{
    public interface INodeInfoCollectorFactory
    {
		INodeInfoCollector GetCollectorForKind(SyntaxNodeKind kind);
    }
}
