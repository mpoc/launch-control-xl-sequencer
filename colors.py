COLORS = {
	'OFF': { 'red': 0, 'green': 0 },
	'RED_1': { 'red': 1, 'green': 0 },
	'RED_2': { 'red': 2, 'green': 0 },
	'RED_3': { 'red': 3, 'green': 0 },
	'GREEN_1': { 'red': 0, 'green': 1 },
	'GREEN_2': { 'red': 0, 'green': 2 },
	'GREEN_3': { 'red': 0, 'green': 3 },
	'YELLOW_1': { 'red': 1, 'green': 1 },
	'YELLOW_2': { 'red': 2, 'green': 2 },
	'YELLOW_3': { 'red': 3, 'green': 3 },
	'AMBER_2': { 'red': 2, 'green': 1 },
	'AMBER_3': { 'red': 3, 'green': 2 },
	'DARK_AMBER_3': { 'red': 3, 'green': 1 },
	'GREEN_YELLOW_2': { 'red': 1, 'green': 2 },
	'GREEN_YELLOW_3': { 'red': 2, 'green': 3 },
	'MORE_GREEN_YELLOW_3': { 'red': 1, 'green': 3 },
}

def color_components_to_color_byte(components):
    return components['red'] + (components['green'] << 4)

def color_byte_to_color_components(byte):
    return {
        'red': byte & 15,
        'green': byte >> 4,
    }
