# Nano BASIC WASM

Nano BASIC インタプリタの WebAssembly 版です。

## 概要

このプロジェクトは [nano_basic](https://github.com/zencha201/nano_basic) ライブラリを WebAssembly にコンパイルし、Web ブラウザ上で BASIC プログラムを実行できるようにしたものです。

## 実行

[GitHub Pages上で実行](https://zencha201.github.io/nano_basic_wasm/)

## ビルド方法

### 前提条件

- Git
- Make
- Python 3（サーバー実行用）

Emscripten SDK は自動的にインストールされます。

### ビルド

```bash
# サブモジュールを初期化（まだの場合）
git submodule update --init --recursive

# ビルド（Emscripten がない場合は自動的にインストールされます）
make

# ビルドしてHTTPサーバーを起動
make run

# デバッグビルド
make DEBUG=1
```

**注意:** 初回ビルド時は Emscripten SDK のダウンロードとインストールが自動的に行われるため、時間がかかります。

ビルドが成功すると、`dist/` ディレクトリに以下のファイルが生成されます：
- `nano_basic.js` - JavaScript ラッパー
- `nano_basic.wasm` - WebAssembly モジュール

### Makefileコマンド

- `make` - ビルドのみ実行
- `make run` - ビルド後、ポート8000でHTTPサーバーを起動
- `make clean` - ビルド成果物を削除
- `make clean-all` - ビルド成果物とEmscripten SDKを削除

## 実行方法

### ローカルサーバーで実行

```bash
# Makeコマンドで実行（推奨）
make run

# または手動でサーバーを起動
python3 -m http.server 8000
```

ブラウザで `http://localhost:8000` を開いてください。

## 使い方

Web インターフェースでは、BASIC コマンドを入力して実行できます。

### 特徴

- **自動継続実行**: `RUN` コマンドでプログラムを実行すると、自動的に継続実行されます
- **リアルタイム出力**: プログラムの出力がリアルタイムでターミナルに表示されます
- **無限ループ対応**: タイマーベースの実行により、ブラウザがフリーズせずに無限ループも実行可能
- **ファイル保存機能**: `SAVE`/`LOAD` コマンドでプログラムをブラウザのローカルストレージに保存・読み込み可能

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

### プログラムの保存と読み込み

SAVE と LOAD コマンドを使用して、プログラムをブラウザのローカルストレージに保存・読み込みできます。

```basic
10 PRINT "HELLO"
20 END
SAVE MYPROG
```

```basic
LOAD MYPROG
LIST
RUN
```

**注意:**
- ファイル名はローカルストレージのキーとして使用されます
- データはブラウザのローカルストレージに保存されるため、ブラウザのデータをクリアすると削除されます
- 保存されたプログラムは同じブラウザ・同じドメインでのみアクセス可能です

### 無限ループの例

```basic
10 PRINT "HELLO"
20 GOTO 10
RUN
```

プログラムを停止するには、ページをリロードしてください。

## API

JavaScript から直接 WASM モジュールを呼び出すこともできます。

### 初期化と実行

```javascript
// モジュールを初期化
Module._nano_basic_wasm_init();

// コマンドを実行
const input = "10 PRINT \"HELLO\"";
const lengthBytes = Module.lengthBytesUTF8(input) + 1;
const ptr = Module._malloc(lengthBytes);
Module.stringToUTF8(input, ptr, lengthBytes);

const result = Module._nano_basic_wasm_exec(ptr);
Module._free(ptr);

// 出力を取得
const output = Module.UTF8ToString(
    Module._nano_basic_wasm_get_output()
);
```

### 利用可能な関数

- `_nano_basic_wasm_init()` - インタプリタを初期化
- `_nano_basic_wasm_exec(ptr)` - コマンドを実行（戻り値: NB_STATE）
- `_nano_basic_wasm_continue()` - RUN MODE継続実行
- `_nano_basic_wasm_continue_input(value)` - INPUT命令の入力値を設定して処理を継続（戻り値: NB_STATE）
- `_nano_basic_wasm_set_input(value)` - INPUT命令の入力値を設定
- `_nano_basic_wasm_get_output()` - 出力バッファを取得
- `_nano_basic_wasm_clear_output()` - 出力バッファをクリア

### 状態定数

```javascript
const NB_STATE_RUN_MODE = 0;      // 実行モード（継続実行が必要）
const NB_STATE_REPL = 1;          // 入力待ち
const NB_STATE_INPUT_NUMBER = 2;  // 数値入力待ち
const NB_STATE_END = 3;           // 終了
```

## ライセンス

MIT License - 詳細は LICENSE ファイルを参照してください。

## 参考

- [nano_basic](https://github.com/zencha201/nano_basic) - オリジナルの C 実装
- [Emscripten](https://emscripten.org/) - WebAssembly コンパイラ
