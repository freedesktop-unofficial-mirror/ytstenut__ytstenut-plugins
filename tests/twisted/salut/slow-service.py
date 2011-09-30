#!/usr/bin/env python
#
# Copyright (C) 2011 Intel Corp.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA

import dbus

from salutservicetest import call_async, EventPattern, assertEquals, \
    ProxyWrapper, assertNotEquals, assertSameSets
from saluttest import exec_test, wait_for_contact_in_publish, make_result_iq, \
    sync_stream
import salutconstants as cs
import yconstants as ycs
from caps_helper import *

from twisted.words.xish import xpath
from twisted.words.xish.domish import Element
from twisted.words.xish import domish

from avahitest import AvahiAnnouncer, AvahiListener
from avahitest import get_host_name
from xmppstream import setup_stream_listener, connect_to_stream

CLIENT_NAME = 'il-cliente-del-futuro'

banshee = {
    'urn:ytstenut:capabilities#org.gnome.Banshee':
    {'type': ['application'],
     'name': ['en_GB/Banshee Media Player',
              'fr/Banshee Lecteur de Musique'],
     'capabilities': ['urn:ytstenut:capabilities:yts-caps-audio',
                      'urn:ytstenut:data:jingle:rtp']
     }
}

evince = {
    'urn:ytstenut:capabilities#org.gnome.Evince':
    {'type': ['application'],
     'name': ['en_GB/Evince Picture Viewer',
              'fr/Evince uh, ow do you say'],
     'capabilities': ['urn:ytstenut:capabilities:pics'],
     }
}

def test(q, bus, conn):
    forbidden = [EventPattern('dbus-signal', signal='ServiceAdded'),
                 EventPattern('dbus-signal', signal='ServiceRemoved')]
    q.forbid_events(forbidden)

    conn.Connect()

    q.expect('dbus-signal', signal='StatusChanged', args=[0, 0])

    # announce a contact with the right caps
    ver = compute_caps_hash([], [], banshee)
    txt_record = { "txtvers": "1", "status": "avail",
        "node": CLIENT_NAME, "ver": ver, "hash": "sha-1"}
    contact_name = "test-service@" + get_host_name()
    listener, port = setup_stream_listener(q, contact_name)

    announcer = AvahiAnnouncer(contact_name, "_presence._tcp", port, txt_record)

    handle = wait_for_contact_in_publish(q, bus, conn, contact_name)

    # this is the first presence, Salut connects to the contact
    e = q.expect('incoming-connection', listener=listener)
    incoming = e.connection

    # Salut looks up its capabilities
    event = q.expect('stream-iq', connection=incoming,
        query_ns=ns.DISCO_INFO)
    query_node = xpath.queryForNodes('/iq/query', event.stanza)[0]
    assertEquals(CLIENT_NAME + '#' + ver, query_node.attributes['node'])

    contact_handle = conn.RequestHandles(cs.HT_CONTACT, [contact_name])[0]

    # send good reply
    result = make_result_iq(event.stanza)
    query = result.firstChildElement()
    query['node'] = CLIENT_NAME + '#' + ver
    x = query.addElement((ns.X_DATA, 'x'))
    x['type'] = 'result'

    # FORM_TYPE
    field = x.addElement((None, 'field'))
    field['var'] = 'FORM_TYPE'
    field['type'] = 'hidden'
    field.addElement((None, 'value'), content='urn:ytstenut:capabilities#org.gnome.Banshee')

    # type
    field = x.addElement((None, 'field'))
    field['var'] = 'type'
    field.addElement((None, 'value'), content='application')

    # name
    field = x.addElement((None, 'field'))
    field['var'] = 'name'
    field.addElement((None, 'value'), content='en_GB/Banshee Media Player')
    field.addElement((None, 'value'), content='fr/Banshee Lecteur de Musique')

    # capabilities
    field = x.addElement((None, 'field'))
    field['var'] = 'capabilities'
    field.addElement((None, 'value'), content='urn:ytstenut:capabilities:yts-caps-audio')
    field.addElement((None, 'value'), content='urn:ytstenut:data:jingle:rtp')

    incoming.send(result)

    # this will be fired as text channel caps will be fired
    q.expect('dbus-signal', signal='ContactCapabilitiesChanged',
             predicate=lambda e: contact_handle in e.args[0])

    # add evince
    tmp = banshee.copy()
    tmp.update(evince)
    ver = compute_caps_hash([], [], tmp)
    txt_record['ver'] = ver
    announcer.update(txt_record)

    # Salut looks up our capabilities
    event = q.expect('stream-iq', connection=incoming,
        query_ns='http://jabber.org/protocol/disco#info')
    query_node = xpath.queryForNodes('/iq/query', event.stanza)[0]
    assert query_node.attributes['node'] == \
        CLIENT_NAME + '#' + txt_record['ver']

    # send good reply
    result['id'] = event.stanza['id']
    query['node'] = CLIENT_NAME + '#' + ver

    x = query.addElement((ns.X_DATA, 'x'))
    x['type'] = 'result'

    # FORM_TYPE
    field = x.addElement((None, 'field'))
    field['var'] = 'FORM_TYPE'
    field['type'] = 'hidden'
    field.addElement((None, 'value'), content='urn:ytstenut:capabilities#org.gnome.Evince')

    # type
    field = x.addElement((None, 'field'))
    field['var'] = 'type'
    field.addElement((None, 'value'), content='application')

    # name
    field = x.addElement((None, 'field'))
    field['var'] = 'name'
    field.addElement((None, 'value'), content='en_GB/Evince Picture Viewer')
    field.addElement((None, 'value'), content='fr/Evince uh, ow do you say')

    # capabilities
    field = x.addElement((None, 'field'))
    field['var'] = 'capabilities'
    field.addElement((None, 'value'), content='urn:ytstenut:capabilities:pics')

    incoming.send(result)

    # this will be fired as text channel caps will be fired
    q.expect('dbus-signal', signal='ContactCapabilitiesChanged',
             predicate=lambda e: contact_handle in e.args[0])

    # now finally ensure the sidecar
    path, props = conn.Future.EnsureSidecar(ycs.STATUS_IFACE)
    assertEquals({}, props)

    status = ProxyWrapper(bus.get_object(conn.bus_name, path),
                          ycs.STATUS_IFACE, {})

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({contact_name: {
                'org.gnome.Banshee':
                    ('application',
                     {'en_GB': 'Banshee Media Player',
                      'fr': 'Banshee Lecteur de Musique'},
                     ['urn:ytstenut:capabilities:yts-caps-audio',
                      'urn:ytstenut:data:jingle:rtp']),
                  'org.gnome.Evince':
                      ('application',
                       {'en_GB': 'Evince Picture Viewer',
                        'fr': 'Evince uh, ow do you say'},
                       ['urn:ytstenut:capabilities:pics'])}
                }, discovered)

    # sweet.

if __name__ == '__main__':
    exec_test(test)
