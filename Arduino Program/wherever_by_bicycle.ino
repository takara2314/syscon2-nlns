/*
 * wherever by bicycle
 * 第2回システム創成コンテストの、
 * 鳥羽商船5班『No limit Non stop』のチームのArduinoコードです。
 * 
 * 使用外部ライブラリ：
 * 	sigfox-shield-arduino-master
 * 	Adafruit_Unified_Sensor
 * 	Adafruit_BME280_Library
 * 	TinyGPSPlus
 */


#include <SIGFOX.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>


// 課題名
const String ASSIGNMENT_NAME = "wherever by bicycle";
// チーム名
const String TEAM_NAME = "No limit Non stop";

// 測定開始までのウェイトタイム [秒]
const unsigned int WAIT_TIME = 3;
// メッセージ送信回数上限
const unsigned int MAX_MESSAGE_COUNT = 10;
// メッセージ送信間隔 [ミリ秒]
const unsigned long int MESSAGE_INTERVAL = 300000UL;
// ページ送信間隔 [秒]
const unsigned int PAGE_INTERVAL = 2;
// RX(受信)ポートのピン (GPS)
const unsigned int RX_PIN = 10;
// TX(送信)ポートのピン (GPS)
const unsigned int TX_PIN = 11;

// UnaBiz Emulator を使用しない
const String UBEdevice = "NOTUSED";
const bool useEmulator = false;
// Sigfoxライブラリが実行した出力結果を表示しない
const bool echo = false;
// 日本のSigfox周波数
const Country country = COUNTRY_JP;
// 以上の設定を適応した状態でUnaShieldを定義
UnaShieldV2S transceiver(country, useEmulator, UBEdevice, echo);
// BME280センサを定義
Adafruit_BME280 bme;
// Sigfoxクラウドからの戻り値(下り回線)を収納する変数
String response;

// GPS用のシリアルポートを定義
SoftwareSerial SerialGPS(RX_PIN, TX_PIN);
// GPSライブラリを定義
TinyGPSPlus gps;


void setup() {
	// シリアル通信を起動
	Serial.begin(9600);
	// GPS用のシリアル通信を起動
	Serial1.begin(9600);

	horizonPrint("■", 18, true);
	Serial.println(ASSIGNMENT_NAME);
	Serial.print("メッセージ送信回数上限: "); Serial.print(MAX_MESSAGE_COUNT); Serial.println("回");
	Serial.print("メッセージ送信間隔: "); Serial.print(MESSAGE_INTERVAL); Serial.println("ミリ秒");
	Serial.print("ページ送信間隔: "); Serial.print(PAGE_INTERVAL); Serial.println("秒");
	Serial.print("\n");
	Serial.println("デバイスID, PAC: ");
	// Sigfoxモジュールを起動
	if (transceiver.begin())
		Serial.println("Sigfoxモジュールの起動: OK");
	else
		stop(F("Sigfoxモジュールの起動: NG"));
	// BME280センサを起動
	if (bme.begin(0x76))
		Serial.println("BME280センサの起動: OK");
	else
		stop("BME280センサの起動: NG");
	horizonPrint("■", 18, true);

	Serial.print("計測開始まで "); countPrint(WAIT_TIME);
	Serial.print("\n\n");
}


void loop() {
	// 温度を取得
	float temperature = bme.readTemperature();
	// 湿度の取得
	float humidity = bme.readHumidity();

	// データを読み込み
	char c = Serial1.read();
	gps.encode(c);
	Serial.println(c);
	Serial.println(gps.location.isUpdated());
	// 緯度の取得
	float latitude = gps.location.lat();
	// 経度の取得
	float longitude = gps.location.lng();
	
	// Sigfoxクラウドに温度、湿度、緯度、経度を送信
	sendSigfox(temperature, humidity, latitude, longitude);

	delay(MESSAGE_INTERVAL);
	Serial.print("\n\n");
}


// horizonPrint は水平線を指定された文字数の文だけ出力する
void horizonPrint(String symbol, int num, bool newLine) {
	for (int i = 0; i < num; i++)
		Serial.print(symbol);
	if (newLine)
		Serial.print("\n");
}

// bracketsSander は文字列を括弧で挟んで出力する
void bracketsSander(String str) {
	Serial.print("("); Serial.print(str); Serial.print(")");
}


// countPrint はカウントダウンを1スペース間隔で出力する
void countPrint(int num) {
	for (int i = num; i > 0; i--) {
		delay(1000);
		Serial.print(i);
		if (i != 1)
			Serial.print(" ");
	}
	delay(1000);
	Serial.print("\n");
}


// sendSigfox はSigfoxクラウドにデータを送信する総合的なターミナル
void sendSigfox(float temperature, float humidity, float latitude, float longitude) {
	// 何度かに分けて送信するので、現在のページ数を定義
	unsigned int pageCount = 0;
	// 送信するデータ(16進String)
	String sendData;
	// 送信結果
	bool sendResult;

	// 各データの16進数String型版を定義
	String hexTemperature = convertFloatToHex(temperature);
	String hexHumidity = convertFloatToHex(humidity);
	String hexLatitude = convertFloatToHex(latitude);
	String hexLongitude = convertFloatToHex(longitude);

	// 1ページ目：温度、湿度
	pageCount = 1;
	horizonPrint(":", 12, false); Serial.print(" 1ページ目 "); horizonPrint(":", 12, true);
	Serial.print("温度: "); Serial.print(temperature, 6); Serial.print("℃ "); bracketsSander("0x" + hexTemperature); Serial.print("\n");
	Serial.print("湿度: "); Serial.print(humidity, 6); Serial.print("% "); bracketsSander("0x" + hexHumidity); Serial.print("\n");
	sendData = transceiver.toHex(1) + hexTemperature + hexHumidity;
	Serial.print("送信データ: "); Serial.println(sendData);
	Serial.print("サイズ: "); Serial.print(sendData.length() / 2); Serial.println("bytes");
	horizonPrint(":", 34, true);
	// Sigfoxクラウドへデータを送信
	sendResult = transceiver.sendMessage(sendData);
	if (sendResult)
		Serial.println("Sigfoxクラウドに送信成功しました。");
	else
		Serial.println("Sigfoxクラウドに送信失敗しました。");

	delay(PAGE_INTERVAL * 1000);
	Serial.print("\n");
	
	// 2ページ目：緯度、経度
	horizonPrint(":", 12, false); Serial.print(" 2ページ目 "); horizonPrint(":", 12, true);
	Serial.print("緯度: "); Serial.print(latitude, 6); Serial.print("° "); bracketsSander("0x" + hexLatitude); Serial.print("\n");
	Serial.print("経度: "); Serial.print(longitude, 6); Serial.print("° "); bracketsSander("0x" + hexLongitude); Serial.print("\n");
	sendData = transceiver.toHex(2) + hexLatitude + hexLongitude;
	Serial.print("送信データ: "); Serial.println(sendData);
	Serial.print("サイズ: "); Serial.print(sendData.length() / 2); Serial.println("bytes");
	horizonPrint(":", 34, true);
	// Sigfoxクラウドへデータを送信
  	sendResult = transceiver.sendMessage(sendData);
	if (sendResult)
		Serial.println("Sigfoxクラウドに送信成功しました。");
	else
		Serial.println("Sigfoxクラウドに送信失敗しました。");
}


// convertFloatToHex はfloat型を16進数String型へ変換する
String convertFloatToHex(float num) {
	// 共用体を定義
	union {
		uint32_t B32;
		float Float;
	} floatb32;

	floatb32.Float = num;
	return String(floatb32.B32, HEX);
}
