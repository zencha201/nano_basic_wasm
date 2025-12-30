#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "nano_basic.h"

#define CODE_SIZE (128)
#define VALUE_SIZE (24 * 3)
#define STACK_SIZE (16)
#define BUF_SIZE (1024)

static NB_I8 memory[CODE_SIZE + VALUE_SIZE + STACK_SIZE] = {'\0'};
static NB_I8 buf[BUF_SIZE] = {'\0'};
static NB_I8 output_buffer[BUF_SIZE] = {'\0'};
static NB_SIZE output_pos = 0;

// JavaScript から呼び出される関数を定義
EM_JS(void, js_output_char, (char ch), {
    if (typeof Module.onOutput === 'function') {
        Module.onOutput(String.fromCharCode(ch));
    }
});

EM_JS(void, js_output_flush, (), {
    if (typeof Module.onFlush === 'function') {
        Module.onFlush();
    }
});

// ローカルストレージ操作用のJavaScript関数
EM_JS(char*, js_localstorage_get, (const char* key), {
    var keyStr = UTF8ToString(key);
    var value = localStorage.getItem(keyStr);
    if (value === null) {
        return null;
    }
    var lengthBytes = lengthBytesUTF8(value) + 1;
    var strPtr = _malloc(lengthBytes);
    stringToUTF8(value, strPtr, lengthBytes);
    return strPtr;
});

EM_JS(void, js_localstorage_set, (const char* key, const char* value), {
    var keyStr = UTF8ToString(key);
    var valueStr = UTF8ToString(value);
    localStorage.setItem(keyStr, valueStr);
});

EM_JS(void, js_free_string, (char* ptr), {
    _free(ptr);
});

// プラットフォーム依存関数の実装
static void platfom_print_ch(NB_I8 ch)
{
    if (output_pos < BUF_SIZE - 1) {
        output_buffer[output_pos++] = ch;
        output_buffer[output_pos] = '\0';
    }
    js_output_char(ch);
}

// ファイルI/O用のバッファとステート管理
#define FILE_BUFFER_SIZE (4096)
static NB_I8 file_buffer[FILE_BUFFER_SIZE] = {'\0'};
static NB_I8 file_name[256] = {'\0'};
static NB_I8 *file_read_ptr = NULL;
static NB_SIZE file_write_pos = 0;
static NB_BOOL file_is_write_mode = NB_FALSE;
static NB_BOOL file_is_open = NB_FALSE;

static NB_I8 _buf[256] = "";

static NB_BOOL platform_fopen(const NB_I8 *name, NB_BOOL write_mode)
{
    if (file_is_open) {
        return NB_FALSE; // 既にファイルが開かれている
    }
    
    // ファイル名を保存
    strncpy(file_name, name, sizeof(file_name) - 1);
    file_name[sizeof(file_name) - 1] = '\0';
    
    file_is_write_mode = write_mode;
    file_is_open = NB_TRUE;
    
    if (write_mode) {
        // 書き込みモード: バッファを初期化
        file_buffer[0] = '\0';
        file_write_pos = 0;
    } else {
        // 読み込みモード: ローカルストレージからデータを取得
        char *data = js_localstorage_get(name);
        if (data == NULL) {
            // データが存在しない場合は空文字列として扱う
            file_buffer[0] = '\0';
            file_read_ptr = file_buffer;
        } else {
            // データをバッファにコピー
            strncpy(file_buffer, data, FILE_BUFFER_SIZE - 1);
            file_buffer[FILE_BUFFER_SIZE - 1] = '\0';
            js_free_string(data);
            file_read_ptr = file_buffer;
        }
    }
    
    return NB_TRUE;
}

static void platform_fclose()
{
    if (!file_is_open) {
        return;
    }
    
    if (file_is_write_mode) {
        // 書き込みモードの場合、ローカルストレージに保存
        js_localstorage_set(file_name, file_buffer);
    }
    
    file_is_open = NB_FALSE;
    file_read_ptr = NULL;
    file_write_pos = 0;
}

static NB_BOOL platform_fread(NB_I8 **buf, NB_SIZE *size)
{
    if (!file_is_open || file_is_write_mode) {
        return NB_FALSE;
    }
    
    if (file_read_ptr == NULL || *file_read_ptr == '\0') {
        return NB_FALSE; // EOF
    }
    
    // 1行読み込み（改行文字まで、または文字列終端まで）
    NB_SIZE i = 0;
    while (file_read_ptr[i] != '\0' && file_read_ptr[i] != '\n' && i < 255) {
        _buf[i] = file_read_ptr[i];
        i++;
    }
    _buf[i] = '\0';
    
    // ポインタを次の行に進める
    if (file_read_ptr[i] == '\n') {
        file_read_ptr += i + 1; // 改行文字の次へ
    } else {
        file_read_ptr += i; // 文字列終端へ
    }
    
    *buf = _buf;
    *size = i;
    
    return NB_TRUE;
}

