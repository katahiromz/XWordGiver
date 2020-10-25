#!/usr/bin/env python
# xword.py --- クロスワードファイル (XWJ: JSON形式) を扱うPythonプログラム。
# Author: Katayama Hirofumi MZ
# License: MIT
#
# クロスワードファイル (*.xwj; JSON形式) の検査用、画像出力用にお使い下さい。
# XWDファイルには対応してません。JSON形式のXWJファイルをお使い下さい。

def JSON形式を開く(filename):
	data = None
	with open(filename, 'r', encoding='utf-8-sig') as fp:
		import json
		data = json.load(fp)
	return data

def JSON形式で保存(filename, data):
	if data == None:
		raise FileError
	import sys
	data['creator_info'] = 'Python (' + sys.argv[0] + ") " + sys.version
	with open(filename, 'w', encoding='utf-8-sig') as fp:
		import json
		json.dump(data, fp, indent=4, ensure_ascii=False)

def 四隅に黒マス(row_count, column_count, cell_data):
	if cell_data[0][0] == '■':
		return True
	if cell_data[row_count - 1][0] == '■':
		return True
	if cell_data[row_count - 1][column_count - 1] == '■':
		return True
	if cell_data[0][column_count - 1] == '■':
		return True
	return False

def 連黒(row_count, column_count, cell_data):
	for i in range(0, row_count):
		for j in range(0, column_count):
			if cell_data[i][j] == '■':
				if i + 1 < row_count and cell_data[i + 1][j] == '■':
					return True
				if j + 1 < column_count and cell_data[i][j + 1] == '■':
					return True
	return False

def 三方向黒マス(row_count, column_count, cell_data):
	for i in range(0, row_count):
		for j in range(0, column_count):
			count = 0
			if i > 0 and cell_data[i - 1][j] == '■':
				count += 1
			if j > 0 and cell_data[i][j - 1] == '■':
				count += 1
			if i + 1 < row_count and cell_data[i + 1][j] == '■':
				count += 1
			if j + 1 < column_count and cell_data[i][j + 1] == '■':
				count += 1
			if count >= 3:
				return False
	return False

def 分断(row_count, column_count, cell_data):
	pairs = [[0, 0]]
	flags = [False] * row_count * column_count
	while len(pairs) > 0:
		pair = pairs[len(pairs) - 1]
		del pairs[len(pairs) - 1]
		x = pair[0]
		y = pair[1]
		if not flags[x + y * column_count]:
			flags[x + y * column_count] = True
			if x != 0:
				pairs.append([x - 1, y])
			if x + 1 < column_count:
				pairs.append([x + 1, y])
			if y != 0:
				pairs.append([x, y - 1])
			if y + 1 < row_count:
				pairs.append([x, y + 1])
	for flag in flags:
		if not flag:
			return True
	return False

def 単語の重複(h, v):
	words = []
	for pair in h.values():
		if pair[0] in words:
			return True
		words.append(pair[0])
	for pair in v.values():
		if pair[0] in words:
			return True
		words.append(pair[0])
	return False

def 斜同字(row_count, column_count, cell_data):
	for i in range(0, row_count):
		for j in range(0, column_count):
			ch = cell_data[i][j]
			if ch == '　' or ch == '■':
				continue
			if i + 1 < row_count and j + 1 < column_count:
				if cell_data[i + 1][j + 1] == ch:
					return True
			if i > 0 and j + 1 < column_count:
				if cell_data[i - 1][j + 1] == ch:
					return True
	return False

