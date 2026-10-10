#!/usr/bin/python3
"""Private D-Bus file-chooser response fixture; never registers on the desktop bus."""
import json
import os
from pathlib import Path
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

if os.environ.get('DISPLAY') in (None,':0',':0.0') or not os.environ.get('DBUS_SESSION_BUS_ADDRESS','').startswith('unix:path=/tmp/dbus-'):
    raise SystemExit('Use a private xvfb-run and dbus-run-session')
DBusGMainLoop(set_as_default=True)
bus=dbus.SessionBus()
name=dbus.service.BusName('org.freedesktop.portal.Desktop',bus)
responses=json.loads(os.environ['BC_QA_ROM_RESPONSES'])

class Portal(dbus.service.Object):
    @dbus.service.method('org.freedesktop.DBus.Properties',in_signature='ss',out_signature='v')
    def Get(self,interface,key):
        if interface!='org.freedesktop.portal.FileChooser' or key!='version':raise ValueError('Unexpected property request')
        return dbus.UInt32(3,variant_level=1)

    @dbus.service.method('org.freedesktop.portal.FileChooser',in_signature='ssa{sv}',out_signature='o',sender_keyword='sender')
    def OpenFile(self,parent,title,options,sender):
        if not responses:raise ValueError('Unexpected additional file chooser request')
        value=responses.pop(0)
        print(json.dumps({'event':'qa_rom_chooser','title':str(title),'filters':str(options.get('filters')),'selected':value}),flush=True)
        if isinstance(value,dict) and value.get('error'):
            raise dbus.exceptions.DBusException('QA chooser unavailable',name='org.freedesktop.portal.Error.Failed')
        path='/org/freedesktop/portal/desktop/request/'+sender[1:].replace('.','_')+'/'+str(options['handle_token'])
        def respond():
            message=dbus.lowlevel.SignalMessage(path,'org.freedesktop.portal.Request','Response')
            message.set_destination(sender)
            result={} if value is None else {'uris':dbus.Array([Path(value).resolve().as_uri()],signature='s')}
            message.append(dbus.UInt32(1 if value is None else 0),dbus.Dictionary(result,signature='sv'),signature='ua{sv}')
            bus.send_message(message)
            return False
        GLib.timeout_add(200,respond)
        return dbus.ObjectPath(path)

portal=Portal(bus,'/org/freedesktop/portal/desktop')
print('Private ROM portal ready',flush=True)
GLib.MainLoop().run()
