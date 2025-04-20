To run the following demo,

use the following directory structure

```
- LmicMesher/
  - lib/
    - LoRaMesher/
    - MCCI_LoRaWAN_LMIC_library/
    - RadioLib/
  - src/
    - main.cpp
  - platformio.ini
```

To make the relevant directories in `lib/`, you need to download them 

e.g.

```sh
mkdir lib

git clone https://github.com/LoRaMesher/LoRaMesher lib/LoRaMesher
git clone https://github.com/mcci-catena/arduino-lmic lib/MCCI_LoRaWAN_LMIC_library
git clone https://github.com/jgromes/RadioLib lib/RadioLib

mkdir src
touch src/main.cpp # this is the demo code

touch platformio.ini # this is the part responsible for building and flashing the devices
```


`src/main.cpp` contains code based on the [counter example](https://github.com/LoRaMesher/LoRaMesher/tree/ae18690fd9feb3165f38a02fb9e0d80ca41558b5/examples/Counter) and [lorawan example from LMIC](https://github.com/mcci-catena/arduino-lmic/blob/01a880fa84b72961ba7e73607c573185386020b2/examples/ttn-otaa-sx1262/ttn-otaa-sx1262.ino)

You can open this directory as a platformio project via the vscode extension or simply use [`platformio cli (pio)`](https://docs.platformio.org/en/latest/core/index.html) to build and upload code.

To build the project, you can simply run 
```sh
pio run
```

To build and upload the project, you can run 
```sh
pio run -t upload
```

You can run `pio run -t compiledb` to produce `compile_commands.json` which can by used by Language Servers to find definitions, references etc.

## Modifications in the downloaded libraries

- In `./lib/MCCI_LoRaWAN_LMIC_library/` you need to configure `./lib/MCCI_LoRaWAN_LMIC_library/project_config/lmic_project_config.h` to configure your radio and region spec

- In `./lib/LoRaMesher/src/BuildOptions.h` you need to uncomment `// #define LM_TESTING` (.i.e. define `LM_TESTING` variable)
- In `./lib/LoRaMesher/src/LoraMesher.cpp` you need to change the following to define custom topology

```cpp
#ifdef LM_TESTING

const int NUM_NODES = 4;

const uint16_t label_addresses[NUM_NODES] = {
    0xAE14,
    0xB5B8,
    0x121C,
    0x1850,
};

const bool adjacency_matrix[NUM_NODES][NUM_NODES] = {
    {0,1,0,0},
    {1,0,1,0},
    {0,1,0,1},
    {0,0,1,0},
};

bool LoraMesher::canReceivePacket(uint16_t source) {
    int idx_self = -1 ;
    int idx_other = -1 ;

    uint16_t this_addr = getLocalAddress();
    for(int i = 0; i < NUM_NODES; i++){
        if(label_addresses[i] == this_addr){ idx_self = i; }
        if(label_addresses[i] == source){ idx_other = i; }
    }
    if (idx_self == -1 || idx_other == -1){return false;} 

    return adjacency_matrix[idx_self][idx_other];

}
#endif

```

## How To read the output logs

If compiled with `CORE_DEBUG_LEVEL=5` defined, the node will produce verbose
logs on the serial which can be used to inspect and monitor the state of the
nodes.
This can be changed in the [platformio](./platformio.ini)  file by using

```ini
build_flags = 
	-D CORE_DEBUG_LEVEL=5
```

<!-- For example, consider the following situation: -->
<!---->
<!-- We have three devices with addresses: `DE6C`,`F02C`,`D110` -->
<!---->
<!-- and they are connected as follows -->
<!---->
<!-- `DE6C <-> F02C <-> D110` -->
<!---->
<!-- and  `DE6C` has the gateway role. -->
<!---->
<!-- A sample representative output log is shown (simply reading the contents of respective ports) -->
<!---->
<!-- - `DE6C` -->
<!---->
<!---->
<!-- `` -->

### Routing Table info

For example, the logs may contain lines like this


#### DE6C

```
[168794][I][RoutingTableService.cpp:192] printRoutingTable(): [LoRaMesher] Current routing table:
[168803][I][RoutingTableService.cpp:206] printRoutingTable(): [LoRaMesher] 0 - F02C via F02C metric 1 Role 0
[168812][I][RoutingTableService.cpp:206] printRoutingTable(): [LoRaMesher] 1 - D110 via F02C metric 2 Role 0
```

- Here the numbers in the `[ ]` represent the number of `millis()` since the starting of the program.
- The next section can be `[I]`, `[V]` or `[E]` to represent the LOG_LEVEL of the message.
- It is followed the filename and linenumber triggering the log
- Then the name fo the calling function is printed
- In this case, the function is `printRoutingTable` and it prints the node's current routing table.
    - For example, in theis case, the routing table is for the node `DE6C` (not mentioned in this part of the logs)
    - It shows that  `DE6C` can reach node `F02C` in `1` hop (i.e. it can reach it directly) and the node `F02C` has role `0` (normal node, not a gateway)
    - It shows that  `DE6C` can reach node `D110` in `2` hop (i.e. it can reach it via a node that is currently connected directly) and the node has role `0` (normal node, not a gateway)

Here are the similar logs from `F02C` and `D110` respectively

#### F02C

```

[ 88966][I][RoutingTableService.cpp:192] printRoutingTable(): [LoRaMesher] Current routing table:
[ 88975][I][RoutingTableService.cpp:206] printRoutingTable(): [LoRaMesher] 0 - D110 via D110 metric 1 Role 0
[ 88984][I][RoutingTableService.cpp:206] printRoutingTable(): [LoRaMesher] 1 - DE6C via DE6C metric 1 Role 1
```
#### D110

```

[ 95482][I][RoutingTableService.cpp:192] printRoutingTable(): [LoRaMesher] Current routing table:
[ 95491][I][RoutingTableService.cpp:206] printRoutingTable(): [LoRaMesher] 0 - F02C via F02C metric 1 Role 0
[ 95501][I][RoutingTableService.cpp:206] printRoutingTable(): [LoRaMesher] 1 - DE6C via F02C metric 2 Role 1
```


These logs show that `DE6C` has a gateway role (`Role 1`).


### Packet Info

The logs may contain the mentions of the `printHeaderPacket` function which logs the information found in the `AppPacket`

For example this is the log from `F02C` device from the previously described topology. 

```

[1528011][V][LoraMesher.cpp:754] printHeaderPacket(): [LoRaMesher] Packet send -- Size: 17 Src: D110 Dst: DE6C Id: 87 Type: 2 Via: DE6C Seq_Id: 0 Num: 0

```

- It says that the node `F02C` has sent a packet
- `Size`: The packet has size `17` bytes
- `Src`: the packet's original source was `D110` (hence it is a forwaded packet).
- `Dst`: the packet's destination is `DE6C` (the gateway node in this case).
- `Id` field represents the packet index sent out by the source node (hence `(Src, Id)` pair uniquely identifies a packet in the network and can be used to trace its journey) 
- `Type`: The type field represents whether the packet is a data packet (type `2`) or a routing packet (type `4`)
- `Via`: since `F02C`'s routing table says that to reach `DE6C`, the next node you need to send the data to is `DE6C`, the `via` field is modified using that.
- `Seq_Id` and `Num` are used to identify routing (control) packets. (they will be 0 for data packets)

Examples of more logs



- The same packet from `D110`

```
[1494019][V][LoraMesher.cpp:754] printHeaderPacket(): [LoRaMesher] Packet send -- Size: 17 Src: D110 Dst: DE6C Id: 87 Type: 2 Via: F02C Seq_Id: 0 Num: 0
```

- This packet when it is recieved by `F02C`

```
[1527449][V][LoraMesher.cpp:754] printHeaderPacket(): [LoRaMesher] Packet received -- Size: 17 Src: D110 Dst: DE6C Id: 87 Type: 2 Via: F02C Seq_Id: 0 Num: 0
```

- And finally when it is recieved by `DE6C`

```
[1568130][V][LoraMesher.cpp:754] printHeaderPacket(): [LoRaMesher] Packet received -- Size: 17 Src: D110 Dst: DE6C Id: 87 Type: 2 Via: DE6C Seq_Id: 0 Num: 0
```

And finally `DE6C` prints 

```
ReceivedUserData_TaskHandle notify received
Queue receiveUserData size: 1
Packet arrived from D110 with size 8
DATA:74 d110
```
<!-- where the last line is used to parse this data  -->
