namespace CodeParserCommon
{
    public interface ICodeRegisterer
    {
		void SendCodeContentsToServer(SourceCodeInfo sourceCode, System.Guid version);
    }
}