var data = null;
function copyText(text){
	const el = document.createElement('textarea');
	el.value = text;
	el.setAttribute('readonly', '');
	el.style.position = 'absolute';
	el.style.left = '-9999px';
	document.body.appendChild(el);
	el.select();
	document.execCommand('copy');
	document.body.removeChild(el);
}
function shuffle(array) {
	var currentIndex = array.length, temporaryValue, randomIndex;
	while (0 !== currentIndex) {
		randomIndex = Math.floor(Math.random() * currentIndex);
		currentIndex -= 1;
		temporaryValue = array[currentIndex];
		array[currentIndex] = array[randomIndex];
		array[randomIndex] = temporaryValue;
	}
	return array;
}
function text2table(text) {
	text = text.replace(/━/g, "");
	text = text.replace(/┏┓\n┃/g, '<table class="data" align="center"><tr>');
	text = text.replace(/┃\n┗┛/g, "</tr></table>");
	text = text.replace(/┃\n┃/g, "</tr><tr>");
	text = text.replace(/■/g, '<td class="black"></td>');
	text = text.replace(/　/g, '<td class="white"></td>');
	return text;
}
function refresh(target){
	if (data === null) {
		return;
	}
	shuffle(data);
	var html = "";
	for (var i = 0; i < data.length; ++i) {
		var item = data[i];
		if (target == "horizontal") {
			if (item['num_columns'] <= item['num_rows'])
				continue;
		}
		else if (target == "vertical") {
			if (item['num_columns'] >= item['num_rows'])
				continue;
		}
		else {
			if (item['num_columns'] != item['num_rows'])
				continue;
		}
		if (target == "small") {
			if (item['num_columns'] >= 8 || item['num_rows'] >= 8)
				continue;
		}
		if (target == "medium") {
			if (!(8 <= item['num_columns'] && item['num_columns'] <= 12 &&
				  8 <= item['num_rows'] && item['num_rows'] <= 12))
			{
				continue;
			}
		}
		if (target == "large") {
			if (!(item['num_columns'] >= 13 && item['num_rows'] >= 13))
				continue;
		}
		html += '<div class="item">';
		html += '<p class="center">' + item['num_columns'] + " x " + item['num_rows'] + "</p>";
		html += text2table(item['data'].join("\n"));
		html += "<p class=\"center\"><a href=\"javascript:copyText('" + item['data'].join("\\n") + "')\"><img src=\"copy.png\" alt=\"\" />コピー</a></p>"
		html += '</div>';
	}
	$("#contents").html(html);
}
$(function(){
	$.getJSON("./data.json", function(json) {
		data = json;
		refresh("");
	});
});
