using System;

namespace CodeParserCommon
{
    public class SourceFile : ISourceFile
	{
		public string FilePath { get; }
        public string RootDir { get; }

		private DataReader dataReader;

		public SourceFile(string fPath, string rootDir)
        {
			FilePath = fPath;
			RootDir = rootDir;
			dataReader = new DataReader(RootDir);
		}

        public string GetCode()
		{
			return dataReader.GetContentsOfFileAtRoot(FilePath);
		}

        public void UpdateCodeContents(string newContents)
		{
			dataReader.SetContentsOfFileAtRoot(FilePath, newContents);

		}


	}
}
