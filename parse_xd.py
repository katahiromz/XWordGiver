# parse_xd.py --- XDファイルを解析する（クロスワード ギバー用）
# Author: katahiromz
# License: MIT

import sys
import jaconv

# NOTE: UTF-8で出力したい人は、環境変数「PYTHONIOENCODING=utf-8」をセットしてください。

# バージョン情報を表示する
def version():
	print("parse_xd.py Version 1.0 by katahiromz")

# 使い方を表示する
def usage():
	print("Usage: python parse_xd.py your_file.xd")

# 全角文字
ZEN_SPACE       = "　" # U+3000: 全角スペース、空白マスと見なす
ZEN_UNDERLINE   = "＿" # U+FF3F: 全角のアンダースコア、空白マスと見なす
ZEN_SHARP1      = "♯" # U+266F: 黒マスと見なす
ZEN_SHARP2      = "＃" # U+FF03: 黒マスと見なす
ZEN_DOT         = "．" # U+FF0E: 全角ドット(ピリオド)、立入禁止。クロスワード ギバーでは黒マスと見なす
ZEN_BLACK       = "■" # U+25A0: 黒マス

# 表示モード
XG_VIEW_NORMAL   = 0 # 通常ビュー。
XG_VIEW_SKELETON = 1 # スケルトンビュー。

# 黒マスルール
RULE_DONTDOUBLEBLACK = (1 << 0)    # 連黒禁。
RULE_DONTCORNERBLACK = (1 << 1)    # 四隅黒禁。
RULE_DONTTRIDIRECTIONS = (1 << 2)  # 三方黒禁。
RULE_DONTDIVIDE = (1 << 3)         # 分断禁。
RULE_DONTFOURDIAGONALS = (1 << 4)  # 黒斜四連禁。
RULE_POINTSYMMETRY = (1 << 5)      # 黒マス点対称。
RULE_DONTTHREEDIAGONALS = (1 << 6) # 黒斜三連禁。
RULE_LINESYMMETRYV = (1 << 7)      # 黒マス線対称（タテ）。
RULE_LINESYMMETRYH = (1 << 8)      # 黒マス線対称（ヨコ）。
XG_DEFAULT_RULES = (RULE_DONTDIVIDE | RULE_POINTSYMMETRY) # デフォルトのルール

# クロスワード用に文字列を変換する
def normalize_string(s):
	# 空白マス
	s = s.replace(" ", ZEN_SPACE)
	s = s.replace("_", ZEN_SPACE)
	s = s.replace(ZEN_UNDERLINE, ZEN_SPACE)
	# 黒マス
	s = s.replace("#", ZEN_BLACK)
	s = s.replace(ZEN_SHARP1, ZEN_BLACK)
	s = s.replace(ZEN_SHARP2, ZEN_BLACK)
	s = s.replace(".", ZEN_BLACK) # クロスワード ギバーでは立入禁止は黒マスと見なす
	s = s.replace(ZEN_DOT, ZEN_BLACK)
	# ひらがなをカタカナに
	s = jaconv.hira2kata(s)
	# 半角を全角に
	s = jaconv.h2z(s);
	# 小文字を大文字に
	s = s.upper()
	s = s.replace("ァ", "ア")
	s = s.replace("ィ", "イ")
	s = s.replace("ゥ", "ウ")
	s = s.replace("ェ", "エ")
	s = s.replace("ォ", "オ")
	s = s.replace("ッ", "ツ")
	s = s.replace("ャ", "ヤ")
	s = s.replace("ュ", "ユ")
	s = s.replace("ョ", "ヨ")
	s = s.replace("ヵ", "カ")
	s = s.replace("ヶ", "ケ")
	return s

