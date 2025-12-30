# Emscripten コンパイラ設定
EMSDK_DIR = ./emsdk
EMSDK_ENV = $(EMSDK_DIR)/emsdk_env.sh

# emsdk ディレクトリが存在する場合は source コマンドを設定
ifneq ($(wildcard $(EMSDK_ENV)),)
    EMCC_CMD = . $(EMSDK_ENV) && emcc
else
    EMCC_CMD = emcc
endif

EMCC = emcc
EMCFLAGS = -I./lib/nano_basic/include -I./lib/nano_basic/src
EMLDFLAGS = -s WASM=1 \
            -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","lengthBytesUTF8","stringToUTF8","UTF8ToString"]' \
            -s EXPORTED_FUNCTIONS='["_nano_basic_wasm_init","_nano_basic_wasm_exec","_nano_basic_wasm_set_input","_nano_basic_wasm_get_output","_nano_basic_wasm_clear_output","_malloc","_free"]' \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s MODULARIZE=1 \
            -s EXPORT_NAME='createNanoBasicModule'

# Emscripten のチェック
EMCC_EXISTS := $(shell command -v emcc 2> /dev/null)

# セットアップ完了フラグ（無限ループ防止用）
SETUP_DONE ?= 0

# デバッグモード
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    EMCFLAGS += -g -D__DEBUG__
    EMLDFLAGS += -g
else
    EMLDFLAGS += -O3
endif

# ソースファイル
LIB_SRCS = lib/nano_basic/src/calc.c \
           lib/nano_basic/src/command.c \
           lib/nano_basic/src/debug.c \
           lib/nano_basic/src/interpreter.c \
           lib/nano_basic/src/memory.c \
           lib/nano_basic/src/util.c

WASM_SRCS = src/nano_basic_wasm.c

# オブジェクトファイル
LIB_OBJS = $(LIB_SRCS:.c=.o)
WASM_OBJS = $(WASM_SRCS:.c=.o)

# ターゲット
OUTPUT_DIR = dist
WASM_OUTPUT = $(OUTPUT_DIR)/nano_basic.js

all: check-emscripten $(WASM_OUTPUT)

# Emscripten の存在チェックとセットアップ
check-emscripten:
ifndef EMCC_EXISTS
ifeq ($(SETUP_DONE), 0)
	@echo "Emscripten が見つかりません。セットアップを開始します..."
	@$(MAKE) setup-emscripten
	@echo "Emscripten のセットアップが完了しました。ビルドを続行します..."
	@bash -c '. $(EMSDK_ENV) && $(MAKE) SETUP_DONE=1'
	@exit 0
else
	@echo "エラー: Emscripten のセットアップ後も emcc が見つかりません。"
	@echo "手動で以下のコマンドを実行してください:"
	@echo "  source $(EMSDK_DIR)/emsdk_env.sh"
	@exit 1
endif
else
	@echo "Emscripten が見つかりました: $(EMCC_EXISTS)"
endif

# Emscripten のセットアップ
setup-emscripten:
	@if [ ! -d "$(EMSDK_DIR)" ]; then \
		echo "Emscripten SDK をクローンしています..."; \
		git clone https://github.com/emscripten-core/emsdk.git $(EMSDK_DIR); \
	fi
	@cd $(EMSDK_DIR) && ./emsdk install latest
	@cd $(EMSDK_DIR) && ./emsdk activate latest
	@echo ""
	@echo "==================================================================="
	@echo "Emscripten のインストールが完了しました！"
	@echo "==================================================================="

$(WASM_OUTPUT): $(LIB_OBJS) $(WASM_OBJS)
	@mkdir -p $(OUTPUT_DIR)
	$(EMCC_CMD) $(EMCFLAGS) $(EMLDFLAGS) -o $@ $(LIB_OBJS) $(WASM_OBJS)

%.o: %.c
	$(EMCC_CMD) $(EMCFLAGS) -c $< -o $@

clean:
	rm -rf $(LIB_OBJS) $(WASM_OBJS) $(OUTPUT_DIR)

clean-all: clean
	rm -rf $(EMSDK_DIR)

.PHONY: all check-emscripten setup-emscripten clean clean-all
