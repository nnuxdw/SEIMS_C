# SWMM_DW: WISE river-network dynamic wave

`SWMM_DW` is a routing-only migration of the EPA SWMM 5.2.4 dynamic-wave
conduit solver. It uses SI units and can route a node/link graph with loops,
diversions and reverse flow. The module reads the two collections below on its
first initialization only; WISE's regular `REACHES` collection is untouched.

## MongoDB collections

The canonical collection names are `SWMM_NODES` and `SWMM_REACHES` in the
active WISE model database. For compatibility, the module also recognizes
`SWMM_NODE`/`SWMM_LINKS` and `RIVER_NODES`/`RIVER_REACHES` (canonical names
take priority).

### `SWMM_NODES` fields

| Field | Required | Unit / allowed values | Meaning and default |
| --- | --- | --- | --- |
| `NODE_ID` | Yes | Integer, unique | Node identifier. |
| `SUBBASINID` | No | WISE subbasin index | Index of lateral inflow `SBOF + SBIF + SBQG`; default `0` means no direct lateral inflow. |
| `INVERT_ELEV` | Yes | m | Junction/channel-bed elevation. |
| `MAX_DEPTH` | No | m, >= 0 | Bank/crest depth above `INVERT_ELEV`; if omitted, the highest connected channel top is used. |
| `INIT_DEPTH` | No | m, >= 0 | Initial water depth; default `0` (dry). |
| `SURCHARGE_DEPTH` | No | m, >= 0 | Depth permitted above `MAX_DEPTH`; default `0`. Keep `0` for ordinary open-channel nodes. |
| `PONDED_AREA` | No | m2, >= 0 | Fixed water/storage area at the node; `0` (default) uses `SWMM_MIN_SURFACE_AREA`. |
| `NODE_TYPE` | No | `0` or `1` | `0` (default): ordinary hydraulic junction; `1`: fixed-stage outfall. Other values are rejected. |
| `BOUNDARY_STAGE` | No | m | Water-surface elevation used only when `NODE_TYPE=1`; default is `INVERT_ELEV`. |

### `SWMM_REACHES` fields

| Field | Required | Unit / allowed values | Meaning and default |
| --- | --- | --- | --- |
| `REACH_ID` | Yes | Integer, unique | Reach identifier. |
| `SUBBASINID` | No | WISE subbasin index | Target index for the reach contribution to `QRECH`; `0` omits it from the per-subbasin output. |
| `FROM_NODE`, `TO_NODE` | Yes | Existing `NODE_ID` | Defines positive discharge direction: `FROM_NODE -> TO_NODE`. Negative output means reverse flow. |
| `SHAPE` | No | `0`, `1`, `2`, `3` | `0`: open rectangle; `1`: open trapezoid (default); `2`: circular closed conduit; `3`: rectangular closed conduit. Select `0` or `1` for rivers/open channels. |
| `BARRELS` | No | Integer >= 1 | Number of identical parallel passages; default `1`. Use separate reaches, not `BARRELS`, where parallel branches have different geometry or connectivity. |
| `LENGTH` | Yes | m, > 0 | Hydraulic length. |
| `MANNING_N` | Yes | -, > 0 | Manning roughness coefficient. |
| `INLET_OFFSET` | No | m | Reach-bed offset above `FROM_NODE` invert; default `0`. |
| `OUTLET_OFFSET` | No | m | Reach-bed offset above `TO_NODE` invert; default `0`. |
| `FULL_DEPTH` | Yes | m, > 0 | Bankfull depth for open sections; diameter for `SHAPE=2`; inside height for `SHAPE=3`. |
| `BOTTOM_WIDTH` | Yes except `SHAPE=2` | m, > 0 | Bed width for `SHAPE=0/1`, inside width for `SHAPE=3`; ignored for a circular conduit. A river width is never fabricated by a default. |
| `SIDE_SLOPE` | No | H:V, >= 0 | One-bank side slope, used only for `SHAPE=1`; default `2.0` (= 2H:1V on each bank). Ignored for the other shapes. |
| `INIT_FLOW` | No | m3/s | Signed initial flow (`FROM_NODE -> TO_NODE` positive); default `0`. |
| `MAX_FLOW` | No | m3/s, >= 0 | Symmetric absolute flow limit; `0` (default) disables the limit. |
| `INLET_LOSS` | No | - | Inlet local-loss coefficient; default `0`. |
| `OUTLET_LOSS` | No | - | Outlet local-loss coefficient; default `0`. |
| `AVERAGE_LOSS` | No | - | Distributed local-loss coefficient; default `0`. |

Existing aliases such as `ID`, `FROM_NODE_ID`, `TO_NODE_ID`, `CH_LEN`, and
`CH_N` are accepted.

### WISE module parameters

| Parameter | Unit / allowed values | Meaning and default |
| --- | --- | --- |
| `CHANNEL_DT` | s, > 0 | Required WISE channel-routing interval. |
| `SWMM_MAX_TRIALS` | Integer >= 1 | Maximum Picard iterations in one internal dynamic-wave step; default `8`. |
| `SWMM_HEAD_TOL` | m, > 0 | Node-depth convergence tolerance; default `0.001` m. |
| `SWMM_MIN_ROUTE_STEP` | s, >= 0.001 | Lower limit of the internally split routing step; default `1` s. |
| `SWMM_COURANT_FACTOR` | >= 0 | `0` disables Courant substepping; a positive value scales the stable step; default `0.75`. |
| `SWMM_MIN_SURFACE_AREA` | m2, > 0 | Minimum node storage area; default `1.167 m2`, the SI equivalent of SWMM's default 4-ft-diameter node. |

## WISE coupling

`SBOF + SBIF + SBQG` is applied as lateral inflow at every node whose
`SUBBASINID` matches its WISE subbasin. `QRECH[SUBBASINID]` receives the sum of
links carrying that `SUBBASINID`; `QRECH[0]` is total discharge to outfalls.
The `SWMM_*` outputs are ordered by ascending `NODE_ID` or `REACH_ID`, so they
are unambiguous even where a subbasin has multiple links.

Add `SWMM_DW` as the channel-routing module in the model's module setting and
remove the alternative channel-routing module (`IKW_REACH`, `MUSK_CH`, etc.)
from that setting. This prevents two modules from publishing `QRECH` for the
same model run.

The module includes SWMM's Picard iteration, semi-implicit Saint-Venant
momentum update, Froude-based inertial damping, under-relaxation, local-loss
terms, surcharge handling and a Courant-limited internal time step. Pumps,
orifices, weirs, RTC rules, SWMM rainfall-runoff and water-quality routines are
out of scope for this river-routing module.
