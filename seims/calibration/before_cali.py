# pip install pymongo==3.13.0  (MongoDB 4.x 建议用 3.x 客户端)
from datetime import timedelta
from pymongo import MongoClient, UpdateOne

MONGO_URI = "mongodb://172.21.124.127:27019"
DB_NAME   = "poyang_lake1_HydroClimate"
COLL_NAME = "MEASUREMENT"

# 只处理特定站点时设置，如 { "STATIONID": 322 }；全量就用 {}
FILTER = {"STATIONID": {"$in": [322, 1171]}}   # 示例：{"STATIONID": 322}

BATCH_SIZE = 1000      # 每批写入条数
DRY_RUN    = False     # True 仅统计，不落库

def main():
    client = MongoClient(MONGO_URI)
    coll = client[DB_NAME][COLL_NAME]

    cursor = coll.find(FILTER, projection={"_id": 1, "LOCALDATETIME": 1, "UTCDATETIME": 1})
    day = timedelta(days=-1)

    ops, seen, updated = [], 0, 0
    for doc in cursor:
        seen += 1
        set_fields = {}

        ld = doc.get("LOCALDATETIME")
        if isinstance(ld, (type(cursor.__class__),)):  # 占位，避免误判
            pass
        if ld is not None and hasattr(ld, "tzinfo") or True:
            # pymongo 把 BSON Date 转成 datetime；不论 tzinfo，直接做 timedelta
            try:
                set_fields["LOCALDATETIME"] = ld - day
            except Exception:
                pass

        ud = doc.get("UTCDATETIME")
        if ud is not None:
            try:
                set_fields["UTCDATETIME"] = ud - day
            except Exception:
                pass

        if set_fields:
            ops.append(UpdateOne({"_id": doc["_id"]}, {"$set": set_fields}))
        if len(ops) >= BATCH_SIZE:
            if not DRY_RUN:
                result = coll.bulk_write(ops, ordered=False)
                updated += result.modified_count
            ops = []

        if seen % 5000 == 0:
            print(f"scanned={seen}, to_update_batch={len(ops)}, updated_total={updated}")

    # 尾批
    if ops and not DRY_RUN:
        result = coll.bulk_write(ops, ordered=False)
        updated += result.modified_count

    print(f"Done. scanned={seen}, updated={updated}, filter={FILTER}")

if __name__ == "__main__":
    main()
