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

`SWMM_NODES` required fields are `NODE_ID`, `INVERT_ELEV`. Optional fields are
`SUBBASINID`, `MAX_DEPTH` (default 1 m), `INIT_DEPTH`, `SURCHARGE_DEPTH`,
`PONDED_AREA`, `BOUNDARY_STAGE`, and `NODE_TYPE`. `NODE_TYPE = 1` denotes a
fixed-stage outfall; `BOUNDARY_STAGE` is its water surface elevation.

`SWMM_REACHES` required fields are `REACH_ID`, `FROM_NODE`, `TO_NODE`,
`LENGTH`, `MANNING_N`, and `FULL_DEPTH`. Optional fields are `SUBBASINID`,
`SHAPE`, `BARRELS`, `INLET_OFFSET`, `OUTLET_OFFSET`, `BOTTOM_WIDTH`,
`SIDE_SLOPE`, `INIT_FLOW`, `MAX_FLOW`, `INLET_LOSS`, `OUTLET_LOSS`, and
`AVERAGE_LOSS`.

Shape codes are: `0` rectangular open channel, `1` trapezoidal channel,
`2` circular conduit, and `3` rectangular closed conduit. `BOTTOM_WIDTH` is
ignored for circular conduits, whose diameter is `FULL_DEPTH`. Existing aliases
such as `ID`, `FROM_NODE_ID`, `TO_NODE_ID`, `CH_LEN`, and `CH_N` are accepted.

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
