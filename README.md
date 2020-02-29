# wherever by bicycle

第2回システム創成コンテストの、チーム『No limit Non stop』のサーバーです。
主にPythonのFlaskで開発されています。

## 簡単な説明

SigfoxクラウドからのコールバックをJSONのPOSTで受け取り、データを基にハンガーノック状態かどうかを判定し、クライアントにJSONとして返します。
ハンガーノックとは長時間にわたるスポーツを行っている際に、極度の低血糖状態に陥ることを指します。
主に時間、斜度、気温、位置情報のデータを扱います。

## 使用機器

Arduino Uno Rev3

Sigfox Shield for Arduino (UnaShield V2S2)

GNSS (GPS･GLONASS･QZSS) 受信機キット　1PPS出力　みちびき3機対応　アンテナ外付タイプ

GPS/GLONASS　アクティブアンテナ　LNA内臓　DC2.2V~5V対応

SMA-J⇔IPEX (IPX/U.FL) 変換ケーブル (120mm)　テフロン同軸ケーブル

## 機能

SigfoxクラウドからPOSTされたJSONをデータベース(PostgreSQL)にログとして記録し、
pandasでデータ処理を行います。
データには時刻・温度・湿度・緯度・経度が含まれており、環境の異常がないかを推測し、
Sigfoxクラウドを通してArduinoに信号を送ります。

## 使い方

大会運営のPCからGETリクエストできるBOTを作成し、GETしたものをGUIに表示します。