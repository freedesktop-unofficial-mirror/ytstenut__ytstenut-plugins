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
    ProxyWrapper, assertNotEquals
from saluttest import exec_test, wait_for_contact_in_publish, make_result_iq
import salutconstants as cs
import yconstants as ycs
from caps_helper import *

from twisted.words.xish import xpath
from twisted.words.xish.domish import Element
from twisted.words.xish import domish

from avahitest import AvahiAnnouncer, AvahiListener
from avahitest import get_host_name
from xmppstream import setup_stream_listener, connect_to_stream

CAP_NAME = 'h264-over-ants'
CLIENT_NAME = 'fake-client'

def test(q, bus, conn):
    call_async(q, conn.Future, 'EnsureSidecar', ycs.STATUS_IFACE)

    conn.Connect()

    # Now we're connected, the call we made earlier should return.
    e = q.expect('dbus-return', method='EnsureSidecar')
    path, props = e.value
    assertEquals({}, props)

    status = ProxyWrapper(bus.get_object(conn.bus_name, path),
                          ycs.STATUS_IFACE, {})

    # bad capability argument
    call_async(q, status, 'AdvertiseStatus', '', 'service.name', '')
    q.expect('dbus-error', method='AdvertiseStatus')

    # bad service name
    call_async(q, status, 'AdvertiseStatus', CAP_NAME, '', '')
    q.expect('dbus-error', method='AdvertiseStatus')

    # we can't test that the message type="headline" stanza is
    # actually received because it's thrown into the loopback stream
    # immediately.

    # announce a contact with the right caps
    ver = compute_caps_hash([], [ycs.CAPABILITIES_PREFIX + CAP_NAME + '+notify'], [])
    txt_record = { "txtvers": "1", "status": "avail",
        "node": CLIENT_NAME, "ver": ver, "hash": "sha-1"}
    contact_name = "test-status@" + get_host_name()
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

    feature = query.addElement('feature')
    feature['var'] = ycs.CAPABILITIES_PREFIX + CAP_NAME + '+notify'
    incoming.send(result)

    # this will be fired as text channel caps will be fired
    q.expect('dbus-signal', signal='ContactCapabilitiesChanged',
             predicate=lambda e: contact_handle in e.args[0])

    # okay now we know about the contact's caps, we can go ahead

    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({}, discovered)

    el = Element(('urn:ytstenut:status', 'status'))
    el['activity'] = 'messing-with-your-stuff'
    desc = el.addElement('ytstenut:description', content='Yeah sorry about that')
    desc['xml:lang'] = 'en-GB'

    call_async(q, status, 'AdvertiseStatus', CAP_NAME,
               'ants.in.their.pants', el.toXml())

    e, _, sig = q.expect_many(EventPattern('stream-message', connection=incoming),
                              EventPattern('dbus-return', method='AdvertiseStatus'),
                              EventPattern('dbus-signal', signal='StatusChanged',
                                           interface=ycs.STATUS_IFACE))

    # check message
    message = e.stanza
    event = message.children[0]
    items = event.children[0]
    item = items.children[0]
    status_el = item.children[0]

    assertEquals('status', status_el.name)
    assertEquals('messing-with-your-stuff', status_el['activity'])
    assertEquals('ants.in.their.pants', status_el['from-service'])
    assertEquals(CAP_NAME, status_el['capability'])

    # check signal
    contact_id, capability, service_name, status_str = sig.args
    assertEquals(CAP_NAME, capability)
    assertEquals('ants.in.their.pants', service_name)
    assertNotEquals('', status_str)

    # check property
    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'testsuite@testsuite': {CAP_NAME: {'ants.in.their.pants': status_str}}},
                 discovered)

    # unset the status
    call_async(q, status, 'AdvertiseStatus', CAP_NAME,
               'ants.in.their.pants', '')

    e, _, sig = q.expect_many(EventPattern('stream-message', connection=incoming),
                              EventPattern('dbus-return', method='AdvertiseStatus'),
                              EventPattern('dbus-signal', signal='StatusChanged',
                                           interface=ycs.STATUS_IFACE))

    # check message
    message = e.stanza
    event = message.children[0]
    items = event.children[0]
    item = items.children[0]
    status_el = item.children[0]

    assertEquals('status', status_el.name)
    assertEquals('ants.in.their.pants', status_el['from-service'])
    assertEquals(CAP_NAME, status_el['capability'])
    assert 'activity' not in status_el.attributes
    assertEquals([], status_el.children)

    # check signal
    contact_id, capability, service_name, status_str = sig.args
    assertEquals(CAP_NAME, capability)
    assertEquals('ants.in.their.pants', service_name)
    assertEquals('', status_str)

    # check property
    discovered = status.Get(ycs.STATUS_IFACE, 'DiscoveredStatuses',
                            dbus_interface=dbus.PROPERTIES_IFACE)
    assertEquals({'testsuite@testsuite': {CAP_NAME: {'ants.in.their.pants': ''}}},
                 discovered)

if __name__ == '__main__':
    exec_test(test)
