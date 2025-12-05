import pandas as pd
import re
from datetime import datetime
import sys
import os
import glob
from pathlib import Path

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

    out.to_csv(output_csv, index=False, encoding="utf-8")
    with open(output_csv, "r+", encoding="utf-8") as f:
        content = f.read()
        f.seek(0, 0)
        f.write("#UTCTIME\n" + content)

    print(f"\n[FINISH] 输出 {output_csv}，总行数={len(out)}")

def split_excel_by_sheets(src_path: str, out_dir: str):
    """
    把单个 xlsx 的多个 sheet 拆成多个 xlsx
    文件名 = <sheet名>年<站名>站逐日平均流量表.xlsx

    参数：
        src_path: 源文件路径
        out_dir : 输出目录（会在其中创建站名子目录）
    返回：
        输出文件路径列表
    """
    src = Path(src_path)
    station = src.stem.strip()
    station_with_suffix = station if station.endswith("站") else f"{station}站"

    # 在输出目录下创建以站名命名的子目录
    station_dir = Path(out_dir) / station
    station_dir.mkdir(parents=True, exist_ok=True)

    xls = pd.ExcelFile(src, engine="openpyxl")
    saved_files = []

    for sheet in xls.sheet_names:
        year = str(sheet).strip()
        df = pd.read_excel(xls, sheet_name=sheet)

        out_name = f"{year}年{station_with_suffix}逐日平均流量表.xlsx"
        out_path = station_dir / out_name

        with pd.ExcelWriter(out_path, engine="openpyxl") as writer:
            df.to_excel(writer, sheet_name=sheet, index=False)

        saved_files.append(str(out_path.resolve()))
        print(f"已保存：{out_path}")

    return saved_files


# —— 批量版本（可选）——
def batch_split_folder(folder: str, out_root):
    """
    扫描文件夹下的 .xlsx（跳过临时文件 ~$.xlsx），
    每个文件按站名在指定目录下生成子文件夹并拆分。
    """
    folder_path = Path(folder)
    out_root = Path(out_root) if out_root else (folder_path / "拆分结果")
    out_root.mkdir(parents=True, exist_ok=True)

    for file in folder_path.glob("*.xlsx"):
        if file.name.startswith("~$"):
            continue
        split_excel_by_sheets(str(file), out_dir=out_root)


def batch_merge_folder(stations_root: str, station_id_map, out_base: str):
    """
    遍历指定根目录下“以站点命名”的子文件夹，并为每个站点调用 build_observed_q。

    参数
    ----
    stations_root : str
        根目录路径。其下每个子文件夹名称即为站点名（用于在 station_id_map 中取 ID）。
    station_id_map : dict[str, int]
        {站点名: station_id} 的映射表。
    out_base : str
        输出 CSV 的基础目录；最终文件名为 observed_Q_{station_id}.csv。
    """
    stations_root = Path(stations_root)
    out_base = Path(out_base)
    out_base.mkdir(parents=True, exist_ok=True)

    if not stations_root.exists():
        raise FileNotFoundError(f"找不到目录：{stations_root}")

    for item in stations_root.iterdir():
        # 只处理“以站点命名的文件夹”
        if not item.is_dir():
            continue

        station_name = item.name.strip()
        if station_name not in station_id_map:
            print(f"[跳过] 站点'{station_name}'未在 station_id_map 中配置。")
            continue

        station_id = station_id_map[station_name]
        out_csv = out_base / f"observed_Q_{station_id}.csv"

        try:
            print(f"[处理] 站点: {station_name} (ID={station_id})")
            build_observed_q(
                input_dir=str(item),  # 该站点文件夹
                output_csv=str(out_csv),  # 输出路径
                station_id=station_id,
                type_code="Q",  # 固定
                sheet_name=0,  # 固定
                zero_pad=True  # 固定
            )
            print(f"[完成] 输出: {out_csv}")
        except Exception as e:
            print(f"[错误] 站点 {station_name} (ID={station_id}) 处理失败：{e}")

if __name__ == '__main__':
    # step1：批量处理：针对本科生数字化的水文年鉴格式（多个sheet页，每个sheet为一年），分割为多个xlsx文件，每个为一年
    input_benkesheng_dir = r"J:\G\data\鄱阳湖数据\流量\其它站点\原始数据"
    split_dir = r'J:\G\data\鄱阳湖数据\流量\其它站点\拆分数据'

    # split_excel_by_sheets(input_benkesheng,output_dir)  # 单个站点处理
    batch_split_folder(input_benkesheng_dir,split_dir)    # 多站点批量处理
    # step2：批量处理：将分割好的流量文件，按照站点转为SEIMS Observe需要的文件
    observe_dir = r'J:\G\data\鄱阳湖数据\流量\其它站点\站点合并数据'
    station_id_map = {
        "湖口": 1171,
        "外洲": 322,
        "万家埠": 225,
        "李家渡": 457,
        "梅港": 347,
        "虎山": 214,
        "渡峰坑": 123,
        "虬津": 141,
    }

    batch_merge_folder(
        stations_root=split_dir,  # 里面每个子文件夹是一个站名
        station_id_map=station_id_map,
        out_base=observe_dir  # 输出目录
    )

    # 针对百度云下载的水文年鉴格式
    input_dir = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\data_prepare\observed\2008-2020年湖口站逐日平均流量表"
    out_csv   = r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\data_prepare\observed\observed_Q_1171.csv"
    build_observed_q(
        input_dir=input_dir,
        output_csv=out_csv,
        station_id=1171,
        type_code="Q",
        sheet_name=0,
        zero_pad=True
    )
