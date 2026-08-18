#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
根据基准参数、率定改变值和 MongoDB 中的 CHANGE 规则生成最终参数 CSV。

计算规则（与 SEIMS C++ ParamInfo.cpp 一致）：
    AC: 最终值 = 基准值 + 改变值
    RC: 最终值 = 基准值 * 改变值
    VC: 最终值 = 改变值（直接替换）
    NC: 最终值 = 基准值（不改变）

数据按标识列逐行对齐：
    caliparam_sub.csv  <-> param_group2.csv + param_group2_ch.csv，标识列为 subbasin
    caliparam.csv      <-> param_group1.csv，标识列为 FID

仅依赖 pymongo，其余均使用 Python 标准库：
    pip install pymongo
"""

import csv
import os
import re
from decimal import Decimal, InvalidOperation
from pathlib import Path


# ============================================================================
# 1. 手动配置所有文件路径
# ============================================================================

PARAM_GROUP1_CSV = Path(r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\param_group1.csv")
PARAM_GROUP2_CSV = Path(r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\param_group2.csv")
PARAM_GROUP2_CH_CSV = Path(r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model\param_group2_ch.csv")

CALIPARAM_SUB_CSV = Path(r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\caliparam_sub.csv")
CALIPARAM_CSV = Path(r"G:\program\seims\SEIMS_HAND\data\poyang_lake1\poyang_lake1_longterm_model_1171\caliparam.csv")

SUBBASIN_OUTPUT_CSV = Path(
    r"G:\program\seims\SEIMS_HAND\data\poyang_lake\文章数据\subbasin_relate_params.csv"
)
HAND_OUTPUT_CSV = Path(r"G:\program\seims\SEIMS_HAND\data\poyang_lake\文章数据\hand_relate_params.csv")


# ============================================================================
# 2. 手动配置 MongoDB
# ============================================================================

MONGO_IP = "127.0.0.1"
MONGO_PORT = 27017
MONGO_DATABASE = "poyang_lake1_longterm_model_1171"
MONGO_COLLECTION = "PARAMETERS"

# MongoDB 无账号密码时保持为 None；如有认证则直接在这里填写。
MONGO_USERNAME = None
MONGO_PASSWORD = None
MONGO_AUTH_SOURCE = None
MONGO_TIMEOUT_MS = 10000


# ============================================================================
# 3. 其他固定配置
# ============================================================================

INPUT_ENCODING = "utf-8-sig"
OUTPUT_ENCODING = "utf-8"
NODATA_VALUE = Decimal("-9999")
OVERWRITE_OUTPUTS = True

# 若某个参数在率定文件中有有效改变值，但未出现在基准参数 CSV 中，可在这里
# 手动补充统一的基准值。键名大小写不敏感，末尾是否带 _1D 均可。
#
# 示例（数值仅为写法示例，不是真实基准值，请勿直接照抄）：
# BASE_VALUE_OVERRIDES = {
#     "RES_minq": "10.0",
#     "RES_normq": "50.0",
#     "RES_ndq": "2000.0",
# }
BASE_VALUE_OVERRIDES = {}


# ============================================================================
# 4. 输出参数过滤与别名（可选）
# ============================================================================

# 是否仅输出指定的参数子集。False 时输出全部参数列。
FILTER_OUTPUT_PARAMETERS = True

# 当 FILTER_OUTPUT_PARAMETERS 为 True 时，仅输出以下参数。
# 参数名大小写不敏感，末尾是否带 _1D 均可。
OUTPUT_PARAMETER_NAMES = {
    "K_PET",
    "Runoff_co",
    "Ki_1d",
    "BASE_EX_1D",
    "KG_1D",
    "LAKEB_1D",
    "LAKE_ALPHA",
    "LAKE_MNLWL_1D",
    "CH_N",
    "SURLAG"
}

# 参数输出别名映射：键为原始参数名，值为输出 CSV 中使用的列名。
# 参数名大小写不敏感，末尾是否带 _1D 均可。
# 无论 FILTER_OUTPUT_PARAMETERS 是否开启，别名均生效。
OUTPUT_PARAMETER_ALIASES = {
    "K_PET": "K_PET",
    "Runoff_co": "C",
    "Ki_1d":"Cs",
    "BASE_EX_1D":"m",
    "KG_1D":"Cg",
    "LAKEB_1D":"β",
    "LAKE_ALPHA":"Kr",
    "LAKE_MNLWL_1D":"H0",
    "CH_N":"n",
    "SURLAG":"surlag"
}


class ParameterProcessingError(RuntimeError):
    """输入数据不完整或不一致。"""


_ONE_D_SUFFIX = re.compile(r"_1d$", re.IGNORECASE)


def _csv_parameter_key(name):
    """CSV 参数名匹配键：忽略大小写，但保留 _1D。"""
    return str(name).strip().casefold()


def _mongo_parameter_key(name):
    """MongoDB 参数名匹配键：忽略大小写，并移除 CSV 常用的 _1D 后缀。"""
    value = _ONE_D_SUFFIX.sub("", str(name).strip())
    return value.casefold()


def _to_decimal(value, location):
    text = str(value).strip()
    if not text:
        raise ParameterProcessingError("{} 为空，无法参与计算".format(location))
    try:
        number = Decimal(text)
    except InvalidOperation:
        raise ParameterProcessingError(
            "{} 不是有效数值：{!r}".format(location, value)
        )
    if not number.is_finite():
        raise ParameterProcessingError(
            "{} 不是有限数值：{!r}".format(location, value)
        )
    return number


def _decimal_to_text(number):
    """避免把 Decimal 输出为多余的科学计数法或 -0。"""
    text = format(number, "f")
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    if text in ("", "-0"):
        return "0"
    return text


def _read_csv(path):
    if not path.is_file():
        raise FileNotFoundError("CSV 文件不存在：{}".format(path))

    with path.open("r", encoding=INPUT_ENCODING, newline="") as file_obj:
        reader = csv.DictReader(file_obj)
        if not reader.fieldnames:
            raise ParameterProcessingError("CSV 没有表头：{}".format(path))
        fieldnames = list(reader.fieldnames)
        rows = list(reader)

    if not rows:
        raise ParameterProcessingError("CSV 没有数据行：{}".format(path))
    return fieldnames, rows


def _resolve_column(fieldnames, requested_name, path):
    requested_key = str(requested_name).strip().casefold()
    matches = [
        field for field in fieldnames
        if str(field).strip().casefold() == requested_key
    ]
    if len(matches) != 1:
        raise ParameterProcessingError(
            "{} 中无法唯一找到标识列 {!r}".format(path, requested_name)
        )
    return matches[0]


def _check_unique_ids(rows, id_column, path):
    seen = set()
    for row_number, row in enumerate(rows, start=2):
        row_id = str(row[id_column]).strip()
        if not row_id:
            raise ParameterProcessingError(
                "{} 第 {} 行的 {} 为空".format(path, row_number, id_column)
            )
        if row_id in seen:
            raise ParameterProcessingError(
                "{} 中存在重复的 {}：{}".format(path, id_column, row_id)
            )
        seen.add(row_id)


def load_baseline_tables(paths, requested_id_column):
    """
    合并一组基准参数 CSV。

    返回：
        baseline_values[参数键][ID] = Decimal(基准值)
        display_names[参数键] = 原始列名
    """
    baseline_values = {}
    display_names = {}

    for path in paths:
        fieldnames, rows = _read_csv(path)
        id_column = _resolve_column(fieldnames, requested_id_column, path)
        _check_unique_ids(rows, id_column, path)

        for column in fieldnames:
            if column == id_column:
                continue

            parameter_key = _csv_parameter_key(column)
            if parameter_key in baseline_values:
                raise ParameterProcessingError(
                    "基准参数 {!r} 在多个基准 CSV 中重复出现".format(column)
                )

            values_by_id = {}
            for row_number, row in enumerate(rows, start=2):
                row_id = str(row[id_column]).strip()
                values_by_id[row_id] = _to_decimal(
                    row[column],
                    "{} 第 {} 行参数 {}".format(path, row_number, column),
                )

            baseline_values[parameter_key] = values_by_id
            display_names[parameter_key] = column

    return baseline_values, display_names


def load_calibration_table(path, requested_id_column):
    fieldnames, rows = _read_csv(path)
    id_column = _resolve_column(fieldnames, requested_id_column, path)
    _check_unique_ids(rows, id_column, path)
    parameter_columns = [field for field in fieldnames if field != id_column]
    if not parameter_columns:
        raise ParameterProcessingError("{} 中没有参数列".format(path))
    return fieldnames, rows, id_column, parameter_columns


def _parse_baseline_overrides(raw_overrides):
    parsed = {}
    for name, value in raw_overrides.items():
        # 使用 Mongo 参数键，使键名末尾带不带 _1D 都能匹配。
        key = _mongo_parameter_key(name)
        parsed[key] = _to_decimal(value, "BASE_VALUE_OVERRIDES[{}]".format(name))
    return parsed


def _effective_parameter_columns(rows, parameter_columns, path):
    """返回至少含有一个非 -9999 改变值的参数列。"""
    effective = []
    for column in parameter_columns:
        has_effective_value = False
        for row_number, row in enumerate(rows, start=2):
            value = _to_decimal(
                row[column],
                "{} 第 {} 行参数 {}".format(path, row_number, column),
            )
            if value != NODATA_VALUE:
                has_effective_value = True
        if has_effective_value:
            effective.append(column)
    return effective


def _resolve_output_columns(parameter_columns):
    """
    根据配置确定输出参数列及其输出列名。

    返回：[(原始列名, 输出列名), ...]，保持原始顺序。
    - 若 FILTER_OUTPUT_PARAMETERS 为 True，仅保留 OUTPUT_PARAMETER_NAMES 中的参数
    - 若 OUTPUT_PARAMETER_ALIASES 中存在对应别名，则使用别名作为输出列名
    """
    alias_lookup = {
        _mongo_parameter_key(name): alias
        for name, alias in OUTPUT_PARAMETER_ALIASES.items()
    }
    filter_keys = (
        {_mongo_parameter_key(name) for name in OUTPUT_PARAMETER_NAMES}
        if FILTER_OUTPUT_PARAMETERS
        else None
    )

    output_columns = []
    for column in parameter_columns:
        col_key = _mongo_parameter_key(column)
        if filter_keys is not None and col_key not in filter_keys:
            continue
        output_name = alias_lookup.get(col_key, column)
        output_columns.append((column, output_name))
    return output_columns


def validate_baseline_coverage(calibration_specs, baseline_overrides):
    """在连接 MongoDB 前先检查所有有效率定参数是否都有基准值。"""
    missing_messages = []
    for spec in calibration_specs:
        missing = []
        for column in spec["effective_columns"]:
            csv_key = _csv_parameter_key(column)
            override_key = _mongo_parameter_key(column)
            if (
                csv_key not in spec["baselines"]
                and override_key not in baseline_overrides
            ):
                missing.append(column)
        if missing:
            missing_messages.append(
                "{}: {}".format(spec["calibration_path"], ", ".join(missing))
            )

    if missing_messages:
        print(
            "以下参数含有效改变值，但在基准参数 CSV 中没有对应列：\n  - "
            + "\n  - ".join(missing_messages)
            + "\n请补充基准 CSV，或在代码顶部的 BASE_VALUE_OVERRIDES 中填写基准值。"
        )


def read_change_rules_from_mongodb(parameter_names):
    """从 PARAMETERS 集合读取参数的 CHANGE 字段。"""
    try:
        try:
            from pymongo import MongoClient
        except ImportError:
            raise ParameterProcessingError(
                "缺少 pymongo，请先执行：pip install pymongo"
            )

        client_kwargs = {
            "host": MONGO_IP,
            "port": int(MONGO_PORT),
            "serverSelectionTimeoutMS": MONGO_TIMEOUT_MS,
        }
        if MONGO_USERNAME is not None:
            client_kwargs["username"] = MONGO_USERNAME
            client_kwargs["password"] = MONGO_PASSWORD
        if MONGO_AUTH_SOURCE is not None:
            client_kwargs["authSource"] = MONGO_AUTH_SOURCE

        client = MongoClient(**client_kwargs)
        try:
            client.admin.command("ping")
            collection = client[MONGO_DATABASE][MONGO_COLLECTION]
            documents = collection.find({}, {"_id": 0, "NAME": 1, "CHANGE": 1})

            all_rules = {}
            for document in documents:
                if "NAME" not in document or "CHANGE" not in document:
                    continue
                key = _mongo_parameter_key(document["NAME"])
                rule = str(document["CHANGE"]).strip().upper()
                if rule not in ("AC", "RC", "VC", "NC"):
                    continue
                if key in all_rules and all_rules[key] != rule:
                    raise ParameterProcessingError(
                        "MongoDB 中参数 {!r} 存在冲突的 CHANGE：{} 和 {}".format(
                            document["NAME"], all_rules[key], rule
                        )
                    )
                all_rules[key] = rule
        finally:
            client.close()
    except ParameterProcessingError:
        raise
    except Exception as exc:
        raise ParameterProcessingError("连接或读取 MongoDB 失败：{}".format(exc))

    required_keys = {_mongo_parameter_key(name) for name in parameter_names}
    missing = sorted(
        [name for name in parameter_names if _mongo_parameter_key(name) not in all_rules],
        key=str.casefold,
    )
    if missing:
        raise ParameterProcessingError(
            "MongoDB 的 {}.{} 中未找到以下参数的有效 CHANGE(AC/RC/VC/NC)：{}".format(
                MONGO_DATABASE, MONGO_COLLECTION, ", ".join(missing)
            )
        )

    return {key: all_rules[key] for key in required_keys}


def calculate_final_value(baseline_value, change_value, rule):
    """计算单个最终参数值；-9999 始终原样保留。

    规则与 SEIMS C++ ParamInfo.cpp 一致：
        AC: 最终值 = 基准值 + 改变值
        RC: 最终值 = 基准值 * 改变值
        VC: 最终值 = 改变值（直接替换）
        NC: 最终值 = 基准值（不改变）
    """
    if change_value == NODATA_VALUE:
        return NODATA_VALUE
    if baseline_value == NODATA_VALUE:
        raise ParameterProcessingError("有效改变值不能与 -9999 基准值进行计算")
    if rule == "AC":
        return baseline_value + change_value
    if rule == "RC":
        return baseline_value * change_value
    if rule == "VC":
        return change_value
    if rule == "NC":
        return baseline_value
    raise ParameterProcessingError("不支持的 CHANGE 规则：{}".format(rule))


def write_final_parameter_csv(
    calibration_path,
    output_path,
    fieldnames,
    rows,
    id_column,
    parameter_columns,
    baselines,
    baseline_overrides,
    change_rules,
):
    """按 ID 对齐基准值，计算并原子写出最终参数 CSV。"""
    output_columns = _resolve_output_columns(parameter_columns)
    original_to_output = dict(output_columns)
    included_originals = set(original_to_output.keys())
    output_fieldnames = []
    for field in fieldnames:
        if field == id_column:
            output_fieldnames.append(field)
        elif field in included_originals:
            output_fieldnames.append(original_to_output[field])
        # 否则该参数被过滤掉，不输出

    output_rows = []

    for row_number, row in enumerate(rows, start=2):
        row_id = str(row[id_column]).strip()
        output_row = {id_column: row[id_column]}

        for original_column, output_name in output_columns:
            change_value = _to_decimal(
                row[original_column],
                "{} 第 {} 行参数 {}".format(
                    calibration_path, row_number, original_column
                ),
            )

            # -9999 表示该项无有效率定值，直接保留；因此不要求存在基准值和规则。
            if change_value == NODATA_VALUE:
                output_row[output_name] = _decimal_to_text(NODATA_VALUE)
                print("[{}={}] {}: 改变值={} -> 最终值={} (跳过)".format(
                    id_column, row_id, original_column,
                    _decimal_to_text(change_value), _decimal_to_text(NODATA_VALUE),
                ))
                continue

            csv_key = _csv_parameter_key(original_column)
            if csv_key in baselines:
                if row_id not in baselines[csv_key]:
                    raise ParameterProcessingError(
                        "参数 {} 的基准 CSV 中缺少 {}={} 的记录".format(
                            original_column, id_column, row_id
                        )
                    )
                baseline_value = baselines[csv_key][row_id]
            elif _mongo_parameter_key(original_column) in baseline_overrides:
                baseline_value = baseline_overrides[_mongo_parameter_key(original_column)]
            else:
                raise ParameterProcessingError(
                    "参数 {} 没有可用的基准值".format(original_column)
                )

            mongo_key = _mongo_parameter_key(original_column)
            if mongo_key not in change_rules:
                raise ParameterProcessingError(
                    "参数 {} 没有可用的 MongoDB CHANGE 规则".format(original_column)
                )

            rule = change_rules[mongo_key]
            final_value = calculate_final_value(
                baseline_value, change_value, rule
            )
            output_row[output_name] = _decimal_to_text(final_value)
            print("[{}={}] {}: 原始值={} 规则={} 改变值={} -> 最终值={}".format(
                id_column, row_id, original_column,
                _decimal_to_text(baseline_value), rule,
                _decimal_to_text(change_value), _decimal_to_text(final_value),
            ))

        output_rows.append(output_row)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    if output_path.exists() and not OVERWRITE_OUTPUTS:
        raise FileExistsError("输出文件已存在：{}".format(output_path))

    temporary_path = Path(str(output_path) + ".tmp")
    try:
        with temporary_path.open(
            "w", encoding=OUTPUT_ENCODING, newline=""
        ) as file_obj:
            writer = csv.DictWriter(
                file_obj,
                fieldnames=output_fieldnames,
                extrasaction="raise",
                lineterminator="\n",
            )
            writer.writeheader()
            writer.writerows(output_rows)
        os.replace(str(temporary_path), str(output_path))
    except Exception:
        if temporary_path.exists():
            temporary_path.unlink()
        raise


def generate_final_parameter_csvs():
    """主方法：读取、校验、计算并生成两份最终参数 CSV。"""
    baseline_overrides = _parse_baseline_overrides(BASE_VALUE_OVERRIDES)

    hand_baselines, _ = load_baseline_tables(
        [PARAM_GROUP1_CSV], requested_id_column="FID"
    )
    subbasin_baselines, _ = load_baseline_tables(
        [PARAM_GROUP2_CSV, PARAM_GROUP2_CH_CSV],
        requested_id_column="subbasin",
    )

    hand_fields, hand_rows, hand_id, hand_columns = load_calibration_table(
        CALIPARAM_CSV, requested_id_column="FID"
    )
    sub_fields, sub_rows, sub_id, sub_columns = load_calibration_table(
        CALIPARAM_SUB_CSV, requested_id_column="subbasin"
    )

    hand_effective = _effective_parameter_columns(
        hand_rows, hand_columns, CALIPARAM_CSV
    )
    sub_effective = _effective_parameter_columns(
        sub_rows, sub_columns, CALIPARAM_SUB_CSV
    )

    # 应用输出过滤：仅对配置中指定且有效的参数进行校验和读取规则
    hand_effective_filtered = [col for col, _ in _resolve_output_columns(hand_effective)]
    sub_effective_filtered = [col for col, _ in _resolve_output_columns(sub_effective)]

    calibration_specs = [
        {
            "calibration_path": CALIPARAM_CSV,
            "effective_columns": hand_effective_filtered,
            "baselines": hand_baselines,
        },
        {
            "calibration_path": CALIPARAM_SUB_CSV,
            "effective_columns": sub_effective_filtered,
            "baselines": subbasin_baselines,
        },
    ]
    validate_baseline_coverage(calibration_specs, baseline_overrides)

    required_parameter_names = sorted(
        set(hand_effective_filtered + sub_effective_filtered), key=str.casefold
    )
    change_rules = read_change_rules_from_mongodb(required_parameter_names)

    write_final_parameter_csv(
        calibration_path=CALIPARAM_SUB_CSV,
        output_path=SUBBASIN_OUTPUT_CSV,
        fieldnames=sub_fields,
        rows=sub_rows,
        id_column=sub_id,
        parameter_columns=sub_columns,
        baselines=subbasin_baselines,
        baseline_overrides=baseline_overrides,
        change_rules=change_rules,
    )
    write_final_parameter_csv(
        calibration_path=CALIPARAM_CSV,
        output_path=HAND_OUTPUT_CSV,
        fieldnames=hand_fields,
        rows=hand_rows,
        id_column=hand_id,
        parameter_columns=hand_columns,
        baselines=hand_baselines,
        baseline_overrides=baseline_overrides,
        change_rules=change_rules,
    )

    print("已生成：{}".format(SUBBASIN_OUTPUT_CSV))
    print("已生成：{}".format(HAND_OUTPUT_CSV))
    return SUBBASIN_OUTPUT_CSV, HAND_OUTPUT_CSV


if __name__ == "__main__":
    generate_final_parameter_csvs()
