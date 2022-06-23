const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>ESP Web Server</title>
  <style>
		:root {
			--hue: 0;
			--toggle-on: #369c97;
			--toggle-off: #525252;
		}
    html {
			font-family: Arial;
			display: inline-block;
			text-align: center;
			color: #cacaca;
			background-color: #001722;
		}
    h2 {
			font-size: 2.3rem;
		}
    p {
			font-size: 1.5rem;
		}
    body {
			max-width: 400px;
			margin:0px auto;
			padding-bottom: 25px;
		}
    .slider {
			-webkit-appearance: none;
			height: 25px;
			margin: 18px 0;
			background: #FFD65C;
			border: none;
			border-radius: 5px;
      outline: none;
			-webkit-transition: .2s;
			transition: opacity .2s;
		}
		.tableSlider {
			width: 92%%;
			position: relative;
			right: 4%%;
		}
    .slider::-webkit-slider-thumb {
			-webkit-appearance: none;
			appearance: none;
			width: 25px;
			height: 35px;
			background: #ffffff80;
			border: none;
			border-radius: 5px;
			cursor: pointer;
		}
    .slider::-moz-range-thumb {
			width: 25px;
			height: 35px;
			background: #ffffff80;
			border: none;
			border-radius: 5px;
			cursor: pointer;
		}
    #hueSlider {
      background: #FFD65C; /* For browsers that do not support gradients */
      background: -webkit-linear-gradient(to right,hsl(0,100%%,50%%),hsl(60,100%%,50%%),hsl(120,100%%,50%%),hsl(180,100%%,50%%),hsl(240,100%%,50%%),hsl(300,100%%,50%%),hsl(360,100%%,50%%)); /* For Safari 5.1 to 6.0 */
      background: -moz-linear-gradient(to right,hsl(0,100%%,50%%),hsl(60,100%%,50%%),hsl(120,100%%,50%%),hsl(180,100%%,50%%),hsl(240,100%%,50%%),hsl(300,100%%,50%%),hsl(360,100%%,50%%)); /* For Firefox 3.6 to 15 */
      background: linear-gradient(to right,hsl(0,100%%,50%%),hsl(60,100%%,50%%),hsl(120,100%%,50%%),hsl(180,100%%,50%%),hsl(240,100%%,50%%),hsl(300,100%%,50%%),hsl(360,100%%,50%%));
    }
		#satSlider {
			background: #FFD65C; /* For browsers that do not support gradients */
      background: -webkit-linear-gradient(to right,white,hsl(var(--hue),100%%,50%%)); /* For Safari 5.1 to 6.0 */
      background: -moz-linear-gradient(to right,white,hsl(var(--hue),100%%,50%%)); /* For Firefox 3.6 to 15 */
      background: linear-gradient(to right,white,hsl(var(--hue),100%%,50%%));
		}
		#valSlider {
			background: #FFD65C; /* For browsers that do not support gradients */
      background: -webkit-linear-gradient(to right,black,white); /* For Safari 5.1 to 6.0 */
      background: -moz-linear-gradient(to right,black,white); /* For Firefox 3.6 to 15 */
      background: linear-gradient(to right,black,white);
		}
		button {
			border: none;
			border-radius: 0.4em;
			margin: 0 1.5rem;
			width: 7em;
			height: 3em;
			background-color: var(--toggle-on);
			cursor: pointer;
			color: #ffffff;
			font-size: 1rem;
			text-shadow: 1px 1px black;
		}
		table {
			width: 100%%;
			margin-bottom: 2rem;
		}
		.tableLabel {
			font-size: 1.5rem;
			width: 4rem;
		}
  </style>
