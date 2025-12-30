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

// プラットフォーム依存関数の実装
void platform_print_ch(NB_I8 ch)
{
    if (output_pos < BUF_SIZE - 1) {
        output_buffer[output_pos++] = ch;
        output_buffer[output_pos] = '\0';
    }
    js_output_char(ch);
}

static FILE *fp = NULL;

NB_BOOL platform_fopen(const NB_I8 *name, NB_BOOL write_mode)
{
    // WASM環境ではファイルI/Oは非対応
    return NB_FALSE;
}

void platform_fclose()
{
    // WASM環境ではファイルI/Oは非対応
}

static NB_I8 _buf[256] = "";
NB_BOOL platform_fread(NB_I8 **buf, NB_SIZE *size)
{
    // WASM環境ではファイルI/Oは非対応
    return NB_FALSE;
}

NB_BOOL platform_fwrite(NB_LINE_NUM num, NB_I8 *buf, NB_SIZE size)
{
    // WASM環境ではファイルI/Oは非対応
    return NB_FALSE;
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
