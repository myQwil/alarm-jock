#!/usr/bin/env python3

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk

class PasswordDialog(Gtk.Dialog):
	def __init__(self, parent):
		super().__init__(title='Password', transient_for=parent, flags=0)
		self.add_buttons(
		  Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL
		, Gtk.STOCK_OK    , Gtk.ResponseType.OK
		)
		self.connect('key-press-event', self.on_key_press_event)
		self.connect('response', self.on_response)
		self.set_default_size(150, 100)
		self.result = ''

		label = Gtk.Label(label="What's the password?")
		self.entry = Gtk.Entry()
		self.entry.set_visibility(False)

		box = self.get_content_area()
		box.add(label)
		box.add(self.entry)
		self.show_all()

	def on_response(self, widget, response_id):
		self.result = self.entry.get_text()

	def get_result(self):
		return self.result

	def on_key_press_event(self, widget, event):
		if event.keyval == Gdk.KEY_Return:
			self.response(Gtk.ResponseType.OK)

if __name__ == '__main__':
	dialog = PasswordDialog(None)
	dialog.run()
	print(dialog.get_result())
	dialog.destroy()
