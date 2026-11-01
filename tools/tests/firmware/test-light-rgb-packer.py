#!/usr/bin/env python3
"""Exercise the actual GET JSON packer with real cJSON and property codec code.

Every cJSON allocation point is failed in turn. Source property ownership,
complete output, unsigned DID formatting and cleanup are checked without a
board, credentials, network or ESP32 build. Requires a POSIX host C compiler.
"""

import argparse
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile


QM_HEADER = r"""
#ifndef GET_PACK_HOST_QM_H
#define GET_PACK_HOST_QM_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
typedef unsigned char bool_t;
typedef int qm_err_t;
#define QM_EOK 0
#define QM_EIO 5
#define QM_EINVAL 22
#define QM_ENOMEM 12
#define QM_ENOSYS 38
#define QM_LOGI(...) ((void)0)
#define QM_LOGW(...) ((void)0)
#define QM_LOGE(...) ((void)0)
#define QM_LOGD(...) ((void)0)
#define qm_snprintf snprintf
#define cpu_to_be16(v) htons(v)
#define cpu_to_be32(v) htonl(v)
#define be16_to_cpu(v) ntohs(v)
#define be32_to_cpu(v) ntohl(v)
void *get_host_malloc(size_t size);
void get_host_free(void *pointer);
#define qm_malloc get_host_malloc
#define qm_free get_host_free
#endif
"""

PREAMBLE = r"""
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "qm.h"
#include "qm_spec_api.h"
#include "qm_spec_core.h"
#include "cJSON.h"
#define SPEC_JSON_KEY_DID "did"
#define SPEC_JSON_KEY_SIID "siid"
#define SPEC_JSON_KEY_PIID "piid"
#define SPEC_JSON_KEY_CODE "code"
#define SPEC_JSON_KEY_VALUE "value"

static int allocation_calls, live_allocations;
static int fail_allocation = -1, failed_allocations;
static int add_calls, fail_add = -1;

void *get_host_malloc(size_t size)
{
    int call = allocation_calls++;
    if (call == fail_allocation) { ++failed_allocations; return NULL; }
    void *result = malloc(size);
    if (result) ++live_allocations;
    return result;
}

void get_host_free(void *pointer)
{
    if (pointer) { assert(live_allocations > 0); --live_allocations; free(pointer); }
}

static cJSON_bool checked_add_item(cJSON *array, cJSON *item)
{
    if (add_calls++ == fail_add) return 0;
    return cJSON_AddItemToArray(array, item);
}

/* AddItemToArray normally allocates nothing. The explicit failure case tests
 * its ownership contract; all normal calls still execute real cJSON code. */
#define cJSON_AddItemToArray checked_add_item
"""

