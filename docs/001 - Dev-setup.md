# Setup
## Dependencies
- VSCode
- Node > 18
- Docker (for Windows, Docker on Linux)

## Clone repo
cd ~/src 

git clone git@github.com:rohanraja/xtrace-core.git xtrace

cd ~/src/xtrace

## Run init for your platform
/scripts/init/init.sh (Or init.bat for Windows)


## Build docker dev containers
- Open xTrace folder in VSCode
- Run VSCode command "Run task" (Hit Control + Shift + P and search for run task) 

- Run VSCode task named "A) Build dev containers"

# Start

## Run docker dev containers and setup
- Run VSCode task named "B) Run dev containers"
- Run VSCode task named "2.2) Rails DB migrate" when setting up first time

## Run all servers in dev container
- Run VSCode task named "C) Run all servers in dev container"

## Open webclient
- Run VSCode task named "4.) Open web client"

## Open Vscode with dev environment tools

### Open VSCode for ROR API server
1. Ensure dev containers are running by running VSCode task named "B) Run dev containers"
2. In VSCode ensure docker container extension is installed
3. Go to Docker tab, select the running container for APIserver, right click and run "Attach VSCode"

## Run design doc server
<!-- TODO - Add to vscode tasks -->
docker run -it --rm -p 8080:8080 -v ./structurizer:/usr/local/structurizr structurizr/lite