# XWJ (JSON) ファイルをチェックする。
def JSON形式をチェック(filename=None, data=None):
	if data == None:
		data = JSON形式を開く(filename)
	if data == None:
		raise RuntimeError('ファイル「' + filename + '」の読み込みに失敗しました。')
	# キーの存在確認。
	if 'cell_data' not in data:
		raise RuntimeError('「cell_data」キーがありません。')
	if 'column_count' not in data:
		raise RuntimeError('「column_count」キーがありません。')
	if 'creator_info' not in data:
		raise RuntimeError('「creator_info」キーがありません。')
	if 'has_hints' not in data:
		raise RuntimeError('「has_hints」キーがありません。')
	if 'has_mark' not in data:
		raise RuntimeError('「has_mark」キーがありません。')
	if 'header' not in data:
		raise RuntimeError('「header」キーがありません。')
	if data['has_hints']:
		if 'hints' not in data:
			raise RuntimeError('「hints」キーがありません。')
		if 'h' not in data['hints']:
			raise RuntimeError('「hints.h」キーがありません。')
		if 'v' not in data['hints']:
			raise RuntimeError('「hints.v」キーがありません。')
	if 'is_solved' not in data:
		raise RuntimeError('「is_solved」キーがありません。')
	if 'notes' not in data:
		raise RuntimeError('「notes」キーがありません。')
	if 'row_count' not in data:
		raise RuntimeError('「row_count」キーがありません。')
	# 列数、行数を確認。
	row_count = data['row_count']
	column_count = data['column_count']
	if len(data['cell_data']) != row_count:
		raise RuntimeError('行数が合いません。')
	for line in data['cell_data']:
		if len(line) != column_count:
			raise RuntimeError('列数が合いません。')
	# 解ならすべて埋まっているはず。
	cell_data = data['cell_data']
	if data['is_solved']:
		for line in cell_data:
			flag = False
			try:
				line.index('　')
				flag = True
			except:
				pass
			if flag:
				raise RuntimeError('解なのに空マスがあります。')
	# 二重マスが付いているなら正当な二重マスがあるはず。
	if data['has_mark']:
		if 'mark_word' not in data:
			raise RuntimeError('「mark_word」キーがありません。')
		if 'marks' not in data:
			raise RuntimeError('「marks」キーがありません。')
		if len(data['mark_word']) != len(data['marks']):
			raise RuntimeError('「mark_word」と「marks」の長さが一致しません。')
		for mark in data['marks']:
			if cell_data[mark[0] - 1][mark[1] - 1] != mark[2]:
				raise RuntimeError('二重マスの文字が一致しません')
	# 解ならヒントとマスが合致するはず。
	if data['has_hints']:
		hints = data['hints']
		h = {}
		v = {}
		if data['is_solved']:
			for item in hints['h']:
				if len(item) != 3:
					raise RuntimeError('ヒントがおかしいです。')
				h[item[0]] = [item[1], item[2]]
			for item in hints['v']:
				if len(item) != 3:
					raise RuntimeError('ヒントがおかしいです。')
				v[item[0]] = [item[1], item[2]]
			number = 1
			for i in range(0, row_count):
				for j in range(0, column_count):
					tate = yoko = False
					if cell_data[i][j] == '■':
						continue
					if i == 0 or cell_data[i - 1][j] == '■':
						if i + 1 < row_count and cell_data[i + 1][j] != '■':
							tate = True
					if j == 0 or cell_data[i][j - 1] == '■':
						if j + 1 < column_count and cell_data[i][j + 1] != '■':
							yoko = True
					if tate:
						k = 0
						for ch in v[number][0]:
							if cell_data[i + k][j] != ch:
								raise RuntimeError('ヒントの文字が一致しません。')
							k += 1
					if yoko:
						k = 0
						for ch in h[number][0]:
							if cell_data[i][j + k] != ch:
								raise RuntimeError('ヒントの文字が一致しません。')
							k += 1
					if tate or yoko:
						number += 1
	# 解なら正当であるはず。
	if data['is_solved']:
		if 四隅に黒マス(row_count, column_count, cell_data):
			raise RuntimeError('四隅に黒マスがあります。')
		if 連黒(row_count, column_count, cell_data):
			raise RuntimeError('連黒。')
		if 三方向黒マス(row_count, column_count, cell_data):
			raise RuntimeError('三方向黒マス。')
		if 分断(row_count, column_count, cell_data):
			raise RuntimeError('分断。')
		if 単語の重複(h, v):
			raise RuntimeError('単語の重複。')
	return True