TESTS = r"""
#undef cJSON_AddItemToArray

static qm_spec_property_operation_t *new_operation(void)
{
    qm_spec_property_operation_t *operation = qm_spec_property_operation_creat();
    assert(operation);
    return operation;
}

static qm_spec_property_t *append(qm_spec_property_operation_t *operation, int piid)
{
    qm_spec_property_t *property = qm_spec_property_creat();
    assert(property);
    property->did = UINT32_MAX;
    assert(qm_spec_property_pack_xiid(property, 4, (uint16_t)piid) == QM_EOK);
    assert(qm_spec_property_add(operation, property) == QM_EOK);
    return property;
}

static qm_spec_property_operation_t *rgb_operation(void)
{
    qm_spec_property_operation_t *operation = new_operation();
    assert(qm_spec_property_pack_bool(append(operation, 1), 1) == QM_EOK);
    assert(qm_spec_property_pack_uint8(append(operation, 2), 50) == QM_EOK);
    assert(qm_spec_property_pack_uint32(append(operation, 3), 0xABCDEF) == QM_EOK);
    qm_spec_property_t *error = append(operation, 99);
    assert(qm_spec_property_pack_number(error, 1234567) == QM_EOK);
    error->code = -QM_ENOSYS; /* A stale value must never survive an error. */
    return operation;
}

static qm_spec_property_operation_t *typed_operation(void)
{
    qm_spec_property_operation_t *operation = new_operation();
    assert(qm_spec_property_pack_bool(append(operation, 1), 0) == QM_EOK);
    assert(qm_spec_property_pack_number(append(operation, 2), -54321) == QM_EOK);
    assert(qm_spec_property_pack_int8(append(operation, 3), -100) == QM_EOK);
    assert(qm_spec_property_pack_uint8(append(operation, 4), 250) == QM_EOK);
    assert(qm_spec_property_pack_int16(append(operation, 5), -30000) == QM_EOK);
    assert(qm_spec_property_pack_uint16(append(operation, 6), 60000) == QM_EOK);
    assert(qm_spec_property_pack_int32(append(operation, 7), INT32_MIN) == QM_EOK);
    assert(qm_spec_property_pack_uint32(append(operation, 8), UINT32_MAX) == QM_EOK);
    char long_text[4097];
    memset(long_text, 'x', sizeof(long_text) - 1); long_text[sizeof(long_text) - 1] = '\0';
    assert(qm_spec_property_pack_string(append(operation, 9), long_text,
                                       (int)strlen(long_text)) == QM_EOK);
    qm_spec_property_t *error = append(operation, 10);
    assert(qm_spec_property_pack_string(error, "do-not-return-this", 18) == QM_EOK);
    error->code = -QM_EIO;
    return operation;
}

static qm_spec_data_info_t info_for(qm_spec_property_operation_t *operation)
{
    qm_spec_data_info_t info = {0};
    info.operation.property_operation = operation;
    return info;
}

static void free_output(qm_spec_data_info_t *info)
{
    if (info->updata) cJSON_free(info->updata);
    info->updata = NULL; info->updata_len = 0;
}

static cJSON *parse_output(const qm_spec_data_info_t *info, int expected)
{
    assert(info->updata && info->updata_len == (int)strlen((char *)info->updata));
    cJSON *root = cJSON_Parse((char *)info->updata);
    assert(root && cJSON_IsArray(root) && cJSON_GetArraySize(root) == expected);
    for (int i = 0; i < expected; ++i) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        cJSON *did = cJSON_GetObjectItemCaseSensitive(item, "did");
        assert(cJSON_IsString(did) && strcmp(did->valuestring, "4294967295") == 0);
        assert(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(item, "siid")));
        assert(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(item, "piid")));
        assert(cJSON_IsNumber(cJSON_GetObjectItemCaseSensitive(item, "code")));
    }
    return root;
}

static void test_values(void)
{
    qm_spec_property_operation_t *operation = rgb_operation();
    qm_spec_data_info_t info = info_for(operation);
    assert(qm_spec_json_cloud_get_pack(&info) == QM_EOK);
    cJSON *root = parse_output(&info, 4);
    assert(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 0), "value")));
    assert(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 1), "value")->valuedouble == 50);
    assert(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 2), "value")->valuedouble == 0xABCDEF);
    cJSON *error = cJSON_GetArrayItem(root, 3);
    assert(cJSON_GetObjectItemCaseSensitive(error, "code")->valueint == -QM_ENOSYS);
    assert(cJSON_GetObjectItemCaseSensitive(error, "value") == NULL);
    cJSON_Delete(root); free_output(&info); qm_spec_property_operation_delete(operation);
    assert(live_allocations == 0);
    puts("PASS: unsigned DID, typed RGB and error-without-stale-value survive actual JSON packing");
}

static void test_formats(void)
{
    qm_spec_property_operation_t *operation = typed_operation();
    qm_spec_data_info_t info = info_for(operation);
    assert(qm_spec_json_cloud_get_pack(&info) == QM_EOK);
    cJSON *root = parse_output(&info, 10);
    assert(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 0), "value")));
    const double expected[] = {-54321, -100, 250, -30000, 60000, INT32_MIN, UINT32_MAX};
    for (int i = 0; i < 7; ++i) {
        cJSON *value = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, i + 1), "value");
        assert(cJSON_IsNumber(value) && value->valuedouble == expected[i]);
    }
    cJSON *text = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 8), "value");
    assert(cJSON_IsString(text) && strlen(text->valuestring) == 4096);
    assert(cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 9), "value") == NULL);
    cJSON_Delete(root); free_output(&info); qm_spec_property_operation_delete(operation);
    assert(live_allocations == 0);
    puts("PASS: every existing scalar/string format is complete, including growing print buffers");
}

static void test_allocations(int typed)
{
    qm_spec_property_operation_t *operation = typed ? typed_operation() : rgb_operation();
    int input_allocations = live_allocations;
    qm_spec_data_info_t info = info_for(operation);
    allocation_calls = 0;
    assert(qm_spec_json_cloud_get_pack(&info) == QM_EOK);
    int points = allocation_calls;
    free_output(&info);
    assert(live_allocations == input_allocations);
    qm_spec_property_t originals[16];
    int n = 0;
    for (qm_spec_property_t *p = operation->property; p; p = p->next) originals[n++] = *p;
    for (int failure = 0; failure < points; ++failure) {
        info = info_for(operation);
        allocation_calls = 0; failed_allocations = 0; fail_allocation = failure;
        int result = qm_spec_json_cloud_get_pack(&info);
        fail_allocation = -1;
        assert(failed_allocations == 1);
        assert(result == -QM_ENOMEM);
        assert(info.updata == NULL && info.updata_len == 0);
        assert(info.operation.property_operation == operation);
        assert(live_allocations == input_allocations);
        int index = 0;
        for (qm_spec_property_t *p = operation->property; p; p = p->next) {
            assert(memcmp(p, &originals[index++], sizeof(*p)) == 0);
        }
        assert(index == n);
    }
    info = info_for(operation);
    assert(qm_spec_json_cloud_get_pack(&info) == QM_EOK);
    cJSON *root = parse_output(&info, typed ? 10 : 4);
    cJSON_Delete(root); free_output(&info); qm_spec_property_operation_delete(operation);
    assert(live_allocations == 0);
    printf("PASS: all %d cJSON allocation points fail cleanly for %s; no partial output or leaks\n",
           points, typed ? "mixed formats/long text" : "RGB batch");
}

static void test_array_attach_failure(void)
{
    qm_spec_property_operation_t *operation = rgb_operation();
    int baseline = live_allocations;
    for (int index = 0; index < 4; ++index) {
        qm_spec_data_info_t info = info_for(operation);
        add_calls = 0; fail_add = index;
        assert(qm_spec_json_cloud_get_pack(&info) == -QM_ENOMEM);
        fail_add = -1;
        assert(info.updata == NULL && info.updata_len == 0);
        assert(live_allocations == baseline);
    }
    qm_spec_property_operation_delete(operation);
    assert(live_allocations == 0);
    puts("PASS: failed array attachment frees both unowned object and prior batch items");
}

static void test_invalid(void)
{
    assert(qm_spec_json_cloud_get_pack(NULL) == -QM_EINVAL);
    qm_spec_data_info_t info = {0};
    assert(qm_spec_json_cloud_get_pack(&info) == -QM_EINVAL);
    qm_spec_property_operation_t *operation = new_operation();
    info = info_for(operation);
    uint8_t caller_buffer[] = "owned by caller";
    info.updata = caller_buffer;
    info.updata_len = 15;
    assert(qm_spec_json_cloud_get_pack(&info) == -QM_EINVAL);
    assert(info.updata == caller_buffer && info.updata_len == 15);
    info = info_for(operation);
    assert(qm_spec_json_cloud_get_pack(&info) == QM_EOK);
    assert(strcmp((char *)info.updata, "[]") == 0);
    free_output(&info);
    qm_spec_property_t *property = append(operation, 1);
    assert(qm_spec_property_pack_float32(property, 1.0f) == QM_EOK);
    info = info_for(operation);
    assert(qm_spec_json_cloud_get_pack(&info) == -QM_ENOSYS);
    assert(info.updata == NULL && info.updata_len == 0);
    property->code = -QM_EIO;
    assert(qm_spec_json_cloud_get_pack(&info) == QM_EOK);
    cJSON *root = cJSON_Parse((char *)info.updata);
    assert(root && cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(root, 0), "value") == NULL);
    cJSON_Delete(root); free_output(&info); qm_spec_property_operation_delete(operation);
    assert(live_allocations == 0);
    puts("PASS: invalid inputs and unsupported successful values cannot publish partial success");
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    cJSON_Hooks hooks = {get_host_malloc, get_host_free};
    cJSON_InitHooks(&hooks);
    if (!strcmp(argv[1], "values")) test_values();
    else if (!strcmp(argv[1], "formats")) test_formats();
    else if (!strcmp(argv[1], "oom-rgb")) test_allocations(0);
    else if (!strcmp(argv[1], "oom-formats")) test_allocations(1);
    else if (!strcmp(argv[1], "array-failure")) test_array_attach_failure();
    else if (!strcmp(argv[1], "invalid")) test_invalid();
    else assert(!"unknown test");
    return 0;
}
"""


