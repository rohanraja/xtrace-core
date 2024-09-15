## Deployment steps:


### Pre-requisites:

#### 1.) Copy SSH private key for server into local client
#### 2.) Copy /data folder
```bash
scp -r apiserver/db/development.sqlite3 rohan-linux@beast:/home/rohan-linux/code/xtrace/ide1/data/api-db/production.sqlite3
```
#### 3.) Modify .env file with server details
#### 4.) Setup a private docker registry and modify details in .env file



### Step 1 - Build docker container for production

```bash
bash scripts/buildDocker.prod.sh
```

### Step 2 - Set environment variables for production

```bash
bash scripts/tranferDockerImages.sh
```