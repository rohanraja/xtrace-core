using System;

namespace CodeParserCommon
{
    public interface ISourceFileHooker
    {
        void AddHooksToSourceCode(SourceCodeInfo sourceCodeInfo, Guid codeVersion);
    }
}