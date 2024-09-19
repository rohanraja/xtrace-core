using System.Collections.Generic;

namespace CodeParserCommon
{
    public interface ISourceCodeInfo
    {
		List<ISourceFile> ListCodeFiles();
		ISourceFile GetProjectFile();
    }
}