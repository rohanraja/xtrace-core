
```mermaid
sequenceDiagram
    participant user as User Web Client
    participant api as Web API Service (rails)
    participant db as Storage
    participant dotnet as Dotnet Exec Service (.net)
    participant injector as xTrace Injector Service (.net)
    participant stage as Code stage folder
    participant dotnetcli as Dotnet CLI (bash)
    Note over user,dotnetcli: Create a temp .net project for the source code in the shared staging folder
    user ->>+ api: RunDSACode (codeId)
    api ->> db: GetCode(codeId)
    db -->> api: [SourceCode]
    api ->>+ dotnet: CreateTmpProjectFolder(sourceCode)
    dotnet ->> stage: CreateTmpFolder
    stage ->> dotnet: TmpFolderPath
    dotnet ->> dotnetcli: console new TmpFolderPath
    dotnet ->> stage: CopySourceCodeToProject 
    dotnet ->>- api: [TmpProjectFolderPath] 

    Note over user,dotnetcli: Inject xTrace hooks into the temp project
    api ->> injector: InjectHooks(TmpProjectFolderPath) 
    Note over user,dotnetcli: Execute injected DSA code
    api ->> dotnet: ExecuteDotnetProject(TmpProjectFolderPath)
    dotnet ->> dotnetcli: dotnet run
    dotnet ->> api: 
    Note over user,dotnetcli: Refresh xTrace cache
    api ->> api: RefreshCache
    api ->>- user: DSA Execution complete

