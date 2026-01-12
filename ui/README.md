# Amen Break Chopper - UI

Amen Break Chopper のユーザーインターフェース (UI) のソースコードです。
React + TypeScript + Vite で構築されており、ビルド成果物は JUCE プラグインの WebView に読み込まれます。

## 必要要件

*   Node.js (v18以上推奨)
*   npm

## セットアップ

依存関係をインストールします。

```bash
npm install
```

## 開発 (ローカル)

ローカルサーバーを立ち上げて UI を開発できます。
※ プラグインとの連携機能（波形表示など）は、ブラウザ単体では動作しませんが、レイアウトの確認などに利用できます。

```bash
npm run dev
```

## ビルド

プラグインに組み込むための本番用ビルドを作成します。
実行すると `dist` フォルダにファイルが出力されます。

```bash
npm run build
```

### JUCE プラグインへの組み込みについて

1.  `npm run build` を実行します。
2.  `dist` フォルダ内のファイル (`index.html`, `assets/xxx.js`, `assets/xxx.css`) が生成されます。
3.  Xcode で `AmenBreakChopper.jucer` からプロジェクトを開き、`dist` フォルダが "Folder Reference"（青いフォルダアイコン）として参照されていることを確認してください。
4.  iOS アプリをビルドすると、これらのファイルがアプリ内にコピーされ、WebView から読み込まれます。

> **注意**: ビルドごとに JS/CSS のファイル名（ハッシュ値）が変わる場合があります。
> その場合、`.jucer` ファイル内のファイル参照も更新する必要があります（自動化も検討中ですが、現在は手動更新またはフォルダ参照が必要です）。