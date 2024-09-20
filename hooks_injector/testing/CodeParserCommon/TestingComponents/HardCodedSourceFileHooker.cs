using System;
namespace CodeParserCommon.TestingComponents
{
	public class HardCodedSourceFileHooker : ISourceFileHooker
    {
        public HardCodedSourceFileHooker()
        {
        }

        public void AddHooksToSourceCode(SourceCodeInfo sourceCodeInfo, Guid version)
		{
			foreach (var sourceFile in sourceCodeInfo.SourceFiles)
            {
                string fileName = sourceFile.FilePath;
				string hookedfileName = string.Format("{0}.hooked", fileName);
				string outText = sourceCodeInfo.GetContentsOfFileAtRoot(hookedfileName);
                sourceFile.UpdateCodeContents(outText);
            }
		}
	}
}
