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

import avahi
import dbus

from salutservicetest import call_async, EventPattern, assertEquals, \
    assertLength, assertContains, sync_dbus, ProxyWrapper
from saluttest import exec_test, elem_iq, elem
from avahitest import AvahiAnnouncer, AvahiListener, txt_get_key, get_host_name
from xmppstream import connect_to_stream
import salutconstants as cs
import ns
import yconstants as ycs

def test(q, bus, conn):
    contact_name = "test-hct@" + get_host_name()

    call_async(q, conn.Future, 'EnsureSidecar', ycs.STATUS_IFACE)
    conn.Connect()

    q.expect('dbus-signal', signal='StatusChanged', args=[0, 0])

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

    AvahiListener(q).listen_for_service("_presence._tcp")
    e = q.expect('service-added', name = self_handle_name,
        protocol = avahi.PROTO_INET)
    service = e.service
    service.resolve()

    q.expect('service-resolved', service=service)

    # now update the caps
    conn.ContactCapabilities.UpdateCapabilities([
        ('well.gnome.name', [],
         ['com.meego.xpmn.ytstenut.Channel/uid/org.gnome.Banshee',
          'com.meego.xpmn.ytstenut.Channel/type/application',
          'com.meego.xpmn.ytstenut.Channel/name/en_GB/Banshee Media Player',
          'com.meego.xpmn.ytstenut.Channel/name/fr/Banshee Lecteur de Musique',
          'com.meego.xpmn.ytstenut.Channel/caps/urn:ytstenut:capabilities:yts-caps-audio',
          'com.meego.xpmn.ytstenut.Channel/caps/urn:ytstenut:data:jingle:rtp'])])

    e, _ = q.expect_many(EventPattern('service-resolved', service=service),
                         EventPattern('dbus-signal', signal='ServiceAdded'))
    node = txt_get_key(e.txt, 'node')
    ver = txt_get_key(e.txt, 'ver')

    # don't check the hash here because it could change in salut and
    # it would break this test even though it wouldn't /actually/ be
    # broken

    outbound = connect_to_stream(q, contact_name,
        self_handle_name, str(e.pt), e.port)
    e = q.expect('connection-result')
    assert e.succeeded, e.reason
    e = q.expect('stream-opened', connection=outbound)

    # ask what this ver means
    request = \
        elem_iq(outbound, 'get', from_=contact_name)(
          elem(ns.DISCO_INFO, 'query', node=(node + '#' + ver))
        )
    outbound.send(request)

    # receive caps
    e = q.expect('stream-iq', connection=outbound,
                 query_ns=ns.DISCO_INFO, iq_id=request['id'])

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
    assertEquals({'testsuite@testsuite':
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
    assertEquals({'testsuite@testsuite':
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
    exec_test(test)
