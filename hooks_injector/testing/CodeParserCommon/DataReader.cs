using System;
using System.Collections.Generic;
using System.IO;

namespace CodeParserCommon
{
    public class DataReader
    {

		public string ProjectPath;

        public string GetMainFile()
		{
			return GetContentsOfFileAtRoot("Program.cs");
		}

        public string GetContentsOfFileAtRoot(string fname)
		{
			string fPath = GetFullPath(fname);
			return File.ReadAllText(fPath);

		}

		public string GetFullPath(string fname)
		{
			string fpath = Path.Combine(ProjectPath, fname.Replace("\\", "/" ));
			return fpath;

		}

		public DataReader(string projectPath)
		{
			ProjectPath = projectPath;
		}

        public void SetContentsOfFileAtRoot(string fPath, string newContents)
        {
			var fullPath = GetFullPath(fPath);
			File.WriteAllText(fullPath, newContents);
        }

        public string GetCSProjContents()
		{
			string fname = GetCsProjFname();
			return GetContentsOfFileAtRoot(fname);
		}

		public virtual string GetCsProjFname()
		{
			throw new NotImplementedException() ;
		}
    }
}
