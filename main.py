import re
import os
import json
import pandas as pd
from datetime import datetime
from flask import Flask, request, render_template, jsonify
from sqlalchemy import create_engine


copyrightName = "The 5th group of System Creation Contest 2 holded by KOSEN of Joint Education Project in Toba Colledge"
postDataPath = "POST-data"
app = Flask(__name__)
dbURL = os.getenv("DATABASE_URL")
dbTableName = "post_data"
engine = create_engine(dbURL)


# home はリンク指定なしで来た時に404を送らないようにする
@app.route("/")
def home():
	return copyrightName


# post_data にリンクを指定し、Sigfoxクラウドからのコールバックの処理をする
@app.route("/post", methods=["GET", "POST"])
def post_data():
	if request.method == "GET":
		return "POST only!"
	
	elif request.method == "POST":
		# ここでJSONを取得
		# page::uint:16:little-endian data1::float:32 data2::float:32
		tempData = request.get_json()
		print("SigfoxクラウドからPOSTされたJSON(dict): {0}".format(tempData))

		# 1ページ目
		if tempData["page"] == 1:
			# キー名を変更
			tempData = changeKeyName(tempData, "data1", "temperature")
			tempData = changeKeyName(tempData, "data2", "humidity")
			# 総合的な辞書データをグローバルで定義
			global data
			data = tempData

			# Sigfoxクラウドに返すデータ
			returnData = "first_page"
			# Sigfoxクラウドに返すJSON
			returnJSON = {}
			# JSONの中のデバイス名の中に返すデータを入れる
			returnJSON[data["device"]] = {
				"downlinkData": returnData}
		
		# 2ページ目
		elif tempData["page"] == 2:
			# キー名を変更して加え書き
			tempData = changeKeyName(tempData, "data1", "latitude")
			tempData = changeKeyName(tempData, "data2", "longitude")
			data.update(tempData)
			# ページメンバーは仮的なものなので削除
			del data["page"]
			# データの時間を取得し、UNIX時間を日時に変換
			timeOfData = int(data["time"])
			data["time"] = str(datetime.fromtimestamp(timeOfData))
			# 文字列の時刻データをpandasのdatetime型に変換
			data["time"] = pd.to_datetime(data["time"])
			# ログデータをデータベースのレコードに追加
			dbWriter(data)

			# Sigfoxクラウドに返すデータ
			returnData = "OK"
			# Sigfoxクラウドに返すJSON
			returnJSON = {}
			# JSONの中のデバイス名の中に返すデータを入れる
			returnJSON[data["device"]] = {
				"downlinkData": returnData}

		# SigfoxクラウドにJSONを返す
		return jsonify(returnJSON)


# changeKeyName は辞書のキーの名前を変更する
def changeKeyName(targetDict, oldKeyName, newKeyName):
	targetDict[newKeyName] = targetDict[oldKeyName]
	del targetDict[oldKeyName]

	return targetDict


# dbWriter はPostgreSQL(データベース)にログデータのレコードを追加する
def dbWriter(data):
	# pandasのデータフレームを作成 (仮としてindexも追加)
	df = pd.DataFrame(data, index=[0])
	# 指定したテーブルにレコードを追加 (indexはSQLに送らない)
	df.to_sql(dbTableName, con=engine, if_exists="append", index=False)


# check はデータベースに記録されているログデータを確認する
@app.route("/check", methods=["GET", "POST"])
def check():
	if request.method == "GET":
		if request.args.get("device"):
			deviceID = request.args.get("device")
			# データベースのテーブルのデータを読み込み
			readTableData = pd.read_sql_table(dbTableName, dbURL)

			# デバイスカラムの中からデバイスIDが見つかれば
			if deviceID in readTableData["device"].values:
				# そのデバイスの列を抜粋し、値を取得
				deviceLogs = readTableData.query("device == @deviceID")
				deviceLogsWithoutDevice = deviceLogs.drop(columns="device")

				showColumns = ""
				for i in range(len(deviceLogsWithoutDevice.columns.values)):
					logsColumnsStr = deviceLogsWithoutDevice.columns.values[i]
					showColumns += logsColumnsStr
					if not i == len(logsColumnsStr) - 1:
						showColumns += " "

				showLogs = ""
				for i in range(len(deviceLogs)):
					deviceLogsStr = str(deviceLogsWithoutDevice.values[i]).replace("[", "").replace("]", "")
					showLogs += deviceLogsStr
					if not i == len(deviceLogs) - 1:
						showLogs += "<br>"
				
				return render_template("index.html",
						deviceID = deviceID,
						status = "OK",
						columns = showColumns,
						logs = showLogs)
			
			else:
				return render_template("index.html",
						deviceID = request.args.get("device") + " (存在しない)",
						status = "NoSuchDevice")
		
		else:
			return copyrightName

	elif request.method == "POST":
		return "GET only!"


if __name__ == "__main__":
	print(copyrightName)
	app.run(host=os.getenv("HOST"),
			port=int(os.getenv("PORT")),
			debug=False)