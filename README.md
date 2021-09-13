# kvdb
Simple key-value storage.

## Building

### Building with docker

The simplest (but not fastest for the first time ) way to build application from sources is to execute commands:

  > cd <path_to_kvdb>/docker
  > docker-compose run build_all

To do this, You only will need installed docker daemon and docker-compose on your machine. You can download latest versions following by this links:

  >  [docker desktop](https://www.docker.com/products/docker-desktop)
  >  [docker compose](https://docs.docker.com/compose/install/)

Code was tested with Docker version 20.10.7 and docker-compose version 1.29.2 on Ubuntu 20.04.3 LTS

### Building from command line

Required packets:
   - g++
   - libboost-system1.71-dev
   - libboost-program-options1.71-dev
   - libboost-log1.71-dev
   - cmake
   
Build from sources:

   > cd <path_to_kvdb>
   > ./build.sh
   
Build script will create directory <path_to_kvdb>/build and store all build artifacts in there

## Using command line tools

### KVDB Server

Executable file of server is *kvdb_server*. It accetpt next arguments:

   - --port=<number> *optional, default value is 1524* specifies port to accept connections on. Server will start listen on address *localhost:<port>*
   - --file=<filename> *optional, default value is ./memfile.map* specifies path to memory mapped file where server will store it's data. One can specify not existing file - server will create new one.
   
Example of command:
  
   > ./kvdb_server --port=5001 --file=./mymemfile.map
  
### KVDB Client
   
Execution command format:

   > ./kvdb_cli <NAMEDARGS> <COMMAND> 
   
named arguments:

   - --hostname=<addr> *required* accepts address or name of remote KVDB server
   - --port=<port> *optional, default value is 1524* port number of KVDB server to connect to 
   
positional argument (command):

   Command can consist of 2 to 3 separate strings:
   - Command name. Possible values are: *INSERT*, *UPDATE*, *GET*, *DELETE*
   - Key string placed in double qutes: "Some key"
   - Value string placed in double quotes: "Some value"
       
   **Note**: Key and value strings MUST NOT contain \0 symbol. Otherwise, it will cause wrong behaviour in string serialization-deserialization and, therefore string data corruption.
       
   INSERT and UPDATE command accepts both Key and Value arguments, GET and UPDATE commands only accepts Key argument
   
#### Examples of usage:
       
   > ./kvdb_cli --hostname=localhost --port=5001 INSERT "Some Key" "Some Value"
   > ./kvdb_cli --hostname=localhost --port=5001 UPDATE "Some Key" "Some Other Value"
   > ./kvdb_cli --hostname=localhost --port=5001 GET "Some Key"
   > ./kvdb_cli --hostname=localhost --port=5001 DELETE "Some Key"
       
### Running KVDB server in docker:

To run KVDB server using docker launch next commands:

   > cd <path_to_kvdb>/docker
   > docker-compose up -d server
   
This command will run kvdb_server in docker environment and detach terminal from this process. Server will listening on localhost:5001

### Test

There are integration test available in file <path_to_kvdb>/test.py
This is simple python script performing next scenario:
   - launching kvdb_server in docker
   - creating number of key:value records in map
   - restarting server
   - checking that all previously created records are still contained by DB
   - deleting all previously created values



