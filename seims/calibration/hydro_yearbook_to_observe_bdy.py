import pandas as pd
import re
from datetime import datetime
import sys
import os
import glob

# --- 月份文本 -> 数字 ---
def to_month(x):
    s = str(x).strip()
    m_map = {
        "一月":1,"二月":2,"三月":3,"四月":4,"五月":5,"六月":6,
        "七月":7,"八月":8,"九月":9,"十月":10,"十一月":11,"十二月":12,
        "Jan":1,"Feb":2,"Mar":3,"Apr":4,"May":5,"Jun":6,
        "Jul":7,"Aug":8,"Sep":9,"Oct":10,"Nov":11,"Dec":12
    }
    if s in m_map: return m_map[s]
    m = re.search(r"(\d{1,2})", s)
    return int(m.group(1)) if m else None

def fmt_dt(series, zero_pad=False):
    if zero_pad:
        return series.dt.strftime("%Y/%m/%d 00:00")
    if sys.platform.startswith("win"):
        return series.dt.strftime("%Y/%#m/%#d 0:00")
    return series.dt.strftime("%Y/%-m/%-d 0:00")

# 无效值（含字母、负数等）返回 None
def parse_value_keep_blank(val):
    if pd.isna(val):  # 空
        return None
    s = str(val).strip()
    if s == "":
        return None
    if re.search(r"[A-Za-z]", s):  # 含任何字母（如 120N）
        return None
    s2 = s.replace(",", "")        # 去千分位
    try:
        v = float(s2)
    except Exception:
        return None
    if v < 0:                      # 负数无效
        return None
    return v

def read_one_year_df(in_xlsx, year, station_id=1, type_code="Q", sheet_name=0):
    try:
        raw = pd.read_excel(in_xlsx, sheet_name=sheet_name, header=None, engine="openpyxl")
    except Exception:
        raw = pd.read_excel(in_xlsx, sheet_name=sheet_name, header=None)

    # 固定区域：第2行2~13列=月；第1列3~33行=日；第3行起2~13列=数据
    months = [to_month(v) for v in raw.iloc[1, 1:13].tolist()]
    days   = raw.iloc[2:33, 0].tolist()
    data   = raw.iloc[2:33, 1:13].values

    rec = []
    for i, day in enumerate(days):
        try:
            day = int(str(day).strip())
        except Exception:
            continue
        if not (1 <= day <= 31):
            continue

        for j, m in enumerate(months):
            if m is None or not (1 <= m <= 12):
                continue
            # 合法日期
            try:
                dt = datetime(int(year), int(m), int(day))
            except ValueError:
                continue
            v = parse_value_keep_blank(data[i, j])  # 可能为 None
            rec.append((station_id, dt, type_code, v))

    return pd.DataFrame(rec, columns=["StationID", "DT", "Type", "VALUE"])

def extract_year_from_name(fname):
    base = os.path.basename(fname)
    m = re.search(r'(\d{4})\s*年', base)
    if m: return int(m.group(1))
    m = re.search(r'(19|20)\d{2}', base)
    return int(m.group(0)) if m else None

def build_observed_q(input_dir, output_csv,
                     station_id=1, type_code="Q",
                     sheet_name=0, zero_pad=True,
                     pattern="*.xlsx"):
    files = sorted(glob.glob(os.path.join(input_dir, pattern)))
    if not files:
        raise FileNotFoundError(f"未找到文件：{os.path.join(input_dir, pattern)}")

    all_df = []
    for f in files:
        year = extract_year_from_name(f)
        if year is None:
            print("[WARN] 跳过（无法提取年份）：", f)
            continue
        try:
            df = read_one_year_df(f, year, station_id, type_code, sheet_name)
            all_df.append(df)
            print(f"[OK] 读取 {os.path.basename(f)} 年={year} 行数={len(df)}")
        except Exception as e:
            print(f"[ERROR] 读取失败 {os.path.basename(f)}：{e}")

    if not all_df:
        raise ValueError("没有成功读取到任何数据。")

    big = pd.concat(all_df, ignore_index=True)

    # ★ 关键修改：删除 VALUE 为 None/NaN 的行（不写入 CSV）
    before = len(big)
    big = big.dropna(subset=["VALUE"])
    removed = before - len(big)
    if removed:
        print(f"[INFO] 已移除无效值行：{removed}")

    big.sort_values("DT", inplace=True)
    big["DATETIME"] = fmt_dt(pd.to_datetime(big["DT"]), zero_pad=zero_pad)
    out = big[["StationID", "DATETIME", "Type", "VALUE"]]

    out.to_csv(output_csv, index=False, encoding="utf-8-sig")
    with open(output_csv, "r+", encoding="utf-8-sig") as f:
        content = f.read()
        f.seek(0, 0)
        f.write("#UTCTIME\n" + content)

    print(f"\n[FINISH] 输出 {output_csv}，总行数={len(out)}")

if __name__ == '__main__':
    input_dir = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\data_prepare\observed\2008-2020年湖口站逐日平均流量表"
    out_csv   = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\data_prepare\observed\observed_Q.csv"
    build_observed_q(
        input_dir=input_dir,
        output_csv=out_csv,
        station_id=1171,
        type_code="Q",
        sheet_name=0,
        zero_pad=True
    )
