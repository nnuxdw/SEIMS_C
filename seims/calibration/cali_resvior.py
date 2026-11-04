from pymongo import MongoClient, UpdateOne

def update_calibration_values(mongo_uri: str):
    client = MongoClient(mongo_uri)
    col = client["poyang_lake1_longterm_model_141"]["PARAMETERS"]

    updates = {
        "RES_LC": +0.05,  # 实值 0.15  → 2Lc=0.30，低水保护区适中
        "RES_LN": +0.10,  # 实值 0.40  → 缩短段2，避免长期攒水到 3b
        "RES_LF": +0.03,  # 实值 1.00  → 推迟超限(>Lf)与 4 段触发
        "RES_ADJUST": -0.10,  # 实值 0.85  → Normal_Flood 上移但别到顶
        "RES_minq": -10,  # 实值 20    → 保留一定底流，避免过度抬高枯季
        "RES_normq": +20,  # 实值 160   → 常态出流提高，少攒水
        "RES_normMult": -0.10,  # 实值 0.9   → NormalQ 微抬, 更平稳
        "RES_ndq":  +2300   #实值 4000     # 上限足够覆盖3500
    }

    for name, value in updates.items():
        doc = col.find_one({"NAME": name}, {"CALI_VALUES": 1})
        if not doc:
            print(f"[skip] {name} not found")
            continue

        raw = doc.get("CALI_VALUES", "")
        # 兼容：若已有就是数组，则转成字符串列表处理
        if isinstance(raw, list):
            parts = [str(x) for x in raw]
        else:
            raw = str(raw)
            parts = [p.strip() for p in raw.split(",")] if raw else []

        # 确保至少 1 个元素
        if len(parts) == 0:
            parts = ["0"]

        # 替换第 0 个值
        parts[0] = str(value)

        new_str = ",".join(parts)
        col.update_one({"NAME": name}, {"$set": {"CALI_VALUES": new_str}})
        print(f"[ok] {name} -> CALI_VALUES='{new_str}'")

if __name__ == "__main__":
    # 示例：本地 MongoDB，无账号密码
    update_calibration_values("mongodb://localhost:27017")

