using System;
using System.Collections.Generic;

namespace CodeParserCommon
{
    public class SourceCodeInfo
	{
		public string BaseDirPath;
		private DataReader dataReader;
		public List<string> CodeFiles = new List<string>() { };

		public List<SourceFile> SourceFiles = new List<SourceFile>() { };

        public SourceCodeInfo(string baseDirPath)
        {
            this.BaseDirPath = baseDirPath;
			dataReader = new DataReader(baseDirPath);
		}

		public void AddCodeFile(string relativePath)
		{
			CodeFiles.Add(relativePath);

			SourceFiles.Add(new SourceFile(relativePath, BaseDirPath));
		}

		public bool RemoveCodeFile(string relativePath)
        {
			bool replaced = false;
			string item2 = "";
			foreach(var ff in CodeFiles)
			{
				if (ff.Replace("\\", "/").Equals(relativePath))
					item2 = ff;
				
			}
            if(item2 != "")
    			CodeFiles.Remove(item2);
			else
			{
				foreach (var ff in CodeFiles)
                {
					if (ff.Replace("\\", "/").Contains(relativePath))
                        item2 = ff;
                }
				if (item2 != "")
                    CodeFiles.Remove(item2);

			}

			SourceFile item = null;
			foreach(var sf in SourceFiles)
			{
				if (sf.FilePath.Replace("\\", "/").Equals(relativePath))
					item = sf;
			}
			if (item != null)
			{
				replaced = true;
				SourceFiles.Remove(item);
			}
			else
			{
				foreach (var sf in SourceFiles)
                {
					if (sf.FilePath.Replace("\\", "/").Contains(relativePath))
                        item = sf;
                }
				if (item != null)
				{
					replaced = true;
					SourceFiles.Remove(item);
				}
			}

			return replaced;
        }

		public void SetContentsOfFileAtRoot(string projectFileName, string replacedStr)
		{
			dataReader.SetContentsOfFileAtRoot(projectFileName, replacedStr);
		}

		public string GetContentsOfFileAtRoot(string projectFileName)
        {
			return dataReader.GetContentsOfFileAtRoot(projectFileName);
        }

		public List<string> ListCodeFiles()
		{
			throw new NotImplementedException();
		}

		public string GetProjectFile()
		{
			throw new NotImplementedException();
		}
	}
}
