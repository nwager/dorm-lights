import re

def percent():
	with open('./index.html', 'r') as f:

		s = f.read()

		start = s.find('<style>') + len('<style>')
		end = s.find('</style>')

		contents = s[start:end].replace('%','%%')

		before = s[:start]
		after = s[end:]

		html = before + contents + after
		with open('/Users/noahwager/Documents/Arduino/dorm_lights/rawhtml.h', 'w') as h:
			first = 'const char index_html[] PROGMEM = R"rawliteral(\n'
			last = '\n)rawliteral";'
			
			full = first + html + last
			h.write(full)

percent()