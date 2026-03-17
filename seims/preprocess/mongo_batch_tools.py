import subprocess
import os
import logging

# 配置日志，方便查看导出过程和结果
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

def mongodb_restore(
    port="27017",
    host="127.0.0.1",
    databases=None,
    input_path=None

):
    """
    使用mongorestore命令恢复MongoDB数据库

    Args:
        port (str): MongoDB的端口号，默认27017
        host (str): MongoDB的主机地址，默认127.0.0.1
        db_mapping (dict): 数据库名映射字典，格式：{目标数据库名: 备份文件路径}
        backup_base_path (str): 备份文件的基础路径（可选，若db_mapping中已包含完整路径则无需传）

    Returns:
        bool: 所有数据库恢复成功返回True，否则返回False
    """
    # 校验必填参数

    # 记录恢复是否全部成功
    all_success = True

    # 遍历数据库映射，执行恢复命令
    for target_db in databases:
        # 如果传入了基础路径，拼接完整的备份文件路径
        backup_full_path = os.path.join(input_path, target_db)


        # 检查备份文件路径是否存在
        if not os.path.exists(backup_full_path):
            logging.error(f"备份文件路径不存在: {backup_full_path}")
            all_success = False
            continue

        logging.info(f"开始恢复数据库: {target_db}，备份文件路径: {backup_full_path}")

        # 构建mongorestore命令（兼容低版本参数格式，去掉等号）
        cmd = [
            "mongorestore",
            "--host", host,
            "--port", port,
            "-d", target_db,       # 修复：参数和值分开，去掉等号
            backup_full_path       # 备份文件路径
        ]

        try:
            # 执行命令并捕获输出（完全兼容Python 3.7以下版本）
            result = subprocess.run(
                cmd,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )

            logging.info(f"数据库 {target_db} 恢复成功")
            # 手动解码字节流为字符串
            stdout_msg = result.stdout.decode('utf-8', errors='ignore')
            logging.debug(f"命令输出: {stdout_msg}")

        except subprocess.CalledProcessError as e:
            logging.error(f"数据库 {target_db} 恢复失败")
            logging.error(f"错误码: {e.returncode}")
            # 手动解码错误信息
            stderr_msg = e.stderr.decode('utf-8', errors='ignore')
            logging.error(f"错误信息: {stderr_msg}")
            all_success = False
        except Exception as e:
            logging.error(f"恢复数据库 {target_db} 时发生未知错误: {str(e)}")
            all_success = False

    return all_success

def mongodb_dump(
    host="172.21.124.127:27019",
    databases=None,
    output_path=None
):
    """
    使用mongodump命令备份MongoDB数据库

    Args:
        host (str): MongoDB的主机和端口，格式为 host:port
        databases (list): 需要备份的数据库名称列表
        output_path (str): 备份文件的完整输出路径

    Returns:
        bool: 所有数据库备份成功返回True，否则返回False
    """
    # 校验必填参数
    if databases is None or len(databases) == 0:
        logging.error("数据库列表不能为空！")
        return False

    if output_path is None:
        logging.error("输出路径不能为空！")
        return False

    # 确保输出目录存在
    try:
        os.makedirs(output_path, exist_ok=True)
        logging.info(f"输出目录已准备好: {output_path}")
    except Exception as e:
        logging.error(f"创建输出目录失败: {e}")
        return False

    # 记录备份是否全部成功
    all_success = True

    # 遍历数据库列表，执行备份命令
    for db_name in databases:
        logging.info(f"开始备份数据库: {db_name}")

        # 构建mongodump命令（修复：参数和值不能带等号分开）
        cmd = [
            "mongodump",
            "-h", host,
            "--db", db_name,
            "--out", output_path
        ]

        try:
            # 执行命令并捕获输出（完全兼容Python 3.7以下版本）
            # 移除text参数，手动处理字节流解码
            result = subprocess.run(
                cmd,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )

            logging.info(f"数据库 {db_name} 备份成功")
            # 手动解码字节流为字符串
            stdout_msg = result.stdout.decode('utf-8', errors='ignore')
            logging.debug(f"命令输出: {stdout_msg}")

        except subprocess.CalledProcessError as e:
            logging.error(f"数据库 {db_name} 备份失败")
            logging.error(f"错误码: {e.returncode}")
            # 手动解码错误信息
            stderr_msg = e.stderr.decode('utf-8', errors='ignore')
            logging.error(f"错误信息: {stderr_msg}")
            all_success = False
        except Exception as e:
            logging.error(f"备份数据库 {db_name} 时发生未知错误: {str(e)}")
            all_success = False

    return all_success


# 测试调用示例
if __name__ == "__main__":
    BASINs = [
        # "US_1",
        "US_2",
        "US_3",
        "US_4",
        "US_5",
        "US_6",
        # "US_7",
        # "US_8",
        # "US_9",
        "US_10",
        "US_11",
        "US_12",
        # "US_13",
        "US_14",
        "US_15",
        "US_16",
        "US_17",
        "US_18",
    ]
    base_path = r'G:\program\seims\SEIMS_HAND\data\USA_Small_Watersheds\SQL'
    # host = '172.21.124.127:27017'
    host = '127.0.0.1:27017'

    # 遍历每个流域执行备份
    for basin in BASINs:
        logging.info(f"========== 开始备份流域 {basin} 的数据库 ==========")
        # 拼接每个流域对应的数据库名
        longterm_model_db_name = basin + '_longterm_model'
        hydro_climate_db_name = basin + '_HydroClimate'
        scenario_db_name = basin + '_Scenario'
        # 拼接输出路径
        out_path = os.path.join(base_path, basin, 'SQL')

        # 执行备份
        success = mongodb_dump(
            host=host,
            databases=[longterm_model_db_name, hydro_climate_db_name, scenario_db_name],
            output_path=out_path,
        )

        if success:
            logging.info(f"流域 {basin} 的数据库备份完成！")
        else:
            logging.error(f"流域 {basin} 的部分或全部数据库备份失败！")

        input_path = out_path
        # success = mongodb_restore(
        #     port="27017",
        #     host="127.0.0.1",  # 若需要指定其他IP，改为对应的地址即可
        #     databases=[longterm_model_db_name, hydro_climate_db_name, scenario_db_name],
        #     input_path=input_path,
        # )
        # if success:
        #     logging.info(f"流域 {basin} 的数据库导入完成！")
        # else:
        #     logging.error(f"流域 {basin} 的部分或全部数据库导入失败！")
    logging.info("========== 所有流域数据库备份任务执行完毕 ==========")
