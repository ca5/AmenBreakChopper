---
title: AmenBreakChopper Documentation
---

# AmenBreakChopper

## 概要

AmenBreakChopperは、入力された音声をBPMに同期したディレイで再生するビートチョッパー・エフェクトです。
JUCEフレームワークを使用して開発されたオーディオプラグインで、DelayTimeパラメータはMIDIノートによってコントロール可能です。また、OSCやMIDI CCによる詳細なコントロールにも対応しています。

## インストール方法

配布されるインストーラーやファイルを使用して、お使いの環境にプラグインをインストールしてください。
現在は以下の形式に対応予定です。

*   iOS (AUv3, Standalone)
*   macOS / Windows (VST3)

## 基本的な使い方

1.  お使いのDAWで、AmenBreakChopperをオーディオトラックまたはインストゥルメントトラックにインサートします。
2.  トラックにオーディオ信号を入力すると、エフェクトがかかります。
3.  プラグインのUIはまだ実装されていませんが、DAWの汎用UIからパラメータを操作できます。

## パラメータの詳細

### Main Parameters
*   **Control Mode**: "Internal"と"OSC"のモードを切り替えます。
*   **Delay Time**: ディレイタイムを8分音符単位で設定します (0-15)。
*   **Sequence Position**: 現在のシーケンサーの再生位置を示します (0-15)。
*   **Note Sequence Position**: MIDIノートによって設定されたシーケンス位置を示します (0-15)。

### MIDI Settings
*   **MIDI In Channel**: MIDI入力チャンネルを設定します (0=Omni)。
*   **MIDI Out Channel**: プラグインが生成するMIDIノートの出力チャンネルを設定します (1-16)。

### OSC Settings
*   **OSC Send Port**: OSCメッセージを送信するポート番号です。
*   **OSC Receive Port**: OSCメッセージを受信するポート番号です。

### MIDI CC Settings
*   **MIDI CC Seq Reset**: シーケンスをリセットするためのMIDI CC番号を設定します。
*   **Seq Reset Mode**: CCメッセージを受信した際の動作モードを設定します ("Any", "Gate-On", "Gate-Off")。
*   **MIDI CC Timer Reset**: タイマーをリセットするためのMIDI CC番号を設定します。
*   **Timer Reset Mode**: CCメッセージを受信した際の動作モードを設定します。
*   **MIDI CC Soft Reset**: ソフトリセットを行うためのMIDI CC番号を設定します。
*   **Soft Reset Mode**: CCメッセージを受信した際の動作モードを設定します。

### Delay Adjustment
*   **Delay Adjust**: シーケンサーのタイミングを細かく調整します (-4096 to 4096 samples)。
*   **MIDI CC Delay Adjust Fwd**: Delay Adjustを増加させるためのMIDI CC番号を設定します。
*   **MIDI CC Delay Adjust Bwd**: Delay Adjustを減少させるためのMIDI CC番号を設定します。
*   **Delay Adjust CC Step**: CCでDelay Adjustを操作する際のステップサイズを設定します。

## MIDIコントロール

### ディレイタイムのコントロール
MIDIノートナンバー 0-15 を受信すると、その値が `Note Sequence Position` に設定されます。
シーケンサーが次の8分音符のタイミングに到達した際、現在の `Sequence Position` と `Note Sequence Position` の差分から新しい `Delay Time` が計算されます。

### シーケンサーのリセット
*   **Sequence Reset**: `MIDI CC Seq Reset` で設定したCCを受信すると、次の8分音符のタイミングで `Delay Time` を0にし、`Note Sequence Position` を現在の `Sequence Position` に同期させます。
*   **Timer Reset**: `MIDI CC Timer Reset` で設定したCCを受信すると、シーケンサーのタイミングと再生位置をリセットします。DAWの再生位置に合わせてグリッドを補正します。
*   **Soft Reset**: `MIDI CC Soft Reset` で設定したCCを受信すると、`Sequence Position` と `Note Sequence Position` を0にリセットします。

## OSCコントロール

以下のOSCアドレスでプラグインをコントロールできます。

*   **/delayTime `(int)`**: `Delay Time` を設定します (0-15)。
*   **/sequenceReset**: シーケンスリセットをトリガーします。
*   **/timerReset**: タイマーリセットをトリガーします。
*   **/softReset**: ソフトリセットをトリガーします。
*   **/setNoteSequencePosition `(int)`**: `Note Sequence Position` を直接設定します (0-15)。

プラグインは以下のOSCメッセージを送信します。

*   **/sequencePosition `(int)`**: 現在の `Sequence Position` を送信します。
*   **/noteSequencePosition `(int)`**: 現在の `Note Sequence Position` を送信します。