class クロスワード:
	def __init__(self, filename=None, json_data=None):
		try:
			if filename != None:
				print("ファイル「" + filename + "」を処理中...")
				try:
					self.data = JSON形式を開く(filename)
				except RuntimeError as err:
					raise RuntimeError("ファイル「" + filename + "」が開けません。")
			else:
				self.data = json_data
			JSON形式をチェック(data=self.data)
		except RuntimeError as err:
			print(str(err))
			raise err
		self.列数 = self.data['column_count']
		self.行数 = self.data['row_count']
		self.セル = self.data['cell_data']
		self.タテのカギ = {}
		self.ヨコのカギ = {}
		if self.data['has_hints']:
			self.タテのカギ = self.data['hints']['v']
			self.ヨコのカギ = self.data['hints']['h']
		self.解あり = self.data['is_solved']
		self.二重 = {}
		number = 0
		if self.data['has_mark']:
			for mark in self.data['marks']:
				self.二重[(mark[0] - 1, mark[1] - 1)] = number
				number += 1
		self.付番 = {}
		number = 1
		for i in range(0, self.行数):
			for j in range(0, self.列数):
				tate = yoko = False
				if self.セル[i][j] == '■':
					continue
				if i == 0 or self.セル[i - 1][j] == '■':
					if i + 1 < self.行数 and self.セル[i + 1][j] != '■':
						tate = True
				if j == 0 or self.セル[i][j - 1] == '■':
					if j + 1 < self.列数 and self.セル[i][j + 1] != '■':
						yoko = True
				if tate or yoko:
					self.付番[i * self.行数 + j] = number
					number += 1
	def 四隅に黒マス(self):
		return 四隅に黒マス(self.行数, self.列数, self.セル)
	def 連黒(self):
		return 連黒(self.行数, self.列数, self.セル)
	def 三方向黒マス(self):
		return 三方向黒マス(self.行数, self.列数, self.セル)
	def 分断(self):
		return 分断(self.行数, self.列数, self.セル)
	def 単語の重複(self, h, v):
		return 単語の重複(self.ヨコのカギ, self.タテのカギ)
	def 斜同字(self):
		return 斜同字(self.行数, self.列数, self.セル)
	def JSON形式で保存(self, filename):
		JSON形式で保存(filename, self.data)
		print("JSONファイル「" + filename + "」を保存しました。")
	def 画像形式で保存(self, filename, フォントパス=None, 枠線の幅=5, 線の幅=2, セルサイズ=80, 文字サイズ=50, 小さい文字サイズ=20, 文字を隠す=False, 文字色="black", 背景色="white", 二重マスの色="yellow"):
		from PIL import Image, ImageDraw, ImageFont
		# フォント。
		if フォントパス == None:
			フォントパス = "C:/Windows/fonts/msgothic.ttc"
		font = ImageFont.truetype(フォントパス, 文字サイズ)
		small_font = ImageFont.truetype(フォントパス, 小さい文字サイズ)
		# 画像を作成する。背景は文字色。
		image_size = (セルサイズ * self.列数 + 枠線の幅 * 2, セルサイズ * self.行数 + 枠線の幅 * 2)
		img = Image.new('RGBA', image_size, 文字色)
		# 描画を開始する。
		draw = ImageDraw.Draw(img)
		x0 = 枠線の幅
		y0 = 枠線の幅
		x1 = 枠線の幅 + self.列数 * セルサイズ
		y1 = 枠線の幅 + self.行数 * セルサイズ
		# 背景色でくりぬく。
		draw.rectangle((x0, y0, x1, y1), fill=背景色)
		# 二重マスを描く。
		for i in range(0, self.行数):
			y = 枠線の幅 + i * セルサイズ
			for j in range(0, self.列数):
				x = 枠線の幅 + j * セルサイズ
				y = 枠線の幅 + i * セルサイズ
				ch = self.セル[i][j]
				if ch == '■':
					continue
				if (i, j) in self.二重:
					draw.rectangle([x, y, x + セルサイズ, y + セルサイズ], fill=二重マスの色)
		# 線を描く。
		for i in range(0, self.行数 + 1):
			y = 枠線の幅 + i * セルサイズ
			x0 = 枠線の幅
			x1 = 枠線の幅 + self.列数 * セルサイズ
			draw.rectangle((x0, y - 線の幅 / 2, x1, y + 線の幅 / 2), fill=文字色)
			for j in range(0, self.列数 + 1):
				x = 枠線の幅 + j * セルサイズ
				y0 = 枠線の幅
				y1 = 枠線の幅 + self.行数 * セルサイズ
				draw.rectangle((x - 線の幅 / 2, y0, x + 線の幅 / 2, y1), fill=文字色)
		# 黒マスを描く。
		for i in range(0, self.行数):
			y = 枠線の幅 + i * セルサイズ
			for j in range(0, self.列数):
				x = 枠線の幅 + j * セルサイズ
				ch = self.セル[i][j]
				if ch == '■':
					draw.rectangle([x, y, x + セルサイズ, y + セルサイズ], fill=文字色)
		# 小さい文字を描く。
		for i in range(0, self.行数):
			for j in range(0, self.列数):
				ch = self.セル[i][j]
				if ch == '■':
					continue
				if (i * self.行数 + j) in self.付番:
					x = 枠線の幅 + j * セルサイズ + 線の幅 * 1.5
					y = 枠線の幅 + i * セルサイズ + 線の幅 * 1.5
					draw.text((x, y), str(self.付番[i * self.行数 + j]), fill=文字色, font=small_font)
		# 二重マスの小さい文字を描く。
		for i in range(0, self.行数):
			y = 枠線の幅 + i * セルサイズ
			for j in range(0, self.列数):
				x = 枠線の幅 + (j + 1) * セルサイズ - 線の幅
				y = 枠線の幅 + (i + 1) * セルサイズ - 線の幅
				ch = self.セル[i][j]
				if ch == '■':
					continue
				if (i, j) in self.二重:
					ch = chr(ord('A') + self.二重[(i, j)])
					w, h = draw.textsize(ch, font=small_font)
					draw.text((x - w, y - h), ch, fill=文字色, font=small_font)
		if not 文字を隠す:
			# 文字を描く。
			for i in range(0, self.行数):
				for j in range(0, self.列数):
					ch = self.セル[i][j]
					if ch == '■':
						continue
					x = 枠線の幅 + j * セルサイズ + (セルサイズ - 文字サイズ) / 2
					y = 枠線の幅 + i * セルサイズ + (セルサイズ - 文字サイズ) / 2
					draw.text((x, y), ch, fill=文字色, font=font)
		# 画像を保存する。
		img.save(filename)
		print("画像ファイル「" + filename + "」を保存しました。")

