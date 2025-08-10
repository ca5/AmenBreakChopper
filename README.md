# AmenBreakChopper

## 概要

JUCEフレームワークを使用して開発されたオーディオプラグイン。
入力された音声をBPMに同期したディレイで再生するビートチョッパー・エフェクト。
DelayTimeパラメータはMIDIノートによってコントロール可能。

## 開発ワークフロー

### Projucer

プロジェクトのビルド設定やファイル管理は、`AmenBreakChopper.jucer` ファイルをProjucerアプリケーションで開いて行います。
設定変更後は、必ず「Save Project and Open in IDE...」でXcodeプロジェクトを再生成してください。



## ビルドとインストール（macOS）

ビルド環境の問題により、DAWが古いプラグインを読み込む問題が頻発しました。
ビルド後の確実なテスト手順は以下の通りです：

1. Xcodeでプラグインをビルドする。
2. Xcodeの「Products」フォルダからビルドされた `.component` ファイルをFinderで表示する。
3. そのファイルを `/Library/Audio/Plug-Ins/Components/` にコピーし、既存のファイルを上書きする。
4. DAWを再起動してプラグインを読み込む。

## Gitワークフロー

このプロジェクトの`.jucer`ファイルは、Windows形式の改行コード（CRLF）を含んでいます。
コミット時に改行コードに関する `fatal` エラーが発生した場合は、コミットの直前に `git add --renormalize .` を実行してください。これにより、Gitが改行コードをリポジトリの標準形式に正規化し、エラーを解消します。
