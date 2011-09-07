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

# TODO: move more of the common parts of this test into different
# functions to cut out the duplication!

def test(q, bus, conn, stream):
    call_async(q, conn.Future, 'EnsureSidecar', ycs.STATUS_IFACE)

    conn.Connect()

    # Now we're connected, the call we made earlier should return.
    e = q.expect('dbus-return', method='EnsureSidecar')
    path, props = e.value
    assertEquals({}, props)

    status = ProxyWrapper(bus.get_object(conn.bus_name, path),
                          ycs.STATUS_IFACE, {})

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

    # announce a contact with the right caps
    bare_jid = "test-service@example.com"
    full_jid = bare_jid + "/NeeNawNeeNawIAmAnAmbulance"

    contact_handle = conn.RequestHandles(cs.HT_CONTACT, [bare_jid])[0]

    client_caps['ver'] = compute_caps_hash(identity, features, banshee)
    presence_and_disco(q, conn, stream, full_jid, True, client, client_caps,
                       features, identity, banshee, True, None)

    # this will be fired as text channel caps will be fired
    _, e = q.expect_many(EventPattern('dbus-signal', signal='ContactCapabilitiesChanged',
                                      predicate=lambda e: contact_handle in e.args[0]),
                         EventPattern('dbus-signal', signal='ServiceAdded'))

    contact_id, service_name, details = e.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Banshee', service_name)

    type, name_map, caps = details
    assertEquals('application', type)
    assertEquals({'en_GB': 'Banshee Media Player',
                  'fr': 'Banshee Lecteur de Musique'}, name_map)
    assertSameSets(['urn:ytstenut:capabilities:yts-caps-audio',
                    'urn:ytstenut:data:jingle:rtp'], caps)

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({bare_jid: {
                'org.gnome.Banshee':
                    ('application',
                     {'en_GB': 'Banshee Media Player',
                      'fr': 'Banshee Lecteur de Musique'},
                     ['urn:ytstenut:capabilities:yts-caps-audio',
                      'urn:ytstenut:data:jingle:rtp'])},
                }, discovered)

    # add evince
    tmp = banshee.copy()
    tmp.update(evince)
    client_caps['ver'] = compute_caps_hash(identity, features, tmp)
    presence_and_disco(q, conn, stream, full_jid, True, client, client_caps,
                       features, identity, tmp, False)

    e = q.expect('dbus-signal', signal='ServiceAdded')

    contact_id, service_name, details = e.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Evince', service_name)

    type, name_map, caps = details
    assertEquals('application', type)
    assertEquals({'en_GB': 'Evince Picture Viewer',
                  'fr': 'Evince uh, ow do you say'}, name_map)
    assertSameSets(['urn:ytstenut:capabilities:pics'], caps)

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

    # remove evince
    forbidden = [EventPattern('dbus-signal', signal='stream-iq')]
    q.forbid_events(forbidden)

    client_caps['ver'] = compute_caps_hash(identity, features, banshee)
    send_presence(q, conn, stream, full_jid, client_caps, initial=False)

    e = q.expect('dbus-signal', signal='ServiceRemoved')

    contact_id, service_name = e.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Evince', service_name)

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({bare_jid: {
                'org.gnome.Banshee':
                    ('application',
                     {'en_GB': 'Banshee Media Player',
                      'fr': 'Banshee Lecteur de Musique'},
                     ['urn:ytstenut:capabilities:yts-caps-audio',
                      'urn:ytstenut:data:jingle:rtp'])},
                }, discovered)

    sync_stream(q, stream)

    q.unforbid_events(forbidden)

    # now just evince
    client_caps['ver'] = compute_caps_hash(identity, features, evince)
    presence_and_disco(q, conn, stream, full_jid, True, client, client_caps,
                       features, identity, evince, False)

    sa, sr = q.expect_many(EventPattern('dbus-signal', signal='ServiceAdded'),
                           EventPattern('dbus-signal', signal='ServiceRemoved'))

    contact_id, service_name, details = sa.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Evince', service_name)

    type, name_map, caps = details
    assertEquals('application', type)
    assertEquals({'en_GB': 'Evince Picture Viewer',
                  'fr': 'Evince uh, ow do you say'}, name_map)
    assertSameSets(['urn:ytstenut:capabilities:pics'], caps)

    contact_id, service_name = sr.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Banshee', service_name)

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({bare_jid: {
                  'org.gnome.Evince':
                      ('application',
                       {'en_GB': 'Evince Picture Viewer',
                        'fr': 'Evince uh, ow do you say'},
                       ['urn:ytstenut:capabilities:pics'])}
                }, discovered)

    # just banshee again
    forbidden = [EventPattern('dbus-signal', signal='stream-iq')]
    q.forbid_events(forbidden)

    client_caps['ver'] = compute_caps_hash(identity, features, banshee)
    send_presence(q, conn, stream, full_jid, client_caps, initial=False)

    sr, sa = q.expect_many(EventPattern('dbus-signal', signal='ServiceRemoved'),
                           EventPattern('dbus-signal', signal='ServiceAdded'))

    contact_id, service_name = sr.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Evince', service_name)

    contact_id, service_name, details = sa.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Banshee', service_name)

    type, name_map, caps = details
    assertEquals('application', type)
    assertEquals({'en_GB': 'Banshee Media Player',
                  'fr': 'Banshee Lecteur de Musique'}, name_map)
    assertSameSets(['urn:ytstenut:capabilities:yts-caps-audio',
                    'urn:ytstenut:data:jingle:rtp'], caps)

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({bare_jid: {
                'org.gnome.Banshee':
                    ('application',
                     {'en_GB': 'Banshee Media Player',
                      'fr': 'Banshee Lecteur de Musique'},
                     ['urn:ytstenut:capabilities:yts-caps-audio',
                      'urn:ytstenut:data:jingle:rtp'])}
                }, discovered)

    sync_stream(q, stream)

    # both again
    client_caps['ver'] = compute_caps_hash(identity, features, tmp)
    send_presence(q, conn, stream, full_jid, client_caps, initial=False)

    sa = q.expect('dbus-signal', signal='ServiceAdded')

    contact_id, service_name, details = sa.args
    assertEquals(bare_jid, contact_id)
    assertEquals('org.gnome.Evince', service_name)

    type, name_map, caps = details
    assertEquals('application', type)
    assertEquals({'en_GB': 'Evince Picture Viewer',
                  'fr': 'Evince uh, ow do you say'}, name_map)
    assertSameSets(['urn:ytstenut:capabilities:pics'], caps)

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

    sync_stream(q, stream)

    q.unforbid_events(forbidden)

    # and finally, nothing
    client_caps['ver'] = compute_caps_hash(identity, features, {})
    presence_and_disco(q, conn, stream, full_jid, True, client, client_caps,
                       features, identity, {}, False)

    q.expect_many(EventPattern('dbus-signal', signal='ServiceRemoved',
                               args=[bare_jid, 'org.gnome.Banshee']),
                  EventPattern('dbus-signal', signal='ServiceRemoved',
                               args=[bare_jid, 'org.gnome.Evince']))

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

    # super.

if __name__ == '__main__':
    exec_test(test, do_connect=False)