static NB_BOOL platform_fwrite(NB_LINE_NUM num, NB_I8 *buf, NB_SIZE size)
{
    if (!file_is_open || !file_is_write_mode) {
        return NB_FALSE;
    }
    
    // 行番号を文字列化
    NB_I8 num_str[16];
    int num_len = sprintf(num_str, "%d ", (int)num);
    
    // bufの実際の長さを取得（ヌル終端文字列として扱う）
    NB_SIZE buf_len = strlen(buf);
    
    // バッファに余裕があるかチェック（行番号 + スペース + 内容 + 改行 + 終端）
    if (file_write_pos + num_len + buf_len + 2 >= FILE_BUFFER_SIZE) {
        return NB_FALSE; // バッファオーバーフロー
    }
    
    // 行番号とスペースを書き込み
    memcpy(file_buffer + file_write_pos, num_str, num_len);
    file_write_pos += num_len;
    
    // 内容を書き込み（ヌル終端文字列として）
    memcpy(file_buffer + file_write_pos, buf, buf_len);
    file_write_pos += buf_len;
    
    // 改行を追加
    file_buffer[file_write_pos++] = '\n';
    file_buffer[file_write_pos] = '\0';
    
    return NB_TRUE;
}

NB_VALUE platform_import(NB_VALUE num)
{
    // デフォルト実装
    return num * 2;
}

void platform_export(NB_VALUE num, NB_VALUE value)
{
    // デフォルト実装
}

// カスタムコマンド: IMPORT
NB_RESULT command_import(NB_LINE_NUM *num, const NB_I8 *code, NB_SIZE size, NB_SIZE *pos, NB_STATE *state)
{
    NB_VALUE import_num = 0;
    NB_SIZE index = 0;
    NB_VALUE value = 0;

    IF_ERROR_EXIT(calc(code, size, pos, &import_num));
    IF_FALSE_EXIT(code[*pos] == NB_CODE_STR_SEPALATE_PARAMETER);
    *pos = *pos + 1;
    calc_get_variable_pos(code, size, pos, &index);
    value = platform_import(import_num);
    IF_ERROR_EXIT(memory_variable_set(index, value));

    return NB_RESULT_SUCCESS;
}

// カスタムコマンド: EXPORT
NB_RESULT command_export(NB_LINE_NUM *num, const NB_I8 *code, NB_SIZE size, NB_SIZE *pos, NB_STATE *state)
{
    NB_VALUE export_num = 0;
    NB_VALUE value = 0;

    IF_ERROR_EXIT(calc(code, size, pos, &export_num));
    IF_FALSE_EXIT(code[*pos] == NB_CODE_STR_SEPALATE_PARAMETER);
    *pos = *pos + 1;
    IF_ERROR_EXIT(calc(code, size, pos, &value));
    platform_export(export_num, value);
    
    return NB_RESULT_SUCCESS;
}

// JavaScriptから呼び出される初期化関数
EMSCRIPTEN_KEEPALIVE
void nano_basic_wasm_init()
{
    output_pos = 0;
    output_buffer[0] = '\0';
    
    nano_basic_init(memory, CODE_SIZE, VALUE_SIZE, STACK_SIZE);
    nano_basic_set_platform_print_ch(platfom_print_ch);
    nano_basic_set_platform_fopen(platform_fopen);
    nano_basic_set_platform_fclose(platform_fclose);
    nano_basic_set_platform_fread(platform_fread);
    nano_basic_set_platform_fwrite(platform_fwrite);
    nano_basic_add_command("IMP", command_import);
    nano_basic_add_command("EXP", command_export);
}

// JavaScriptから呼び出されるコード実行関数
EMSCRIPTEN_KEEPALIVE
int nano_basic_wasm_exec(const char *input)
{
    NB_STATE state = NB_STATE_REPL;
    NB_SIZE size = strlen(input) + 1; // 終端文字分を含むサイズ
    
    if (size >= BUF_SIZE) {
        return -1; // エラー: 入力が長すぎる
    }
    
    strcpy(buf, input);
    output_pos = 0;
    output_buffer[0] = '\0';
    
    state = nano_basic_proc(state, buf, size);
    js_output_flush();
    
    return (int)state;
}

// INPUT文で数値入力待ちの後に処理を継続する関数
EMSCRIPTEN_KEEPALIVE
int nano_basic_wasm_continue_input(int value)
{
    NB_STATE state = NB_STATE_INPUT_NUMBER;
    
    // 入力値をセット
    nano_basic_set_input_value((NB_VALUE)value);
    
    // 空の入力で処理を継続
    buf[0] = '\0';
    state = nano_basic_proc(state, buf, 1);
    js_output_flush();
    
    return (int)state;
}

// RUN MODE継続実行用の関数
EMSCRIPTEN_KEEPALIVE
int nano_basic_wasm_continue()
{
    NB_STATE state = NB_STATE_RUN_MODE;
    
    // 空の入力でnano_basic_procを呼び出して継続実行
    buf[0] = '\0';
    state = nano_basic_proc(state, buf, 1);
    js_output_flush();
    
    return (int)state;
}

// JavaScriptから呼び出される入力値設定関数
EMSCRIPTEN_KEEPALIVE
void nano_basic_wasm_set_input(int value)
{
    nano_basic_set_input_value((NB_VALUE)value);
}

// 出力バッファを取得する関数
EMSCRIPTEN_KEEPALIVE
const char* nano_basic_wasm_get_output()
{
    return output_buffer;
}

// 出力バッファをクリアする関数
EMSCRIPTEN_KEEPALIVE
void nano_basic_wasm_clear_output()
{
    output_pos = 0;
    output_buffer[0] = '\0';
}
