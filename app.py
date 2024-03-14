import mido
import time
from controller_config import *
from colors import *

def noop(*args, **kwargs):
    pass

def colorComponentsToColorByte(components):
    return components['red'] + (components['green'] << 4)

def colorByteToColorComponents(byte):
    return {
        'red': byte & 15,
        'green': byte >> 4,
    }

def setLedColor(port, ledIndex, color):
    colorByte = colorComponentsToColorByte(color)
    templateIndex = 0
    msg = mido.Message('sysex', data=[0, 32, 41, 2, 17, 120, templateIndex, ledIndex, colorByte])
    port.send(msg)

GATE_LINE_MODES = [
    {
        'name': 'GATE',
        'current_step_color': COLORS['GREEN_3'],
        'other_step_color': COLORS['OFF'],
        'duty_cycle': 0.9,
    },
    {
        'name': 'TIE',
        'current_step_color': COLORS['YELLOW_3'],
        'other_step_color': COLORS['YELLOW_2'],
        'duty_cycle': 1,
    },
    {
        'name': 'SILENT',
        'current_step_color': COLORS['RED_3'],
        'other_step_color': COLORS['RED_2'],
        'duty_cycle': 0,
    },
]

STEP_LINE_MODES = [
    {
        'name': 'STEP',
        'current_step_color': COLORS['GREEN_3'],
        'other_step_color': COLORS['OFF'],
        'played': True,
    },
    {
        'name': 'SKIP',
        'current_step_color': COLORS['YELLOW_3'],
        'other_step_color': COLORS['YELLOW_2'],
        'played': False,
    },
    {
        'name': 'RESET',
        'current_step_color': COLORS['RED_3'],
        'other_step_color': COLORS['RED_2'],
        'played': False,
    },
    {
        'name': 'STOP',
        'current_step_color': COLORS['GREEN_3'],
        'other_step_color': COLORS['GREEN_1'],
        'played': True,
    },
]

class Button:
    def __init__(self, cc_number, led_index, modes=[], mode_index=0, cc_value=None, midi_channel=0, is_current_step=False, init=noop):
        controllers.append(self)
        self.cc_number = cc_number
        self.cc_value = cc_value
        self.led_index = led_index
        self.modes = modes
        self.mode_count = len(modes)
        self.mode_index = mode_index
        self.midi_channel = midi_channel
        self.is_current_step = is_current_step
        self.on_button_down_callbacks = []
        self.on_button_up_callbacks = []
        self.is_current_step_callbacks = []
        init(self)

    def set_value(self, midi_channel, cc_number, cc_value):
        if midi_channel != self.midi_channel:
            return

        if cc_number != self.cc_number:
            return

        self.cc_value = cc_value
        if cc_value < 64:
            for callback in self.on_button_up_callbacks:
                callback(self)
        elif cc_value >= 64:
            for callback in self.on_button_down_callbacks:
                callback(self)

    def set_is_current_step(self, is_current_step):
        old_is_current_step = self.is_current_step
        self.is_current_step = is_current_step
        if old_is_current_step != is_current_step:
            for callback in self.is_current_step_callbacks:
                callback(self)

    def get_step_led_color(self):
        if self.is_current_step:
            return self.modes[self.mode_index]['current_step_color']
        else:
            return self.modes[self.mode_index]['other_step_color']

    def set_led_color(self, color=None):
        if self.led_index is None:
            return

        if color is None:
            return

        setLedColor(outport, self.led_index, color)

class RadioButtons():
    def __init__(self, buttons, selected_index=0, selected_color=COLORS['GREEN_3'], unselected_color=COLORS['OFF']):
        self.buttons = buttons
        self.selected_index = selected_index
        self.selected_color = selected_color
        self.unselected_color = unselected_color
        self.selected_index_callbacks = []
        self.set_led_colors()

        for i, button in enumerate(buttons):
            def on_button_down_callback(button, i=i):
                self.set_selected_index(i)
            button.on_button_down_callbacks.append(on_button_down_callback)

    def set_selected_index(self, selected_index):
        old_selected_index = self.selected_index
        self.selected_index = selected_index
        self.set_led_colors()
        if old_selected_index != selected_index:
            for callback in self.selected_index_callbacks:
                callback(self)

    def set_led_colors(self):
        for i, button in enumerate(self.buttons):
            if i == self.selected_index:
                button.set_led_color(self.selected_color)
            else:
                button.set_led_color(self.unselected_color)

