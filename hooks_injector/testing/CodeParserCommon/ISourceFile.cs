namespace CodeParserCommon
{
    public interface ISourceFile
    {
        string FilePath { get; }
        string GetCode();
        void UpdateCodeContents(string newContents);
    }
}