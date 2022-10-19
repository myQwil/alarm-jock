#!/usr/bin/env python3

# Alarm Jock - solve math problems to turn off the snooze alarm

import quiz as q
import password as p
from sys import path
from threading import Thread
from datetime import datetime ,timedelta

path.append(f'{path[0]}/../pylibpd')
import pdmain as pd

import gi
gi.require_version('Gtk' ,'3.0')
from gi.repository import Gtk ,Gdk ,GLib

# Preferences

password = '123'
'password for stopping the alarm noise'

volume = 0.03
'volume level in linear amplitude'

snooze = 30 * 60
'interval between snooze alarms in seconds'

qtotal = 12
'amount of math problems to solve to turn off the snooze alarm'


patch = pd.open(volume=volume ,play=False)
dest_vol  = f'{patch}vol'
dest_tgl  = f'{patch}tgl'
dest_test = f'{patch}test'
dest_done = f'{patch}done'
testing = False
pd.flush = True

def on_bang(dest):
	global testing
	if testing:
		pd.playing ,testing = False ,False
pd.libpd_subscribe(dest_done)
pd.libpd_set_bang_callback(on_bang)

class PdThread(Thread):
	def run(self):
		pd.loop()
		pd.libpd_float(dest_tgl ,0)

class AlarmWindow(Gtk.Window):
	def __init__(self):
		super().__init__(title = 'Alarm Jock')
		self.connect('delete-event' ,self.on_delete)
		self.connect('destroy' ,Gtk.main_quit)
		self.audio = None
		self.timer = None
		self.color = 'off'
		self.n = 1

		# # load alarm preference, if it exists
		self.conf = f'{path[0]}/alarm.txt'
		try:
			f = open(self.conf ,'r')
			alarm = int(float(f.readline()))
			f.close()
		except:
			now = datetime.now() + timedelta(seconds=5)
			alarm = int(now.hour*3600 + now.minute*60 + now.second)

		# # CSS styling
		provider = Gtk.CssProvider()
		Gtk.StyleContext().add_provider_for_screen( Gdk.Screen.get_default()
			,provider ,Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION )
		provider.load_from_data(b'''
			.switch {font-size: 16pt}
			.time   {font-size: 24pt}
			.on     {color: turquoise}
			.off    {color: grey}
			.beep   {color: pink}
			.snooze {color: gold}
		''')

		# # table
		table = Gtk.Table()
		self.add(table)
		i = 0 # row index

		# # alarm switch
		self.switch = Gtk.Switch()
		self.switch.connect('notify::active' ,self.on_switch)
		self.switch.set_active(False)
		table.attach(self.switch ,0 ,6 ,i ,i+1)
		# add a dummy label to make the switch taller
		dummy = Gtk.Label()
		context = dummy.get_style_context()
		context.add_class('switch')
		table.attach(dummy ,0 ,6 ,i ,i+1)
		i += 1

		# # clock label
		self.lbl_clock = Gtk.Label(label = datetime.now().strftime('%H:%M:%S'))
		self.context = self.lbl_clock.get_style_context()
		self.context.add_class('time')
		self.context.add_class(self.color)
		table.attach(self.lbl_clock ,0 ,6 ,i ,i+1)
		i += 1

		# # time labels
		table.attach(Gtk.Label(label = 'Hour') ,0 ,2 ,i ,i+1)
		table.attach(Gtk.Label(label = 'Min')  ,2 ,4 ,i ,i+1)
		table.attach(Gtk.Label(label = 'Sec')  ,4 ,6 ,i ,i+1)
		i += 1

		# # spin buttons
		hr = alarm / 3600
		mn = hr % 1 * 60
		sc = mn % 1 * 60

		# # hours
		adjust = Gtk.Adjustment(value=int(hr) ,lower=0 ,upper=23 ,step_increment=1)
		self.hur = Gtk.SpinButton()
		self.hur.set_adjustment(adjust)
		table.attach(self.hur ,0 ,2 ,i ,i+1)

		# # minutes
		adjust = Gtk.Adjustment(value=int(mn) ,lower=0 ,upper=59 ,step_increment=1)
		self.min = Gtk.SpinButton()
		self.min.set_adjustment(adjust)
		table.attach(self.min ,2 ,4 ,i ,i+1)

		# # seconds
		adjust = Gtk.Adjustment(value=int(sc) ,lower=0 ,upper=59 ,step_increment=1)
		self.sec = Gtk.SpinButton()
		self.sec.set_adjustment(adjust)
		table.attach(self.sec ,4 ,6 ,i ,i+1)
		i += 1

		# # button for testing the max volume
		button = Gtk.Button(label='Test Volume')
		button.connect('clicked' ,self.on_test_clicked)
		table.attach(button ,0 ,3 ,i ,i+1)

		# # button for saving the alarm time
		button = Gtk.Button(label='Save Alarm')
		button.connect('clicked' ,self.on_save_clicked)
		table.attach(button ,3 ,6 ,i ,i+1)

		GLib.timeout_add(999 ,self.tick)


	def change_color(self ,color):
		self.context.remove_class(self.color)
		self.context.add_class(color)
		self.color = color

	def set_alarm(self ,state ,snooze=False):
		if state:
			if pd.playing == True:
				return
			pd.playing = True
			self.change_color('beep')
			pd.libpd_float(dest_tgl ,1)
			if not self.audio or not self.audio.is_alive():
				self.audio = PdThread()
				self.audio.start()
		else:
			pd.playing = False
			if snooze:
				self.change_color('snooze')
			else:
				self.change_color('off')
				GLib.source_remove(self.timer)
				self.timer = None
			pd.libpd_float(dest_tgl ,0)
			self.audio.join()

	def pwd_success(self):
		dialog = p.PasswordDialog(self)
		result = (dialog.run() == Gtk.ResponseType.OK
			and dialog.get_result() == password)
		if result:
			self.set_alarm(False ,snooze=True)
		dialog.destroy()
		return result

	def quiz_success(self):
		dialog = q.QuizDialog(self ,qtotal ,self.n)
		result = dialog.run() == Gtk.ResponseType.OK
		self.n = 1 if result else dialog.get_n()
		if result:
			self.set_alarm(False)
		dialog.destroy()
		return result

	def on_switch(self ,switch ,gparam):
		if switch.get_state():
			if not self.timer:
				self.change_color('on')
		elif pd.playing:
			switch.set_active(True)
			self.pwd_success()
		elif self.timer:
			if not self.quiz_success():
				switch.set_active(True)
		else:
			self.change_color('off')

	def snooze(self):
		if not pd.playing:
			self.set_alarm(True)
		return True # repeat timeout indefinitely

	def tick(self):
		#  get current time for comparison to alarm time
		now = datetime.now()
		self.lbl_clock.set_label(now.strftime('%H:%M:%S'))
		#  if the alarm state is true,
		#  wait for the alarm time to equal current time then trigger alarm
		if not pd.playing and self.switch.get_state() \
		and now.hour   == self.hur.get_value() \
		and now.minute == self.min.get_value() \
		and now.second == self.sec.get_value():
			self.set_alarm(True)
			if self.timer:
				GLib.source_remove(self.timer)
			self.timer = GLib.timeout_add(snooze * 1000 ,self.snooze)
		return True # repeat timeout indefinitely

	def on_test_clicked(self ,widget):
		global testing
		pd.libpd_bang(dest_test)
		if not pd.playing:
			pd.playing ,testing = True ,True
		if not self.audio or not self.audio.is_alive():
			self.audio = PdThread()
			self.audio.start()

	def on_save_clicked(self ,widget):
		f = open(self.conf ,'w')
		hr = int(self.hur.get_value())
		mn = int(self.min.get_value())
		sc = int(self.sec.get_value())
		alarm = int(hr*3600 + mn*60 + sc)
		f.write(f'{alarm}\n{hr}:{mn:02d}:{sc:02d}\n')
		f.close()

	def on_delete(self ,widget ,*data):
		return (pd.playing and not self.pwd_success()) \
		    or (self.timer and not self.quiz_success())


win = AlarmWindow()
win.show_all()
Gtk.main()
