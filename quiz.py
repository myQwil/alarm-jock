#!/usr/bin/env python3

from random import random

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

class QuizDialog(Gtk.Dialog):
	def __init__(self, parent, total=3, n=1):
		super().__init__(title='Quiz', transient_for=parent, flags=0)
		self.connect('key-press-event', self.on_key_press_event)
		self.set_default_size(150, 100)
		self.answer = None
		self.total = total
		self.n = n

		self.prob  = Gtk.Label()
		self.entry = Gtk.Entry()
		self.count = Gtk.Label()
		button = Gtk.Button(label='Next')
		button.connect('clicked', self.on_next_clicked)

		box = self.get_content_area()
		box.add(self.prob)
		box.add(self.entry)
		box.add(self.count)
		box.add(button)

		self.new_question()
		self.show_all()

	def get_n(self):
		return self.n

	def new_question(self):
		self.entry.set_text('')
		self.count.set_label(f'{self.n} of {self.total}')

		op = int(4 * random())
		(low, high) = (4, 24) if op < 2 else (2, 12)
		a = int(low + high*random())
		b = int(low + high*random())

		if   op == 0: op = '+'; self.answer = a + b
		elif op == 1: op = '-'; self.answer = a; a += b
		elif op == 2: op = '*'; self.answer = a * b
		elif op == 3: op = '/'; self.answer = a; a *= b
		self.prob.set_label(f"What's {a} {op} {b}?")

	def eval(self):
		text = self.entry.get_text()
		if text.isnumeric() and int(text) == self.answer:
			if self.n == self.total:
				self.n = 1
				self.response(Gtk.ResponseType.OK)
				return
			self.n += 1
		self.new_question()

	def on_next_clicked(self, widget):
		self.set_focus(self.entry)
		self.eval()

	def on_key_press_event(self, widget, event):
		if event.keyval == Gdk.KEY_Return:
			self.eval()

if __name__ == '__main__':
	dialog = QuizDialog(None, 3)
	dialog.run()
	dialog.destroy()
