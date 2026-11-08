const fs = require("node:fs/promises");
const { Workbook } = require("C:/Users/Li'J'ing/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/@oai/artifact-tool");

const base = "D:/SEIMS_C/data/CW_2/workspace/csv";
const dbbase = "D:/SEIMS_C/seims/preprocess/database";
const outPath = `${base}/hru_subbasin_landuse_vegetation.csv`;

async function loadCsv(path, sheetName) {
  const txt = await fs.readFile(path, "utf8");
  const wb = await Workbook.fromCSV(txt, { sheetName });
  const sh = wb.worksheets.getItem(sheetName);
  return sh.getUsedRange(true).values;
}

function rowsToObjects(values) {
  const headers = values[0].map((v) => String(v).replace(/^\uFEFF/, "").trim());
  return values
    .slice(1)
    .filter((r) => r.some((v) => v !== null && v !== ""))
    .map((r) => {
      const o = {};
      headers.forEach((h, i) => {
        o[h] = r[i] == null ? "" : String(r[i]).trim();
      });
      return o;
    });
}

function uniqueMap(rows, key) {
  const map = new Map();
  const duplicates = [];
  for (const row of rows) {
    if (map.has(row[key])) duplicates.push(row[key]);
    map.set(row[key], row);
  }
  return { map, duplicates };
}

function csvEscape(value) {
  const s = value == null ? "" : String(value);
  return /[",\r\n]/.test(s) ? `"${s.replace(/"/g, '""')}"` : s;
}

async function main() {
  const subRows = rowsToObjects(await loadCsv(`${base}/subbasin.csv`, "subbasin"));
  const landRows = rowsToObjects(await loadCsv(`${base}/land_init.csv`, "land_init"));
  const cropRows = rowsToObjects(await loadCsv(`${base}/crop.csv`, "crop"));
  const landLookupRows = rowsToObjects(await loadCsv(`${dbbase}/LanduseLookup.csv`, "land_lookup"));
  const cropLookupRows = rowsToObjects(await loadCsv(`${dbbase}/CropLookup.csv`, "crop_lookup"));

  const subInfo = uniqueMap(subRows, "FID");
  const landInfo = uniqueMap(landRows, "FID");
  const cropInfo = uniqueMap(cropRows, "FID");
  const landLookup = uniqueMap(landLookupRows, "LANDUSE_ID").map;
  const cropLookup = uniqueMap(cropLookupRows, "ICNUM").map;

  const allFids = new Set([...subInfo.map.keys(), ...landInfo.map.keys(), ...cropInfo.map.keys()]);
  const fids = [...allFids].sort((a, b) => Number(a) - Number(b));
  const missing = [];
  const outputRows = [];

  for (const fid of fids) {
    const sub = subInfo.map.get(fid);
    const land = landInfo.map.get(fid);
    const crop = cropInfo.map.get(fid);
    if (!sub || !land || !crop) {
      missing.push({ fid, subbasin: Boolean(sub), land: Boolean(land), crop: Boolean(crop) });
    }
    const landName = land ? landLookup.get(land.LANDUSE_ID) : null;
    const cropName = crop ? cropLookup.get(crop.ICNUM) : null;
    outputRows.push([
      Number(fid),
      sub ? Number(sub.subbasin) : "",
      land ? Number(land.LANDUSE_ID) : "",
      landName ? landName.CODE : "",
      landName ? landName.LANDUSE_NAME : "",
      crop ? Number(crop.ICNUM) : "",
      cropName ? cropName.CPNM : "",
      cropName ? cropName.CROPNAME : "",
      crop ? Number(crop.IDC) : "",
      crop ? Number(crop.BLAI) : "",
      crop ? Number(crop.RDMX) : "",
    ]);
  }

  const headers = [
    "FID",
    "SUBBASIN_ID",
    "LANDUSE_ID",
    "LANDUSE_CODE",
    "LANDUSE_NAME",
    "ICNUM",
    "VEGETATION_CODE",
    "VEGETATION_NAME",
    "IDC",
    "BLAI",
    "RDMX_MM",
  ];
  const csv = `\uFEFF${[headers, ...outputRows]
    .map((row) => row.map(csvEscape).join(","))
    .join("\r\n")}\r\n`;
  await fs.writeFile(outPath, csv, "utf8");

  const verifyWb = await Workbook.fromCSV(csv, { sheetName: "HRU attributes" });
  verifyWb.recalculate();
  const inspect = await verifyWb.inspect({
    kind: "table",
    range: "HRU attributes!A1:K12",
    include: "values,formulas",
    tableMaxRows: 12,
    tableMaxCols: 11,
    maxChars: 8000,
  });
  const preview = await verifyWb.render({
    sheetName: "HRU attributes",
    range: "A1:K15",
    scale: 1,
    format: "png",
  });
  await fs.writeFile(
    "D:/SEIMS_C/.codex_spreadsheet_tmp/hru_attributes_preview.png",
    new Uint8Array(await preview.arrayBuffer()),
  );

  const targetIds = new Set(["1362", "1363", "1364", "1365", "1366", "1367", "1368", "1370", "1375"]);
  const targets = outputRows.filter((r) => targetIds.has(String(r[0])));
  const summary = {
    outPath,
    counts: {
      subbasin: subRows.length,
      land: landRows.length,
      crop: cropRows.length,
      output: outputRows.length,
    },
    duplicateCounts: {
      subbasin: subInfo.duplicates.length,
      land: landInfo.duplicates.length,
      crop: cropInfo.duplicates.length,
    },
    missingJoinCount: missing.length,
    missingLanduseLookup: outputRows.filter((r) => r[2] !== "" && r[3] === "").length,
    missingCropLookup: outputRows.filter((r) => r[5] !== "" && r[6] === "").length,
    targets,
    inspect: inspect.ndjson,
  };
  console.log(JSON.stringify(summary, null, 2));
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
