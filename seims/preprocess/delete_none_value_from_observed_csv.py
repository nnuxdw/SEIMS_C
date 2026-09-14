import csv


def remove_missing_value_rows(input_csv, output_csv):
    """
    保留前两行，从第三行开始删除 VALUE 为空的数据行。

    输出文件：
    1. 第一行强制设置为 #UTCTIME
    2. 只使用英文逗号分隔，不包含 TAB
    3. 所有行列数一致
    4. 使用无 BOM 的 UTF-8 编码
    5. 使用标准 CRLF 换行
    6. 可直接被原来的 read_data_items_from_txt 方法读取
    """

    def parse_input_line(line):
        """
        兼容：
        1. 标准逗号分隔
        2. TAB 分隔
        3. TAB 与逗号混合
        """
        line = line.strip("\r\n")

        if "\t" in line:
            # 如果包含 TAB，优先按照 TAB 拆分
            items = line.split("\t")

            # 清除字段两侧的空格和多余逗号
            items = [
                item.strip().strip(",").strip()
                for item in items
            ]
        else:
            # 使用 csv.reader 解析标准 CSV
            items = next(csv.reader([line]))

            items = [
                item.strip()
                for item in items
            ]

        # 删除行末多余分隔符产生的空字段
        while items and items[-1] == "":
            items.pop()

        return items

    # 使用 utf-8-sig 读取，可以自动清除输入文件中的 BOM
    with open(
        input_csv,
        "r",
        encoding="utf-8-sig",
        errors="ignore",
        newline=""
    ) as infile:
        lines = infile.readlines()

    if len(lines) < 2:
        raise ValueError("CSV 文件不足两行，无法识别表头。")

    # 第二行是真正的字段名
    column_header = parse_input_line(lines[1])

    column_header = [
        column.strip()
        for column in column_header
    ]

    if "VALUE" not in column_header:
        raise ValueError(
            "第二行中没有找到 VALUE 字段，当前字段为：%s"
            % column_header
        )

    column_count = len(column_header)
    value_index = column_header.index("VALUE")

    # 不再沿用原始第一行，直接强制设置为 #UTCTIME
    # 例如4列时，输出为：#UTCTIME,,,
    first_header = ["#UTCTIME"] + [""] * (column_count - 1)

    output_rows = [
        first_header,
        column_header
    ]

    removed_count = 0
    original_data_count = 0

    # 前两行不参与剔除，从第三行开始处理
    for line_number, line in enumerate(lines[2:], start=3):
        if not line.strip():
            continue

        original_data_count += 1
        row = parse_input_line(line)

        # 列数不足时，在末尾补空字段
        if len(row) < column_count:
            row.extend([""] * (column_count - len(row)))

        # 删除超过表头列数的行末空字段
        while len(row) > column_count and row[-1] == "":
            row.pop()

        # 如果仍超过表头列数，说明该行格式异常
        if len(row) > column_count:
            raise ValueError(
                "第 %d 行有 %d 列，但表头只有 %d 列，内容为：%s"
                % (line_number, len(row), column_count, row)
            )

        # 清理字段两侧空格
        row = [
            item.strip()
            for item in row
        ]

        # VALUE 为空时剔除
        if row[value_index] == "":
            removed_count += 1
            continue

        output_rows.append(row)

    # 注意：必须使用 utf-8，不能使用 utf-8-sig
    # 这样输出文件开头不会出现 BOM
    with open(
        output_csv,
        "w",
        encoding="utf-8",
        newline=""
    ) as outfile:
        writer = csv.writer(
            outfile,
            delimiter=",",
            quotechar='"',
            quoting=csv.QUOTE_MINIMAL,
            lineterminator="\r\n"
        )
        writer.writerows(output_rows)

    # 检查输出文件是否错误地包含 BOM
    with open(output_csv, "rb") as check_file:
        first_three_bytes = check_file.read(3)

    if first_three_bytes == b"\xef\xbb\xbf":
        raise RuntimeError(
            "输出文件仍然包含 UTF-8 BOM，SEIMS 可能无法识别 #UTCTIME。"
        )

    print("处理完成：%s" % input_csv)
    print("原始数据行数：%d" % original_data_count)
    print("剔除数据行数：%d" % removed_count)
    print("保留数据行数：%d" % (len(output_rows) - 2))
    print("输出文件：%s" % output_csv)
    print("时间格式：UTCTIME")
    print("文件编码：UTF-8（无 BOM）")
    print("")


if __name__ == "__main__":
    remove_missing_value_rows(
        input_csv=r"G:\program\seims\SEIMS_HAND\data\TPO\Q_100.csv",
        output_csv=r"G:\program\seims\SEIMS_HAND\data\TPO\observed_Q_100.csv"
    )

    remove_missing_value_rows(
        input_csv=r"G:\program\seims\SEIMS_HAND\data\TPO\Q_109.csv",
        output_csv=r"G:\program\seims\SEIMS_HAND\data\TPO\observed_Q_109.csv"
    )

    remove_missing_value_rows(
        input_csv=r"G:\program\seims\SEIMS_HAND\data\TPO\Q_127.csv",
        output_csv=r"G:\program\seims\SEIMS_HAND\data\TPO\observed_Q_127.csv"
    )