def 拡張子をXWJに(filename):
	import os
	name, dotext = os.path.splitext(filename)
	name += ".xwj"
	return name

def 拡張子をJSONに(filename):
	import os
	name, dotext = os.path.splitext(filename)
	name += ".json"
	return name

# 主処理。
def main():
	import sys
	for i in range(1, len(sys.argv)):
		filename = sys.argv[i]
		xword = クロスワード(filename)
		if xword.斜同字():
			print("警告: ファイル「" + filename + "」は、斜同字です。")
		# TODO: ここでxwordに対して何かをする。
		if True:
			xword.画像形式で保存(filename + ".png")
		if False:
			xword.JSON形式で保存(filename + ".json")
		if False:
			import os, shutil
			head, tail = os.path.split(filename)
			new_head = head + "検査済み/"
			if not os.path.exists(new_head):
				os.mkdir(new_head)
			path = new_head + tail
			shutil.move(filename, path)
			print("「" + filename + "」を検査して「" + path + "」に移動しました。")
		if False:
			import os, shutil
			head, tail = os.path.split(filename)
			new_head = head + "検査済み/"
			if not os.path.exists(new_head):
				os.mkdir(new_head)
			path = new_head + tail
			shutil.copy(filename, path)
			print("「" + filename + "」を検査して「" + path + "」にコピーしました。")
if __name__ == "__main__":
	main()
