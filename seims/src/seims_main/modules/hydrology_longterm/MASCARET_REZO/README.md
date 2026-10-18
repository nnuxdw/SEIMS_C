# MASCARET_REZO

This C++ WISE channel-routing module has two input modes. Without additional
configuration it keeps the original reach-to-downstream-reach routing mode.
That mode uses WISE's existing `clsReaches` topology and has the same tree
restriction as before.

The optional node-reach mode accepts any connected graph, including junctions,
diffluences, directed cycles, and links whose computed flow reverses direction.
Set both environment variables before starting WISE:

```powershell
$env:WISE_REZO_NETWORK_FILE = 'C:\study\network.csv'
$env:WISE_REZO_BOUNDARY_FILE = 'C:\study\boundary.csv'
```

The network CSV contains one row for each WISE reach, with reach IDs `1..N`:

```csv
reach_id,from_node,to_node,from_bed_m,to_bed_m
1,1,2,1.0,0.6
2,2,3,0.6,0.4
3,2,4,0.6,0.3
4,4,2,0.3,0.6
```

Node IDs are positive integers. Repeated node bed elevations must agree. Edge
orientation defines the positive sign of `QRECH`; negative flow is allowed.
Every reach ID must occur exactly once. The boundary CSV specifies fixed water
surface elevations in metres:

```csv
node_id,stage_m
3,0.5
```

Each connected component needs at least one fixed-stage node. This supplies an
open water boundary and anchors the water level. A fixed-stage node can supply
or absorb water to keep its level constant. The current boundary file describes
constant stages, not time-varying tide or gate rules.

The normal WISE `REACHES` collection still supplies width, depth, length, side
slope and Manning roughness for every reach. Its old `DOWNSTREAM` and layer
fields remain available to other modules, but this module ignores them when
the node-reach files are configured. Existing `clsReaches` records can therefore
remain in place; for a new cyclic study, use placeholder legacy downstream
values and supply the actual topology in `network.csv`. Do not run legacy
tree-only channel modules against those placeholders.

Use the whole-network, single-process WISE configuration (`SUBBASINID=0`).
`SBOF`, `SBIF` and `SBQG` provide nonnegative reach lateral discharges in m³/s.
Each reach's lateral inflow is split equally between its end nodes. `QRECH`
reports signed reach discharge in m³/s; `CHWTRDEPTH` reports the mean of its
endpoint depths in metres. The outlet scalar `QRECH` uses `OUTLETID` as the
reach ID to sample. Array index zero remains reserved.

The solver uses reach-node water balance, explicit local inertia, Manning
friction and adaptive substeps. Node storage is assembled from half of each
incident reach's trapezoidal volume. This is a reduced hydraulic model and
does not reproduce TELEMAC-MASCARET REZO's Preissmann scheme, detailed cross
sections, structures or boundary laws. A numerical comparison with a TELEMAC
study is required before using it for engineering conclusions.

The CMake target is `MASCARET_REZO`. Independent topology, conservation and
reverse-flow tests are available through `test/CMakeLists.txt` and its CSV
fixtures. Configure and build that small test project separately, then run
`ctest` in its build directory.
