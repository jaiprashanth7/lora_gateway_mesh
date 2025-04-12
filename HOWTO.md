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

You can run `pio run -t compiledb` to produce `compile_commands.json` which can by used by LSPs to find functions.

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