def extract_function(source, name):
    match = re.search(r"(?m)^static int " + re.escape(name) + r"\([^;]*?\)\s*\{", source)
    if not match:
        raise ValueError(f"Missing actual function {name}")
    position = match.end() - 1
    depth = 0
    mode = "code"
    while position < len(source):
        char, pair = source[position], source[position:position + 2]
        if mode == "line":
            if char == "\n":
                mode = "code"
        elif mode == "comment":
            if pair == "*/":
                mode = "code"
                position += 1
        elif mode in ('"', "'"):
            if char == "\\":
                position += 1
            elif char == mode:
                mode = "code"
        elif pair == "//":
            mode = "line"
            position += 1
        elif pair == "/*":
            mode = "comment"
            position += 1
        elif char in ('"', "'"):
            mode = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():position + 1]
        position += 1
    raise ValueError(f"Unclosed actual function {name}")


def main():
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-baseline", action="store_true",
                        help="Compile SDK HEAD and require the original OOM regression to fail")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CC", "cc"))
    if not compiler or not shutil.which(compiler[0]) or os.name == "nt":
        parser.error("Requires a POSIX host C compiler (set CC if necessary)")
    sdk = root / "src/firmware"
    spec = sdk / "qumios-dev/components/qm_spec"
    cjson = sdk / "qumios-dev/components/cjson"
    relative = "qumios-dev/components/qm_spec/src/qm_spec_core.c"
    source = subprocess.check_output(["git", "show", f"HEAD:{relative}"], cwd=sdk).decode() \
        if args.check_baseline else (sdk / relative).read_text()
    actual = extract_function(source, "qm_spec_json_cloud_get_pack")
    with tempfile.TemporaryDirectory(prefix="rgb-get-packer-test-") as folder:
        temp = Path(folder)
        (temp / "qm.h").write_text(QM_HEADER)
        for name in ("qm_types.h", "qm_log.h"):
            (temp / name).write_text('#include "qm.h"\n')
        for name in ("qm_spec_api.h", "qm_spec_core.h", "qm_spec_log.h"):
            shutil.copyfile(spec / "include" / name, temp / name)
        shutil.copyfile(spec / "src/qm_spec_api.c", temp / "qm_spec_api.c")
        fixture = temp / "test_packer.c"
        fixture.write_text(PREAMBLE + "\n" + actual + "\n" + TESTS)
        binary = temp / "test-packer"
        command = compiler + ["-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(temp),
                              "-I", str(cjson), str(fixture), str(temp / "qm_spec_api.c"),
                              str(cjson / "cJSON.c"), "-lm", "-o", str(binary)]
        subprocess.run(command, check=True)
        if args.check_baseline:
            result = subprocess.run([str(binary), "oom-rgb"], capture_output=True, text=True, timeout=20)
            if result.returncode == 0:
                raise SystemExit("SDK HEAD unexpectedly passed the allocation regression")
            print("EXPECTED FAILURE (SDK HEAD): allocation failure produced partial GET output")
        else:
            for case in ("values", "formats", "oom-rgb", "oom-formats", "array-failure", "invalid"):
                subprocess.run([str(binary), case], check=True, timeout=20, cwd=temp)


if __name__ == "__main__":
    main()
