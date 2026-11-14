# mcnote

Minecraft Bedrock Edition (Windows / GDK) 向けの、音ブロック自動演奏ツールです。
MIDIファイルを読み込んで、周りの音ブロックを順番に叩いて演奏します。

> **This is an unofficial tool. Not affiliated with Mojang or Microsoft. Use at your own risk.**

## できること / Features

- MIDIファイルを選んで、周囲の音ブロックで自動演奏(トラック選択、オクターブ調整)
- 音ブロックを自動でスキャンして、座標と音程を学習(Calibrate / Prepare & Play)
- DirectX 11 / DirectX 12 の両方に対応したオーバーレイ(Dear ImGui)
- 開閉: `Insert` キー、または `M` キー(ゲーム中。チャットやメニューが開いている間は無効)

## 動作環境 / Requirements

- Windows 10/11、Minecraft Bedrock Edition (GDK版)
- 動作確認したバージョン: `<Minecraft のバージョン>`(ゲームの更新で動かなくなることがあります)
- DLLインジェクター(このリポジトリには含まれません。下の「使い方」を参照)

## 使い方 / Usage

1. [Releases](../../releases) から `mcnote.dll` をダウンロードします。
2. Minecraft を起動して、お使いのDLLインジェクターで `mcnote.dll` を読み込みます
   (例: FateClient Injector、MCBE DLL Injector など。**これらはこのプロジェクトとは無関係で、安全性を保証しません**。
   公式の配布元から入手して、自己責任で使ってください)。
3. ワールドに入って、`Insert` か `M` でメニューを開きます。
4. **サバイバルで、何かのブロックを1回だけ左クリック**します(ゲームモードの判定のため。ゲームを起動するたびに1回)。
5. 音ブロックを、同じ楽器で、音程0〜24の25個そろえて置きます(手動で調律)。
6. `Browse...` でMIDIを選んで、`Prepare & Play`(または `Calibrate` → `Play`)を押します。

## 注意 / Notes

- **クリエイティブとスペクテイターでは動きません**(安全のため、サバイバル・アドベンチャーのみ)。
- 自動操作を行うため、**サーバーによっては規約違反や、BANの対象になる可能性があります**。
  自分のワールドや、許可のあるサーバーでのみ使ってください。
- ゲームが更新されると、フックの位置が変わって動かなくなることがあります。
  その場合は、`mcnote.log` を添えて Issue を立ててください。
- ウイルス対策ソフトが、DLLインジェクターや本DLLを警告することがあります(誤検知を含む)。

## 不具合の報告 / Reporting issues

Issue に、次を書いてください。

- Minecraft のバージョン、DirectX 11 / 12 のどちらか
- `mcnote.log`(DLLと同じフォルダにできます)

## ビルド / Build

```powershell
git clone --recurse-submodules https://github.com/Commander546/MCBE-NoteBlock-midi-Client.git
cd mcnote
xmake f -m release -c
xmake
# build\windows\x64\release\mcnote.dll
```

必要なもの: xmake、MSVC Build Tools、C++23。

## ライセンス / License

MIT License(`LICENSE` を参照)。第三者ライブラリは `THIRD_PARTY_NOTICES.md` を参照してください。
