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

from gabbleservicetest import call_async, EventPattern, assertEquals, \
    ProxyWrapper, assertNotEquals, assertSameSets
from gabbletest import exec_test, make_result_iq, sync_stream
import gabbleconstants as cs
import yconstants as ycs
from gabblecaps_helper import *

from twisted.words.xish import xpath
from twisted.words.xish.domish import Element
from twisted.words.xish import domish

CLIENT_NAME = 'il-cliente-del-futuro'

client = 'http://telepathy.im/fake'
client_caps = {'ver': '0.1', 'node': client}
features = [
    ns.JINGLE_015,
    ns.JINGLE_015_AUDIO,
    ns.JINGLE_015_VIDEO,
    ns.GOOGLE_P2P,
    ]
identity = ['client/pc/en/Lolclient 0.L0L']

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

def test(q, bus, conn, stream):
    bare_jid = "test-service@example.com"
    full_jid = bare_jid + "/NeeNawNeeNawIAmAnAmbulance"

    # we don't want these two signalled, ever.
    forbidden = [EventPattern('dbus-signal', signal='ServiceAdded'),
                 EventPattern('dbus-signal', signal='ServiceRemoved')]
    q.forbid_events(forbidden)

    conn.Connect()

    _, e = q.expect_many(EventPattern('dbus-signal', signal='StatusChanged',
                                      args=[0, 1]),
                         EventPattern('stream-iq', query_ns=ns.ROSTER,
                                      iq_type='get', query_name='query'))

    e.stanza['type'] = 'result'

    item = e.query.addElement('item')
    item['jid'] = bare_jid
    item['subscription'] = 'both'

    stream.send(e.stanza)

    q.expect('dbus-signal', signal='ContactListStateChanged',
            args=[cs.CONTACT_LIST_STATE_SUCCESS])

    # announce a contact with the right caps
    contact_handle = conn.RequestHandles(cs.HT_CONTACT, [bare_jid])[0]

    client_caps['ver'] = compute_caps_hash(identity, features, banshee)
    presence_and_disco(q, conn, stream, full_jid, True, client, client_caps,
                       features, identity, banshee, True, None)

    # this will be fired as text channel caps will be fired
    q.expect('dbus-signal', signal='ContactCapabilitiesChanged',
             predicate=lambda e: contact_handle in e.args[0])

    # add evince
    tmp = banshee.copy()
    tmp.update(evince)
    client_caps['ver'] = compute_caps_hash(identity, features, tmp)
    presence_and_disco(q, conn, stream, full_jid, True, client, client_caps,
                       features, identity, tmp, False)

    sync_stream(q, stream)

    # now finally ensure the sidecar
    path, props = conn.Future.EnsureSidecar(ycs.STATUS_IFACE)
    assertEquals({}, props)

    status = ProxyWrapper(bus.get_object(conn.bus_name, path),
                          ycs.STATUS_IFACE, {})

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({bare_jid: {
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
    exec_test(test, do_connect=False)