# カギ（clue）の1行をパースする
# 戻り値: (clue_name, clue_hint, clue_word) のタプル。パースできなければ None。
#   clue_name: "A1" や "D3" などのカギ番号
#   clue_hint: ヒント文（例題や説明）
#   clue_word: 使用される単語（解答）
def parse_clue(clue):
	# 先頭が "A"（Across）か "D"（Down）でなければ無効
	if clue == "" or (clue[0] != "A" and clue[0] != "D"):
		return None
	# "." がなければ無効（カギ番号と本文の区切り）
	dot_pos = clue.find(".")
	if dot_pos == -1:
		return None
	clue_name = clue[0:dot_pos].strip()
	if clue_name == "":
		return None
	# "." より後ろに "~" がなければ無効（ヒントと単語の区切り）
	tilda_pos = clue.find("~", dot_pos)
	if tilda_pos == -1:
		return None
	clue_body = clue[dot_pos + 1:]
	tilda_pos_in_body = tilda_pos - (dot_pos + 1)
	clue_hint = clue_body[0:tilda_pos_in_body].strip()
	clue_word = clue_body[tilda_pos_in_body + 1:].strip()
	return clue_name, clue_hint, clue_word

# 空白マスがないか？
def is_filled(rows):
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
	iSection = 0 # セクション番号
	cEmpty = 0 # 空行カウンタ
	view_mode = XG_VIEW_NORMAL
	policy = XG_DEFAULT_RULES
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
							mark_str += line0[ich+1:].strip() # 二重マス文字が行ごとに分かれているため、ここで結合
					elif line0.find("NUMCRO-") == 0:
						numcros.append(line0)
					elif line0.find("Box: ") == 0:
						boxes.append(line0)
					elif line0.find("ViewMode:") == 0:
						try:
							view_mode = int(line0[9:].strip(), 0)
						except ValueError as e:
							return None
						if view_mode != XG_VIEW_NORMAL and view_mode != XG_VIEW_SKELETON:
							view_mode = XG_VIEW_NORMAL
					elif line0.find("Policy:") == 0:
						try:
							policy = int(line0[7:].strip(), 0) | RULE_DONTDIVIDE
						except ValueError as e:
							return None
					else:
						notes += line0
						notes += "\n"
	# ヘッダーがなければ失敗
	if header == "":
		return None
	# 盤面データがない、または小さすぎるなら失敗
	if len(rows) <= 2 or len(rows[0]) <= 2:
		return None
	# 盤面データがおかしいなら失敗
	for row in rows:
		if len(row) != len(rows[0]):
			return None
	# 盤面データの文字を変換
	new_rows = []
	for row in rows:
		new_row = normalize_string(row)
		new_rows.append(new_row)
	rows = new_rows

	# 使用単語を取得
	words = []
	for clue in clues:
		parsed = parse_clue(clue)
		if parsed is not None:
			words.append(parsed[2])
	if mark_str != "":
		words.append(mark_str)

	# 前後の空白を取り除く
	header = header.strip()
	notes = notes.strip()

	# カギをパースする
	clue_mapping0 = {} # clue_name -> clue_word
	clue_mapping1 = {} # clue_name -> clue_hint
	for clue in clues:
		parsed = parse_clue(clue)
		if parsed is not None:
			clue_name, clue_hint, clue_word = parsed
			clue_mapping0[clue_name] = clue_word
			clue_mapping1[clue_name] = clue_hint

	return header, notes, rows, clues, words, marks, mark_str, view_mode, policy, numcros, boxes, clue_mapping0, clue_mapping1

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
header, notes, rows, clues, words, marks, mark_str, view_mode, policy, numcros, boxes, clue_mapping0, clue_mapping1 = result
is_full = is_filled(rows) # すべてのマスが埋まっているか？

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
#print("is_full: " + str(is_full))
#print("view_mode: " + str(view_mode))
#print("policy: " + str(policy))
#print("numcros: " + str(numcros))
#print("boxes: " + str(boxes))
#print("clue_mapping0: " + str(clue_mapping0))
#print("clue_mapping1: " + str(clue_mapping1))