class Controller:
    def __init__(self, cc_number, led_index, cc_value=None, midi_channel=0, is_current_step=False):
        controllers.append(self)
        self.cc_number = cc_number
        self.led_index = led_index
        self.cc_value = cc_value
        self.midi_channel = midi_channel
        self.is_current_step = is_current_step
        self.set_led_color()

    def set_value(self, midi_channel, cc_number, cc_value):
        if midi_channel != self.midi_channel:
            return

        if cc_number != self.cc_number:
            return

        old_cc_value = self.cc_value
        self.cc_value = cc_value
        if old_cc_value is None and cc_value is not None:
            self.set_led_color()

    def set_is_current_step(self, is_current_step):
        old_is_current_step = self.is_current_step
        self.is_current_step = is_current_step
        if old_is_current_step != is_current_step:
            self.set_led_color()

    def get_led_color(self):
        if self.cc_value is None:
            if self.is_current_step:
                return COLORS['RED_3']
            else:
                return COLORS['RED_1']
        else:
            if self.is_current_step:
                return COLORS['GREEN_3']
            else:
                return COLORS['GREEN_1']

    def set_led_color(self):
        if self.led_index is None:
            return

        setLedColor(outport, self.led_index, self.get_led_color())

controllers = []

class Sequencer:
    def __init__(self, total_steps, clock, current_step=0):
        self.total_steps = total_steps
        self.clock = clock
        self.current_step = current_step

        self.step_controllers = [[] for i in range(total_steps)]

        self.controllers = []
        for i, controller in enumerate(SEND_A):
            controller_obj = Controller(
                cc_number=controller['cc_number'],
                midi_channel=0,
                led_index=controller['led_index'],
                is_current_step=i == current_step,
            )
            self.controllers.append(controller_obj)
            self.step_controllers[i].append(controller_obj)
        for i, controller in enumerate(SEND_B):
            controller_obj = Controller(
                cc_number=controller['cc_number'],
                midi_channel=0,
                led_index=controller['led_index'],
                is_current_step=i == current_step,
            )
            self.controllers.append(controller_obj)
            self.step_controllers[i].append(controller_obj)
        for i, controller in enumerate(PAN_DEVICE):
            controller_obj = Controller(
                cc_number=controller['cc_number'],
                midi_channel=0,
                led_index=controller['led_index'],
                is_current_step=i == current_step,
            )
            self.controllers.append(controller_obj)
            self.step_controllers[i].append(controller_obj)
        for i, controller in enumerate(FADERS):
            controller_obj = Controller(
                cc_number=controller['cc_number'],
                midi_channel=0,
                led_index=controller['led_index'],
                is_current_step=i == current_step,
            )
            self.controllers.append(controller_obj)
            self.step_controllers[i].append(controller_obj)

        self.buttons = []
        self.step_buttons = [[] for i in range(total_steps)]
        self.gate_line_buttons = []

        def on_button_down_callback(button):
            button.mode_index = (button.mode_index + 1) % button.mode_count
            button.set_led_color(button.get_step_led_color())

        def on_is_current_step_callback(button):
            button.set_led_color(button.get_step_led_color())

        for i, button in enumerate(TRACK_FOCUS):
            button_obj = Button(
                cc_number=button['cc_number'],
                midi_channel=0,
                led_index=button['led_index'],
                modes=GATE_LINE_MODES,
                is_current_step=i == current_step,
                init=lambda button: button.set_led_color(button.get_step_led_color())
            )
            button_obj.on_button_down_callbacks.append(on_button_down_callback)
            button_obj.is_current_step_callbacks.append(on_is_current_step_callback)
            self.buttons.append(button_obj)
            self.step_controllers[i].append(button_obj)
            self.gate_line_buttons.append(button_obj)
        self.step_line_buttons = []
        for i, button in enumerate(TRACK_CONTROL):
            button_obj = Button(
                cc_number=button['cc_number'],
                midi_channel=0,
                led_index=button['led_index'],
                modes=STEP_LINE_MODES,
                is_current_step=i == current_step,
                init=lambda button: button.set_led_color(button.get_step_led_color())
            )
            button_obj.on_button_down_callbacks.append(on_button_down_callback)
            button_obj.is_current_step_callbacks.append(on_is_current_step_callback)
            self.buttons.append(button_obj)
            self.step_controllers[i].append(button_obj)
            self.step_line_buttons.append(button_obj)

        mode_buttons = RadioButtons(
            buttons=[
                Button(
                    midi_channel=0,
                    cc_number=DEVICE['cc_number'],
                    led_index=DEVICE['led_index'],
                ),
                Button(
                    midi_channel=0,
                    cc_number=MUTE['cc_number'],
                    led_index=MUTE['led_index'],
                ),
                Button(
                    midi_channel=0,
                    cc_number=SOLO['cc_number'],
                    led_index=SOLO['led_index'],
                ),
                Button(
                    midi_channel=0,
                    cc_number=RECORD_ARM['cc_number'],
                    led_index=RECORD_ARM['led_index'],
                ),
            ]
        )

        self.clock.on_tick(self.step)

    def get_next_step(self, current_step, initial_step=None):
        if initial_step == current_step:
            return current_step

        if initial_step is None:
            initial_step = current_step

        next_step = (current_step + 1) % self.total_steps
        current_step_button = self.step_line_buttons[current_step]
        current_step_mode = current_step_button.modes[current_step_button.mode_index]
        next_step_button = self.step_line_buttons[next_step]
        next_step_mode = next_step_button.modes[next_step_button.mode_index]

        if current_step_mode['name'] == 'STOP':
            return current_step

        if next_step_mode['name'] == 'RESET':
            for step, step_line_button in enumerate(self.step_line_buttons):
                if step_line_button.modes[step_line_button.mode_index]['played']:
                    return step
            return current_step
        elif next_step_mode['name'] == 'SKIP':
            return self.get_next_step(next_step, initial_step)
        elif next_step_mode['name'] == 'STOP':
            return next_step
        elif next_step_mode['name'] == 'STEP':
            return next_step
        else:
            return current_step

    def get_step_info(self, step):
        controllers = self.step_controllers[step]
        note_controller = controllers[0]
        cv1_controller = controllers[1]
        duty_cycle = 0.9
        return {
            'note': note_controller.cc_value,
            'cv1': cv1_controller.cc_value,
            'duty_cycle': duty_cycle,
        }

    def trigger(self):
        print('trigger on')
        self.clock.once_time(0, lambda: print('trigger off'))

    def gate(self, info):
        # TODO: Scale and quantize
        note = 64 if info['note'] is None else info['note']
        cv1 = 64 if info['cv1'] is None else info['cv1']
        print('gate on with note', note, 'cv1', cv1)
        self.clock.once_time(info['duty_cycle'] * self.clock.interval, lambda: print('gate off'))

    def output(self, info):
        self.trigger()
        self.gate(info)

    def step(self, step=None):
        if step is None:
            step = self.get_next_step(self.current_step)
        self.current_step = step

        self.output(self.get_step_info(step))

        for i, step_controllers in enumerate(self.step_controllers):
            for controller in step_controllers:
                controller.set_is_current_step(i == step)

