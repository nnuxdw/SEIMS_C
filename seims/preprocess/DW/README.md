# DW Preprocess Scripts

This folder keeps the preprocessing scripts used by the HAND-based diffusive
wave channel-routing modules:

- `DW_CH_HAND`
- `DWNEW_CH_HAND`

## Current Script

`cw.py` is the current working script copied from:

```text
D:\a_lp\python\DW_preprocess\cw.py
```

Why this is treated as the active version:

- It is the newest script in the source folder.
- It targets the current `CW_2_longterm_model` database.
- It uses the `CW_2` raster workspace paths.
- It reads `FloodStep.txt`.
- It selects lake/reservoir `CH_OUTLET_HANDID` from the `0.0-0.1 m` HAND band.
- It writes the DW control-section fields used by the routing module:
  - `CH_BED_ELEV`
  - `CH_OUTLET_HANDID`
  - `CH_STREAM_MAX_RELIEF`

## Near-Current Backup

`preprocess_reach_control_bed_hand_lake_01m_fixed.py` is kept beside the active
script because it is the immediately preceding lake-HAND correction version.
The main difference is that it still uses the older exact `0.00,1.00` lake HAND
interval logic, while `cw.py` switches to numeric interval parsing and the
`0.0-0.1 m` target band.

## Archive

`archive/` contains earlier versions for traceability:

- `preprocess_reach_control_bed_hand.py`: older SLC_648/CW_2 transitional version.
- `preprocess_reach_control_bed_hand_old.py`: older SLC_650 version.
- `update_outlet_handid_and_bed_elev_abandoned.py`: abandoned first-pass outlet
  update script, renamed from the original non-ASCII filename.

## Relation To DW Routing

The DW routing code treats `m_chBedElev` and `m_outletHandId` as control-section
metadata. These scripts prepare those values in MongoDB `REACHES`, so the module
can separate:

- whole-reach storage state: `m_chSto`, `m_rteWtrIn`, `m_rteWtrOut`
- HAND inundation state: `m_handWtrDep`, `m_subbasinWtrDep`,
  `m_subbasinInundationArea`
- control-section exchange state: `m_chBedElev`, `m_chWtrDepth`, `m_sfcElv`,
  `m_chWtrWth`, `m_chCrossArea`

The important modeling rule is that the diffusive-wave exchange should use the
control-section state, not the whole-subbasin representative inundation depth.
