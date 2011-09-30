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
    assertLength, assertContains, sync_dbus, ProxyWrapper
from gabbletest import exec_test, elem_iq, elem
from gabblecaps_helper import presence_and_disco, receive_presence_and_ask_caps, \
    disco_caps

import gabbleconstants as cs
import ns
import yconstants as ycs

client = 'http://telepathy.im/fake'
features = [
    ns.JINGLE_015,
    ns.JINGLE_015_AUDIO,
    ns.JINGLE_015_VIDEO,
    ns.GOOGLE_P2P,
    ycs.SERVICE_NS + '#the.target.service'
    ]
identity = ['client/pc/en/Lolclient 0.L0L']

def test(q, bus, conn, stream):
    bare_jid = "test-hct@example.com"
    full_jid = bare_jid + "/LikeLava"

    call_async(q, conn.Future, 'EnsureSidecar', ycs.STATUS_IFACE)
    conn.Connect()

    q.expect('dbus-signal', signal='StatusChanged', args=[0, 1])

    e = q.expect('dbus-return', method='EnsureSidecar')
    path, props = e.value
    assertEquals({}, props)

    status = ProxyWrapper(bus.get_object(conn.bus_name, path),
                          ycs.STATUS_IFACE, {})

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

    self_handle = conn.GetSelfHandle()
    self_handle_name =  conn.InspectHandles(cs.HT_CONTACT, [self_handle])[0]

    caps = {'ver': '0.1', 'node': client}
    presence_and_disco(q, conn, stream, full_jid, True, client, caps,
                       features, identity, {}, True, None)

    # now update the caps
    conn.ContactCapabilities.UpdateCapabilities([
        ('well.gnome.name', [],
         ['com.meego.xpmn.ytstenut.Channel/uid/org.gnome.Banshee',
          'com.meego.xpmn.ytstenut.Channel/type/application',
          'com.meego.xpmn.ytstenut.Channel/name/en_GB/Banshee Media Player',
          'com.meego.xpmn.ytstenut.Channel/name/fr/Banshee Lecteur de Musique',
          'com.meego.xpmn.ytstenut.Channel/caps/urn:ytstenut:capabilities:yts-caps-audio',
          'com.meego.xpmn.ytstenut.Channel/caps/urn:ytstenut:data:jingle:rtp'])])


    _, e = q.expect_many(EventPattern('dbus-signal', signal='ServiceAdded'),
                         EventPattern('stream-presence'))

    e, _, _ = disco_caps(q, stream, e)

    iq = e.stanza
    query = iq.children[0]

    x = None
    for child in query.children:
        if child.name == 'x' and child.uri == ns.X_DATA:
            # we should only have one child
            assert x is None
            x = child
            # don't break here as we can waste time to make sure x
            # isn't assigned twice

    assert x is not None

    for field in x.children:
        if field['var'] == 'FORM_TYPE':
            assertEquals('hidden', field['type'])
            assertEquals('urn:ytstenut:capabilities#org.gnome.Banshee',
                         field.children[0].children[0])
        elif field['var'] == 'type':
            assertEquals('application', field.children[0].children[0])
        elif field['var'] == 'name':
            names = [a.children[0] for a in field.children]
            assertLength(2, names)
            assertContains('en_GB/Banshee Media Player', names)
            assertContains('fr/Banshee Lecteur de Musique', names)
        elif field['var'] == 'capabilities':
            caps = [a.children[0] for a in field.children]
            assertLength(2, caps)
            assertContains('urn:ytstenut:capabilities:yts-caps-audio', caps)
            assertContains('urn:ytstenut:data:jingle:rtp', caps)
        else:
            assert False

    # now add another service
    forbidden = [EventPattern('dbus-signal', signal='ServiceRemoved')]
    q.forbid_events(forbidden)

    conn.ContactCapabilities.UpdateCapabilities([
        ('another.nice.gname', [],
         ['com.meego.xpmn.ytstenut.Channel/uid/org.gnome.Eog',
          'com.meego.xpmn.ytstenut.Channel/type/application',
          'com.meego.xpmn.ytstenut.Channel/name/en_GB/Eye Of Gnome',
          'com.meego.xpmn.ytstenut.Channel/name/it/Occhio Di uno Gnomo',
          'com.meego.xpmn.ytstenut.Channel/caps/urn:ytstenut:capabilities:yts-picz'])])

    e = q.expect('dbus-signal', signal='ServiceAdded')

    sync_dbus(bus, q, conn)

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'test@localhost':
                      {'org.gnome.Banshee': ('application',
                                             {'en_GB': 'Banshee Media Player',
                                              'fr': 'Banshee Lecteur de Musique'},
                                             ['urn:ytstenut:capabilities:yts-caps-audio',
                                              'urn:ytstenut:data:jingle:rtp']),
                       'org.gnome.Eog': ('application',
                                         {'en_GB': 'Eye Of Gnome',
                                          'it': 'Occhio Di uno Gnomo'},
                                         ['urn:ytstenut:capabilities:yts-picz'])}},
                 discovered)

    q.unforbid_events(forbidden)

    forbidden = [EventPattern('dbus-signal', signal='ServiceRemoved',
                              args=[self_handle_name, 'org.gnome.Eog'])]
    q.forbid_events(forbidden)

    conn.ContactCapabilities.UpdateCapabilities([
            ('well.gnome.name', [], [])])

    e = q.expect('dbus-signal', signal='ServiceRemoved',
                 args=[self_handle_name, 'org.gnome.Banshee'])

    sync_dbus(bus, q, conn)

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'test@localhost':
                      {'org.gnome.Eog': ('application',
                                         {'en_GB': 'Eye Of Gnome',
                                          'it': 'Occhio Di uno Gnomo'},
                                         ['urn:ytstenut:capabilities:yts-picz'])}},
                 discovered)

    q.unforbid_events(forbidden)

    conn.ContactCapabilities.UpdateCapabilities([
            ('another.nice.gname', [], [])])

    e = q.expect('dbus-signal', signal='ServiceRemoved',
                 args=[self_handle_name, 'org.gnome.Eog'])

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredServices',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

if __name__ == '__main__':
    exec_test(test, do_connect=False)
