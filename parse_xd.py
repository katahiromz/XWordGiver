# parse_xd.py --- XDファイルを解析する
# Author: katahiromz
# License: MIT

import sys
import jaconv

# バージョン情報を表示する
def version():
	print("parse_xd.py Version 1.0 by katahiromz")

# 使い方を表示する
def usage():
	print("Usage: python parse_xd.py your_file.xd")

# 全角文字
ZEN_SPACE       = "　" # U+3000: 全角スペース
ZEN_UNDERLINE   = "＿" # U+FF3F: 全角のアンダースコア
ZEN_SHARP1      = "♯" # U+266F
ZEN_SHARP2      = "＃" # U+FF03
ZEN_DOT         = "．" # U+FF0E: 全角ドット(ピリオド)
ZEN_BLACK       = "■" # U+25A0: 黒マス

# クロスワード用に文字列を変換する
def normalize_str(str):
	# 空白マス
	str = str.replace(" ", ZEN_SPACE)
	str = str.replace("_", ZEN_SPACE)
	str = str.replace(ZEN_UNDERLINE, ZEN_SPACE)
	# 黒マス
	str = str.replace("#", ZEN_BLACK)
	str = str.replace(ZEN_SHARP1, ZEN_BLACK)
	str = str.replace(ZEN_SHARP2, ZEN_BLACK)
	str = str.replace(".", ZEN_BLACK)
	str = str.replace(ZEN_DOT, ZEN_BLACK)
	# ひらがなをカタカナに
	str = jaconv.hira2kata(str)
	# 半角を全角に
	str = jaconv.h2z(str);
	# 小文字を大文字に
	str = str.upper()
	str = str.replace("ァ", "ア")
	str = str.replace("ィ", "イ")
	str = str.replace("ゥ", "ウ")
	str = str.replace("ェ", "エ")
	str = str.replace("ォ", "オ")
	str = str.replace("ッ", "ツ")
	str = str.replace("ャ", "ヤ")
	str = str.replace("ュ", "ユ")
	str = str.replace("ョ", "ヨ")
	str = str.replace("ヵ", "カ")
	str = str.replace("ヶ", "ケ")
	return str

# 空白マスがないか？
def fulfill(rows):
	for row in rows:
		for ch in row:
			if ch == ZEN_SPACE:
				return False
	return True

# XD文字列をパースする
def parse_xd(xd_str):
	header = ""
	notes = ""
	rows = []
	clues = []
	marks = []
	numcros = []
	boxes = []
	iSection = 0
	cEmpty = 0
	header = ""
	view_mode = -1
	policy = -1
	mark_str = ""

	# 行に分割して一行ずつ処理する
	lines = xd_str.split("\n")
	for line in lines:
		line0 = line.strip()
		if line0 == "" and iSection < 3:
			if cEmpty == 1:
				iSection = iSection + 1
			cEmpty = cEmpty + 1
		else:
			cEmpty = 0
			match iSection:
				case 0:
					header += line0
					header += "\n"
				case 1:
					rows.append(line0)
				case 2:
					clues.append(line0)
				case 3:
					if line0.find("MARK") == 0:
						marks.append(line0)
						ich = line0.find(":")
						if ich != -1:
							mark_str += line0[ich+1:].strip()
					elif line0.find("NUMCRO-") == 0:
						numcros.append(line0)
					elif line0.find("Box: ") == 0:
						boxes.append(line0)
					elif line0.find("ViewMode:") == 0:
						view_mode = int(line0[9:])
					elif line0.find("Policy:") == 0:
						policy = int(line0[7:])
					else:
						notes += line0
						notes += "\n"
	# ヘッダーがなければ失敗
	if header == "":
		return None
	# 盤面データがなければ失敗
	if len(rows) <= 0 or len(rows[0]) <= 0:
		return None
	# 盤面データがおかしいなら失敗
	for row in rows:
		if len(row) != len(rows[0]):
			return None
	# 盤面データの文字を変換
	new_rows = []
	for row in rows:
		new_row = normalize_str(row)
		new_rows.append(new_row)
	rows = new_rows

	# 使用単語を取得
	words = []
	for clue in clues:
		ich = clue.find(" ~ ")
		if ich != -1 and (clue[0] == "A" or clue[0] == "D"):
			words.append(clue[ich + 3:])
	if mark_str != "":
		words.append(mark_str)

	# 前後の空白を取り除く
	header = header.strip()
	notes = notes.strip()

	return header, notes, rows, clues, words, marks, mark_str, view_mode, policy, numcros

if len(sys.argv) != 2 or sys.argv[1] == "--help":
	usage()
	sys.exit(0)

if sys.argv[1] == "--version":
	version()
	sys.exit(0)

# XDファイルを読み込む
filename = sys.argv[1]
try:
	with open(filename, "r", encoding='utf-8') as fp:
		# ファイル全体を文字列として読み込む
		xd_str = fp.read()
except OSError as e:
	print(f"Cannot open file: {filename} ({e})", file=sys.stderr)
	sys.exit(1)

# パースする
result = parse_xd(xd_str)
if result is None:
	print("Error: invalid .xd file", file=sys.stderr)
	sys.exit(1)
[header, notes, rows, clues, words, marks, mark_str, view_mode, policy, numcros] = result
is_fulfill = fulfill(rows) # すべてのマスが埋まっているか？

# カギをパースする
clue_mapping0 = {} # clue_name -> clue_word
clue_mapping1 = {} # clue_name -> clue_hint
for clue in clues:
	if clue[0] == "A" or clue[0] == "D":
		dot_pos = clue.find(".")
		clue_name = clue[0:dot_pos].strip()
		clue_body = clue[dot_pos + 1:].strip()
		if dot_pos != -1 and clue_name != "":
			tilda_pos = clue_body.find("~")
			clue_word = clue_body[tilda_pos+1:].strip()
			clue_hint = clue_body[0:tilda_pos].strip()
			clue_mapping0[clue_name] = clue_word
			clue_mapping1[clue_name] = clue_hint

# TODO: ここでやりたいことをやる。例えば使用単語を出力する。
for word in words:
	print(word)

#print("header: " + str(header))
#print("notes: " + str(notes))
#print("rows: " + str(rows))
#print("clues: " + str(clues))
#print("words: " + str(words))
#print("marks: " + str(marks))
#print("mark_str: " + str(mark_str))
#print("is_fulfill: " + str(is_fulfill))
#print("view_mode: " + str(view_mode))
#print("policy: " + str(policy))
#print("numcros: " + str(numcros))
#print("clue_mapping0: " + str(clue_mapping0))
#print("clue_mapping1: " + str(clue_mapping1))
