# Folders
	- ## common
		- info:: Common libraries and dependencies used by micro services within the repo. Like proto files
		- ### proto
		- ### types
		- ### utilities
		-
	- ## docs
		- 001 - FolderStructure
		-
	- ## extensions
		- info:: Azure dev ops extension, VS Code extension, external plugins
		- vsts

	- ## demo-apps
		- typescript
		- csharp
	- ## scripts
	- ## tests
		- ### data
		- ### service-api-tests
		- ### system-tests
		-
	- ## target
		- info:: compiled binaries and output

	- ## experiments
		- info:: sample code, try out new technologies, reference code

-
- # Conventions
	- folder names should use dash convention


# src
<!-- TODO: Create folder and move parser, webclient, etc -->


## parser
<!-- TODO: Rename to "hooksinjector" -->

### common
<!-- TODO: Create and move common helpers -->
Contains utility code which is globally commona and NOT specific to xTrace

e.g. TmpFilePatTypeScriptSourceFileHookerh generator

### csharp
<!-- TODO: Move existing files to this folder -->

#### tests

##### simple_main_class
- main.cs
- main.expected.cs
- test.csproj

### cpp

#### cpphooksinjector

#### tests

##### simple_main_function
- main.cc
- main.expected.cc

##### simple_class
- main.cc
- main.expected.cc
- classA.cc
- classA.expected.cc
