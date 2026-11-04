import pandas as pd
import matplotlib.pyplot as plt

def plot_sum_flooded_area_by_mask(
    flood_csv: str,       # 图1：包含 subbasin,time,flooded_area_km2
    mask_csv: str,        # 图2：包含 subbasin,caliparam_sub
    time_col: str = "time",
    sub_col: str = "subbasin",
    area_col: str = "flooded_area_km2",
    mask_flag_col: str = "caliparam_sub",
    save_png: str  = None
):
    df = pd.read_csv(flood_csv)
    mask = pd.read_csv(mask_csv)

    df[sub_col] = pd.to_numeric(df[sub_col], errors="raise").astype("int64")
    mask[sub_col] = pd.to_numeric(mask[sub_col], errors="raise").astype("int64")
    df[time_col] = pd.to_datetime(df[time_col], errors="coerce", infer_datetime_format=True)

    white = mask.loc[mask[mask_flag_col] == 1, sub_col].dropna().unique().tolist()
    if not white:
        raise ValueError("mask 中没有 caliparam_sub==1 的子流域")

    dff = df[df[sub_col].isin(white)].copy()
    summed = (dff.groupby(time_col, as_index=False)[area_col]
                  .sum()
                  .rename(columns={area_col: "sum_flooded_area_km2"})
                  .sort_values(time_col))

    # === 曲线图 ===
    plt.figure(figsize=(10, 4))
    plt.plot(summed[time_col], summed["sum_flooded_area_km2"], marker="o")
    plt.xlabel("time")
    plt.ylabel("sum_flooded_area_km2")
    plt.title("Total flooded area over selected subbasins")
    plt.tight_layout()
    if save_png:
        plt.savefig(save_png, dpi=150)
    plt.show()

    return summed, white

if __name__ == '__main__':
    subbasinid = 322
    flood_csv = r'J:\G\program\seims\SEIMS_HAND\data\poyang_lake\inundation_cali\subbasin_flood_area.csv'
    mask_csv = f"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\TNH_caliparam_sub_{subbasinid}.csv"
    png_path = f'G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\obs\obs_flood_{subbasinid}.png'
    summed_df, selected_subs = plot_sum_flooded_area_by_mask(
        flood_csv=flood_csv,   # 图1CSV路径
        mask_csv=mask_csv,              # 图2CSV路径
        save_png=png_path
    )
    print(summed_df.head())
    print("选中的子流域：", selected_subs)
