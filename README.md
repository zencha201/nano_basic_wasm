# Nano BASIC WASM

Nano BASIC インタプリタの WebAssembly 版です。

## 概要

このプロジェクトは [nano_basic](https://github.com/zencha201/nano_basic) ライブラリを WebAssembly にコンパイルし、Web ブラウザ上で BASIC プログラムを実行できるようにしたものです。

## ビルド方法

### 前提条件

- Emscripten SDK がインストールされていること

Emscripten のインストール方法:
```bash
# Emscripten SDK を取得
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# 最新版をインストール
./emsdk install latest
./emsdk activate latest

# 環境変数を設定
source ./emsdk_env.sh
```

### ビルド

```bash
# サブモジュールを初期化（まだの場合）
git submodule update --init --recursive

# ビルド
make

# デバッグビルド
make DEBUG=1
```

ビルドが成功すると、`dist/` ディレクトリに以下のファイルが生成されます：
- `nano_basic.js` - JavaScript ラッパー
- `nano_basic.wasm` - WebAssembly モジュール

## 実行方法

### ローカルサーバーで実行

```bash
# Python 3 の場合
python3 -m http.server 8000

# ブラウザで開く
# http://localhost:8000
```

## 使い方

Web インターフェースでは、BASIC コマンドを入力して実行できます。

### 基本的な使用例

```basic
10 PRINT "HELLO, WORLD!"
20 END
RUN
```

```basic
10 LET A=10
20 LET B=20
30 PRINT A+B
40 END
RUN
```

```basic
10 FOR I=1 TO 10
20 PRINT I
30 NEXT I
40 END
RUN
```

## API

JavaScript から直接 WASM モジュールを呼び出すこともできます。

```javascript
// モジュールを初期化
Module._nano_basic_wasm_init();

// コマンドを実行
const input = "10 PRINT \"HELLO\"";
const result = Module._nano_basic_wasm_exec(
    Module.allocateUTF8(input)
);

// 出力を取得
const output = Module.UTF8ToString(
    Module._nano_basic_wasm_get_output()
);
```

## ライセンス

MIT License - 詳細は LICENSE ファイルを参照してください。

## 参考

- [nano_basic](https://github.com/zencha201/nano_basic) - オリジナルの C 実装
- [Emscripten](https://emscripten.org/) - WebAssembly コンパイラ
