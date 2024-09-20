namespace CodeParserCommon
{
    public interface IProjectParser
    {
        SourceCodeInfo GetSourceCodeInfo(string rootDir, string proName);
    }
}