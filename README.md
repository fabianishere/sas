sas
===
Simple audio streaming server and client written in C, using backpressured 
pipelines, as part of course on Systems Programming.

## Design
The application is composed of four main components:

1. [rxc](/rxc)  
   A simple C library for defining streaming, back-pressured pipelines, similar
   to for example [RxJava](https://github.com/ReactiveX/RxJava).
2. [sas-core](/sas-core)  
   A library containing procedures and data structures that are shared by the
   client and server.
3. [sas-server](/sas-server)  
   A streaming audio server implementation based on rxc.
4. [sas-client](/sas-client)  
   A streaming audio client implementation based on rxc.

### Streaming Protocol
As part of the assignment, we also had to design and implement our own reliable
transport protocol on top of UDP for streaming the audio packets. Our protocol 
is similar to TCP and documented in [PROTOCOL.txt](PROTOCOL.txt)

## Getting Started
### Obtaining the source code
Download the source code by running the following code in your command prompt:
```shell
git clone https://github.com/fabianishere/sas.git
```
or simply 
[grab](https://github.com/fabianishere/sas/archive/master.zip) 
a copy of the source code as a Zip file.

### Building the project
Create the build directory.
```shell
$ mkdir build
$ cd build
```
Brainfuck requires CMake and a C compiler (e.g. Clang or GCC) in order to run. 
Then, simply create the Makefiles:
```shell
$ cmake ..
```
and finally, build it using the building system you chose (e.g. Make):
```sh
$ make
```
### Usage
In the build directory, start the server as follows:
```shell
./sas-server/sas-server
```
Next, start the client as follows:
```shell
./sas-client/sas-client <HOST> <PATH-TO-WAV>
```
where `HOST` refers to the address hosting the server (e.g. localhost) and `PATH-TO-WAV`
refers to the (relative) path from the servers' current working directory
to some WAV file.



## License
sas is available under the [MIT license](LICENSE.txt).