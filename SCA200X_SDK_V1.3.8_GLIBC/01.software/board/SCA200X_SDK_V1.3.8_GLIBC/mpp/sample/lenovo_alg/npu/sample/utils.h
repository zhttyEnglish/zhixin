#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 返回码定义 */
#define SUCCESS 0
#define ERROR_HARDWARE_INFO -1
#define ERROR_ENCRYPTION -2
#define ERROR_VERIFICATION -3
#define ERROR_INVALID_PARAM -4
#define ERROR_FILE_NOT_FOUND -5
#define ERROR_MEMORY_ALLOC -6

/**
 * 生成License请求文件
 * 
 * @param public_key_path 公钥文件路径
 * @param output_dir 输出目录（NULL表示当前目录）
 * @return SUCCESS=0 表示成功，负数表示失败
 */


int generate_license_request(const char *public_key_path, const char *output_dir);

/**
 * 验证 License 文件
 * 
 * @param lic_file_path License 文件路径
 * @param public_key_path 公钥文件路径
 * @param software_version 软件版本
 * @param model_version 模型版本
 * @return SUCCESS=0 表示验证成功，负数表示失败
 */
int verify_license(const char *lic_file_path, const char *public_key_path, 
                   const char *software_version, const char *model_version);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H