print('Output ports:', mido.get_output_names())
launch_control_xl_output = [port for port in mido.get_output_names() if "Launch Control XL" in port][0]
print('Launch Control XL output:', launch_control_xl_output)
outport = mido.open_output(launch_control_xl_output)

print('Input ports:', mido.get_input_names())
launch_control_xl_input = [port for port in mido.get_input_names() if 'Launch Control XL' in port][0]
print('Launch Control XL input:', launch_control_xl_input)
inport = mido.open_input(launch_control_xl_input)

def receive_midi_message():
    msg = inport.poll()
    if msg is None:
        return
    print(msg)
    if msg.is_cc():
        for controller in controllers:
            controller.set_value(msg.channel, msg.control, msg.value)

class Clock:
    def __init__(self, bpm):
        self.bpm = bpm
        self.interval = 60 / bpm
        self.time = time.time()
        self.on_tick_callbacks = []
        self.once_time_callbacks = []

    def set_time(self):
        old_time = self.time
        new_time = time.time()
        diff = new_time - old_time

        for at_time, callback in self.once_time_callbacks:
            if diff >= at_time:
                callback()
                self.once_time_callbacks.remove((at_time, callback))

        if diff >= self.interval:
            self.time = old_time + self.interval
            for callback in self.on_tick_callbacks:
                callback()

    def on_tick(self, callback):
        self.on_tick_callbacks.append(callback)

    def once_time(self, at_time, callback):
        self.once_time_callbacks.append((at_time, callback))

clock = Clock(bpm=120)
sequencer = Sequencer(total_steps=8, clock=clock)

while True:
    receive_midi_message()
    clock.set_time()
    time.sleep(0.001)
