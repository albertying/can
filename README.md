# can

CAN bus decoder in C++20. Give it a DBC file and a candump log, get CSV out.

```
./tool vehicle.dbc candump.log
```
```
1700000000.032871,SteeringAngle,-3.5
1700000000.032871,SteeringSpeed,0
1700000000.033104,BrakePedalPos,0
...
```

Pass just a DBC to list the messages and signals in it:

```
./tool vehicle.dbc
```

## What it does

CAN frames are raw bytes. A DBC file is the schema -- which bits mean what, byte order, scale, offset. This reads both and decodes every frame it recognizes.

Handles Intel (LE) and Motorola (BE) byte order, multiplexed signals, extended 29-bit frame IDs, and scale/offset to physical values.

## Build

```
make        # build
make test   # run tests
```

Requires g++ with C++20 support. No external dependencies.

## Options

```
./tool file.dbc log.log -s SteeringAngle,BrakePedalPos   # filter to specific signals
./tool file.dbc log.log --stats                           # print min/max/mean per signal to stderr
```

Tests were written with AI assistance.