</head>
<body onload="setup()">
  <h2>ESP Web Server</h2>

	<table>
		<tr>
			<td class="tableLabel">H</td>
			<td><input type="range" oninput="updateSlider()" id="hueSlider" min="0" max="255" value="%HUE_VAL%" step="1" class="slider tableSlider"></td>
		</tr>
		<tr>
			<td class="tableLabel">S</td>
			<td><input type="range" oninput="updateSlider()" id="satSlider" min="0" max="255" value="%SAT_VAL%" step="1" class="slider tableSlider"></td>
		</tr>
		<tr>
			<td class="tableLabel">V</td>
			<td><input type="range" oninput="updateSlider()" id="valSlider" min="0" max="255" value="%VAL_VAL%" step="1" class="slider tableSlider"></td>
		</tr>
	</table>

	<button type="button" id="ledToggle" value="%LED_TOGGLE%" onclick="toggleLed()">Lights</button>
  <button type="button" id="clockToggle" value="%CLOCK_TOGGLE%" onclick="toggleClock()">Clock</button>

	<p style="margin: 2rem 0 0rem;">Animation State</p>
	<table id="stateTable">
		<tr>
			<td class="tableLabel" id="stateText">%STATE_VAL%</td>
			<td><input type="range" oninput="updateState(this)" id="stateSlider" min="0" max="2" value="%STATE_VAL%" step="1" class="slider tableSlider"></td>
		</tr>
		<tr id="speedRow" style="display: none;">
			<td class="tableLabel" id="speedText">Sp</td>
			<td><input type="range" oninput="updateSpeed(this)" id="speedSlider" min="0" max="100" value="%RSPEED_VAL%" step="1" class="slider tableSlider"></td>
		</tr>
	</table>
	
<script>
	var standardState = "0";
	var rainbowCycle = "1";

	function setup() {
		// update hue color
		var hue = parseInt(document.getElementById("hueSlider").value);
		var root = document.documentElement;
		root.style.setProperty("--hue", 360*hue/255);

		// update button states
		updateButton(document.getElementById("ledToggle"));
		updateButton(document.getElementById("clockToggle"));

		showSpeed(document.getElementById("stateSlider").value == rainbowCycle);
	}
	function updateSlider() {
		var hue = parseInt(document.getElementById("hueSlider").value);
		var sat = parseInt(document.getElementById("satSlider").value);
		var val = parseInt(document.getElementById("valSlider").value);

		var root = document.documentElement;
		root.style.setProperty("--hue", 360*hue/255);

		var hexcode = (hue<<16) + (sat<<8) + val;
		console.log("0x" + hexcode.toString(16));

		xhrSend("/hex?value="+hexcode);
	}
	function toggleClock() {
		var button = document.getElementById("clockToggle");
		button.value = button.value == "1" ? "0" : "1";
		updateButton(button);

		xhrSend("/toggleclock?value="+button.value);
	}
	function toggleLed() {
		var button = document.getElementById("ledToggle");
		button.value = button.value == "1" ? "0" : "1";
		updateButton(button);

		xhrSend("/toggleled?value="+button.value);
	}
	function updateButton(button) {
		var rootStyle = getComputedStyle(document.documentElement);
		var on_color = rootStyle.getPropertyValue("--toggle-on");
		var off_color = rootStyle.getPropertyValue("--toggle-off");
		
		button.style.setProperty("background-color", button.value == "1" ? on_color : off_color);
	}
	function updateState(element) {
		var value = element.value;
		document.getElementById("stateText").innerHTML = value;

		showSpeed(value == rainbowCycle);

		xhrSend("/state?value="+element.value);
	}
	function showSpeed(create) {
		var row = document.getElementById("speedRow");
		if (create) {
			row.style.setProperty("display", "table-row");
		} else {
			row.style.setProperty("display", "none");
		}
	}
	function updateSpeed(element) {
		xhrSend("/speed?value="+element.value);
	}
	function xhrSend(msg) {
		var xhr = new XMLHttpRequest();
		xhr.open("GET", msg, true);
		xhr.send();
	}
</script>
</body>
</html>
)rawliteral";